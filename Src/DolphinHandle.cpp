#include "DolphinHandle.h"
#include "TensorHandler.h"
#include "Player.h" 
#include "Trainer.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#define FILENM "DH"

#ifdef WIN32
#define SIGKILL 9
int fork() {};
void kill(int pid, int signal) {};
int wait(int* status) {};
#endif


bool WriteToFile(std::string filename, std::string contents)
{
    printf("%s:%d\tWriting %s\n", FILENM, __LINE__, filename.c_str());
    FILE* fd = fopen(filename.c_str(), "w");
    if (!fd)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:fopen", strerror(errno));
        return false;
    }
    fwrite(contents.c_str(), sizeof(char), contents.size(), fd);
    fclose(fd);
    return true;
}

void SendKill(int pid)
{
    printf("%s:%d\tSending kill signal\n", FILENM, __LINE__);
    kill(pid, SIGKILL);
}

bool WaitForDolphinToClose(int pid)
{
    int status;
    int tpid;
    SendKill(pid);
    tpid = waitpid(pid, &status, 0);
    printf("%s:%d\tChild(Status:%d) Exited\n", FILENM, __LINE__, status);
    return true;
}

void DolphinHandle::CopyBaseFiles()
{
    printf("%s:%d\tCopying Base Files\n", FILENM, __LINE__);
    char buff[1028];
    sprintf(buff, "cp -r ./Files/* %s", dolphinUser.c_str());
    system(buff);
}

std::string DolphinHandle::AddController(int player, int pipe_count, std::string id)
{
    printf("%s:%d\tCreating AI Controller: %d\n", FILENM, __LINE__, player + 1);
    Controller* ctrl = new Controller(player); // TODO add delay arg to main
    !ctrl->CreateFifo(aiPipe, pipe_count);
    controllers.push_back(ctrl);
    return Trainer::cfg->getAIPipeConfig(player, pipe_count, id);
}

bool DolphinHandle::dolphin_thread(ThreadArgs* targ)
{
    printf("%s:%d-T\tThread Started\n", FILENM, __LINE__);
    ThreadArgs ta = *targ;
    *ta._pid = fork();
    // Child
    if (*ta._pid == 0)
    {
        printf("%s:%d-T\tLaunching Dolphin\n", FILENM, __LINE__);
        execlp("dolphin-emu",
            "-b",
            "-e",
            Trainer::_ssbmisoLocs[Trainer::_isoidx].c_str(),
            "-u",
            ta._dolphinUser.c_str(),
            NULL);

        fprintf(stderr, "%s:%d-T\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:execlp", strerror(errno));
        return false;
    } // child will not exit this block

    // Parent Code

    // Check if Fork Failed
    if (*ta._pid == -1)
    {
        fprintf(stderr, "%s:%d-T\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:execlp", strerror(errno));
        *ta._running = false;
        return false;
    }

    printf("%s:%d-T%d\tChild successfully launched\n",
        FILENM, __LINE__, *ta._pid);
    Trainer::AddToKillList(*ta._pid);

    // Do this AFTER launching Dolphin, otherwise it will block
    printf("%s:%d-T%d\tOpening controllers\n",
        FILENM, __LINE__, *ta._pid);
    std::vector<TensorHandler*> tHandles;
    for (int i = 0; i < (*ta._controllers).size(); i++)
    {
        printf("%s:%d-T%d\tOpening Controller %d\n",
            FILENM, __LINE__, *ta._pid, i);
        if (!(*ta._controllers)[i]->OpenController())
        {
            *ta._running = false;
            Trainer::cv.notify_all();
            return false;
        }
        printf("%s:%d-T%d\tLinking Controller with Tensor\n",
            FILENM, __LINE__, *ta._pid);
        TensorHandler* th = new TensorHandler;
        th->CreatePipes((*ta._controllers)[i]);
        tHandles.push_back(th);
    }

    printf("%s:%d-T%d\tCreating Memory Watcher!\n",
        FILENM, __LINE__, *ta._pid);
    MemoryWatcher mem = MemoryWatcher(ta._dolphinUser, true);
    if (mem.success == false) {
        fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__,
            "--ERROR:Failed to initialize Memory Scanner");
        Trainer::cv.notify_all();
        return false;
    }

    // Do Input
    Trainer::cv.notify_all();
    printf("%s:%d-T%d\tPausing to load Menu\n",
        FILENM, __LINE__, *ta._pid);
    //wait until the character stage is detected
    int loop = 0, loopLim = 1000;
    TPoint lastSent = std::chrono::high_resolution_clock::now();
    while (mem.current_Menu != CHARACTER_SELECT && *ta._running)
    {
        if (loop++ == loopLim)
        {
            loop = 0;

            std::chrono::duration<double> elapsed =
                std::chrono::high_resolution_clock::now()
                - lastSent;
            if (elapsed.count() > 3) // wait for 3 seconds
            {
                lastSent = std::chrono::high_resolution_clock::now();
                printf("%s:%d-T%d\t%dPausing to load Menu(%d): %d\n",
                    FILENM, __LINE__, *ta._pid, *ta._running ? 1 : 0,
                    mem.current_Menu, CHARACTER_SELECT);
            }
        }

        //update the frame to find the current state
        if (!mem.ReadMemory(true))
        {
            fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__,
                "--ERROR:Memory update failed");
            Trainer::cv.notify_all();
            return false;
        }
    }
    if (CheckClose(ta))
        return false;


    printf("%s:%d-T%d\tSelecting Characters\n",
        FILENM, __LINE__, *ta._pid);
    while (mem.current_Menu != STAGE_SELECT && *ta._running)
    {
        if (loop++ == loopLim)
        {
            loop = 0;

            std::chrono::duration<double> elapsed =
                std::chrono::high_resolution_clock::now()
                - lastSent;
            if (elapsed.count() > 3) // wait for 3 seconds
            {
                lastSent = std::chrono::high_resolution_clock::now();
                printf("%s:%d-T%d\tSelecting Characters(%d): %d\n",
                    FILENM, __LINE__, *ta._pid,
                    mem.current_Menu, STAGE_SELECT);
            }
        }

        //update the frame to find the current cursor pos
        if (!mem.ReadMemory(true))
        {
            fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__,
                "--ERROR:Memory update failed");
            Trainer::cv.notify_all();
            return false;
        }
        bool selected = true;
        for (int i = 0; i < tHandles.size(); i++)
            selected &= tHandles[i]->SelectLocation(&mem, false);
        if (selected)
            (*ta._controllers).back()->ButtonPressRelease("START");
    }
    if (CheckClose(ta))
        return false;

    printf("%s:%d-T%d\tSelecting Stage\n",
        FILENM, __LINE__, *ta._pid);
    while (mem.current_Menu != IN_GAME && *ta._running)
    {
        if (loop++ == loopLim)
        {
            loop = 0;

            std::chrono::duration<double> elapsed =
                std::chrono::high_resolution_clock::now()
                - lastSent;
            if (elapsed.count() > 3) // wait for 3 seconds
            {
                lastSent = std::chrono::high_resolution_clock::now();
                printf("%s:%d-T%d\tSelecting Stage(%d): %d\n",
                    FILENM, __LINE__, *ta._pid,
                    mem.current_Menu, IN_GAME);
            }
        }

        //update the frame to find the current cursor pos
        if (!mem.ReadMemory(true))
        {
            fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__,
                "--ERROR:Memory update failed");
            Trainer::cv.notify_all();
            return false;
        }
        if (tHandles[0]->SelectLocation(&mem, true))
            (*ta._controllers).front()->ButtonPressRelease("A");
    }
    if (CheckClose(ta))
        return false;

    // Check mem until we get valid player numbers
    printf("%s:%d-T%d\tChecking for valid player data\n",
        FILENM, __LINE__, *ta._pid);
    while (*ta._running && !mem.print())
    {
        if (loop++ == loopLim)
        {
            loop = 0;

            std::chrono::duration<double> elapsed =
                std::chrono::high_resolution_clock::now()
                - lastSent;
            if (elapsed.count() > 3) // wait for 3 seconds
            {
                lastSent = std::chrono::high_resolution_clock::now();
                printf("%s:%d-T%d\tChecking for valid player data: %d:%f\n",
                    FILENM, __LINE__, *ta._pid,
                    mem.GetPlayer(false).facing, mem.GetPlayer(false).pos_x);
            }
        }

        mem.ReadMemory(true);
    }
    if (CheckClose(ta))
        return false;

    printf("%s:%d-T%d\tReady for input!\n",
        FILENM, __LINE__, *ta._pid);
    int memory_update;
    bool openSocket = true, openPipe = true;
    while (*ta._running && openPipe && openSocket)
    {
        if (!mem.ReadMemory(true))
        {
            fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__,
                "--ERROR:Memory update failed");
            break;
        }

        for (int i = 0; i < tHandles.size(); i++)
            openPipe = tHandles[i]->MakeExchange(&mem);
    }

    // Close out
    *ta._running = false;
    return CheckClose(ta);
}

bool DolphinHandle::CheckClose(ThreadArgs& ta)
{
    if (*ta._running)
        return false;
    // Closing, notify the trainer
    printf("%s:%d-T%d: Closing Thread\n",
        FILENM, __LINE__, *ta._pid);
    WaitForDolphinToClose(*ta._pid);
    Trainer::cv.notify_all();
    return true;
}

bool DolphinHandle::StartDolphin(int lst)
{
    char buff[256];
    sprintf(buff, "/ssbm%d/", lst);
    dolphinUser = Trainer::userDir + buff;
    std::string dolphinConfig = dolphinUser + "Config/";

    // Copy the user folder
    printf("%s:%d\tPrepping the user folder: %s\n", FILENM, __LINE__,
        dolphinUser.c_str());
    sprintf(buff, "cp -r %s %s", Trainer::dolphinDefaultUser.c_str(), dolphinUser.c_str());
    system(buff);

    CopyBaseFiles();

    running = true;
    int player = 0;
    int pipe_count = 0;
    std::string id = "ssbm.io";

    // Making sure the pipe folder exists
    aiPipe = dolphinUser + "Pipes/";
    sprintf(buff, "mkdir %s", aiPipe.c_str());
    system(buff);
    aiPipe += id;

    // Write the pipe ini
    std::string GCPadNew = dolphinConfig + "GCPadNew.ini";
    printf("%s:%d\tWriting %s\n", FILENM, __LINE__, GCPadNew.c_str());
    sprintf(buff, "mkdir %s", dolphinConfig.c_str());
    system(buff);

    printf("%s:%d\tCreating MemoryWatcher Directory\n", FILENM, __LINE__);
    std::string memwatch = dolphinUser + "MemoryWatcher/";
    sprintf(buff, "mkdir %s", memwatch.c_str());
    system(buff);
    memwatch += "Locations.txt";

    // Make the GCPadNew.ini
    std::string controllerINI = "";

    printf("%s:%d\tConstructing Controller INI\n", FILENM, __LINE__);
    std::string hotkey;
    switch (_vs)
    {
    case Human:
    case CPU:
        controllerINI += Trainer::cfg->getPlayerPipeConfig(player++);
        hotkey = Trainer::cfg->getHotkeyINI(player, pipe_count, id);
        controllerINI += AddController(player++, pipe_count++, id);
        break;

    case Self:
        controllerINI = AddController(player++, pipe_count++, id);
        hotkey = Trainer::cfg->getHotkeyINI(player, pipe_count, id);
        controllerINI += AddController(player++, pipe_count++, id);
        break;

    default:
        fprintf(stderr, "%s:%d\t%s %d\n", FILENM, __LINE__,
            "--ERROR:Invalid VS Type:", _vs);
        return false;
        break;
    }

    // Write the hotkey for save state
    dolphinConfig += "Hotkeys.ini";

    if (!WriteToFile(GCPadNew, controllerINI))
        return false;

    if (!WriteToFile(memwatch, Trainer::cfg->getLocations()))
        return false;

    if (!WriteToFile(dolphinConfig, hotkey))
        return false;

    ThreadArgs* ta = new ThreadArgs;
    ta->_running = &running;
    ta->_pid = &pid;
    ta->_dolphinUser = dolphinUser;
    ta->_controllers = &controllers;

    printf("%s:%d\tStarting Thread\n", FILENM, __LINE__);
    Trainer::cv.notify_all();
    std::unique_lock<std::mutex> lk(Trainer::mut);
    t = new std::thread(&DolphinHandle::dolphin_thread, ta);
    Trainer::cv.wait(lk);
    started = true;
    return true;
}

DolphinHandle::DolphinHandle(VsType vs) :
    t(NULL),
    _vs(vs)
{
    running = false;
    started = false;
}

DolphinHandle::~DolphinHandle()
{
    printf("%s:%d\tClosing DolphinHandle\n", FILENM, __LINE__);
    running = false;
    printf("%s:%d\tChecking Thread\n", FILENM, __LINE__);
    if (t && t->joinable())
    {
        printf("%s:%d\tJoining Thread\n", FILENM, __LINE__);
        t->join();
    }
    else
        printf("%s:%d\tThread is already destroyed\n", FILENM, __LINE__);

    // Call each destructor
    printf("%s:%d\tDestroying %lu Controllers\n", FILENM, __LINE__,
        controllers.size());

    // Call each destructor
    while (controllers.size() > 0)
    {
        Controller* ctrl = controllers[0];
        ctrl->~Controller();
        controllers.pop_back();
    }

    // Delete the user folder
    char buff[256];
    printf("%s:%d\tDeleting the created user folder\n", FILENM, __LINE__);
    sprintf(buff, "rm -rf %s", dolphinUser.c_str());
    system(buff);
}
