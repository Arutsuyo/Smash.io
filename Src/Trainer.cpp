#include "Trainer.h"
#include <fstream>
#include <signal.h>
#include <stdio.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#define FILENM "TRNR"

std::vector<int> Trainer::killpids;

Trainer* Trainer::_inst = NULL;

bool Trainer::term;

Config* Trainer::cfg;

// ISO locations, will step through
std::string Trainer::_ssbmisoLocs[] = {
    "/mnt/f/Program Files/Dolphin-x64/iso/ssbm.gcm",
    "/mnt/c/Program Files/Dolphin-x64/iso/ssbm.gcm",
    "/mnt/c/User/Nara/Desktop/Dolphin-x64/iso/ssbm.gcm",
    "/mnt/c/Users/aruts/OneDrive/UO/CIS 472/Project/ssbm.gcm",
    "/home/zach/Desktop/CIS472/ssbm.gcm"
};
unsigned int Trainer::_isoidx = -1;
int Trainer::memoryCount = 0;

std::string Trainer::ssbmOverride = "";

// To be filled out in main
std::string Trainer::userDir = "";
std::string Trainer::dolphinDefaultUser = "";

std::string Trainer::PythonCommand = "python.exe";
std::string Trainer::modelName = "ssbm";
int Trainer::predictionType = LOAD_MODEL;


// Used for tracking events in the threads
std::mutex Trainer::mut;
std::condition_variable Trainer::cv;
unsigned Trainer::Concurent = 1;

VsType Trainer::vs = VsType::Self;


/* Helper Functions */
bool file_exists(const char* path)
{
    if (FILE * file = fopen(path, "r")) {
        fclose(file);
        return true;
    }
    else {
        return false;
    }
}

// https://stackoverflow.com/q/18100097/2939859
bool dir_exists(const char* path)
{
    struct stat info;

    if (stat(path, &info) != 0)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:stat", strerror(errno));
        return false;
    }
    else if (S_ISDIR(info.st_mode))
        return true;
    else
        return false;
}

void sigint_handle(int val)
{
    if (val != SIGINT)
        return;

    printf("%s:%d\tReceived SIGINT, closing trainer\n", FILENM, __LINE__);
    Trainer::term = true;
    Trainer::_inst->KillDolphinHandles();
}

bool createSigIntAction()
{
    // Create signal action
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handle;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:sigaction", strerror(errno));
        return false;
    }

    printf("%s:%d\tSignal Handler Created\n", FILENM, __LINE__);
    return true;
}

void Trainer::KillDolphinHandles()
{
    for (unsigned int i = 0; i < _Dhandles.size(); i++)
        _Dhandles[i]->running = false;
}

void Trainer::GetVersionNumber(std::string& parsed)
{
    char version[32];
    std::fstream fs;
    fs.open("Version/version.txt", std::fstream::in);
    if (fs.fail())
    {
        fprintf(stderr, "%s:%d --Error: Version/version.txt is missing.\n", FILENM, __LINE__);
        exit(EXIT_FAILURE);
    }
    fs.getline(version, 32);
    fs.close();
    Trainer::modelName = parsed + version;
    printf("%s:%d\tUsing Model: %s\n", FILENM, __LINE__, modelName.c_str());
}

void Trainer::runTraining()
{
    if (!GenerationManager::Initialize(modelName))
    {
        fprintf(stderr, "%s:%d --Error: Couldn't initialize GenerationManager\n", FILENM, __LINE__);
        exit(EXIT_FAILURE);
    }
    //exit(EXIT_SUCCESS);
    printf("%s:%d\tInitializing Training.\n", FILENM, __LINE__);

    createSigIntAction();

    switch (vs)
    {
    case Self:
        printf("%s:%d\tPlaying vs Self\n", FILENM, __LINE__);
        break;
    case CPU:
        printf("%s:%d\tPlaying vs CPU\n", FILENM, __LINE__);
        break;
    case Human:
        printf("%s:%d\tPlaying vs Human\n", FILENM, __LINE__);
        break;
    default:
        break;
    }

    unsigned int numCreate = vs == VsType::Human ? 1 : Concurent;
    //int numCreate = 1;
    printf("%s:%d\tRunning %d Instance%s\n", FILENM, __LINE__, numCreate, numCreate > 1 ? "s" : "");
    // Cycle user folders to allow controllers to close completely.
    unsigned int usernum = 0;
    printf("%s:%d\tEntering Management Loop\n", FILENM, __LINE__);
    printf("%s:%d\t--Stop the Trainer with CTRL+C\n", FILENM, __LINE__);
    while (!term)
    {
        cv.notify_all();
        std::unique_lock<std::mutex> lk(mut);

        bool readyToCull = GenerationManager::GenerationReady();
        
        while (!readyToCull && _Dhandles.size() < numCreate)
        {
            printf("%s:%d\tCreating Handler %lu\n", FILENM, __LINE__, _Dhandles.size());
            DolphinHandle* dh = new DolphinHandle(vs);
            _Dhandles.push_back(dh);
        }

        for (unsigned int i = 0; i < _Dhandles.size(); i++)
        {
            DolphinHandle* dh = _Dhandles[i];
            if (!dh->started && !readyToCull)
            {
                lk.unlock();
                cv.notify_all();
                printf("%s:%d\tStarting(0) Dolphin Instance %d\n", FILENM, __LINE__, i);
                if (!dh->StartDolphin(usernum++))
                {
                    printf("%s:%d\t--ERROR: Dolphin Failed to start(0)\n", FILENM, __LINE__);
                    term = true;
                    break;
                }
                printf("%s:%d\tDolphin Instance %d Started(0)\n", FILENM, __LINE__, i);
                cv.notify_all();
                lk.lock();
            }
        }

        if (term)
            break;

        printf("%s:%d\tChecking if running\n", FILENM, __LINE__);
        for (unsigned int i = 0; i < _Dhandles.size(); i++)
        {
            DolphinHandle* dh = _Dhandles[i];
            // Check if the match finished
            if (!dh->running && dh->started)
            {
                printf("%s:%d\tDolphin Instance %d stopped\n", FILENM, __LINE__, i);
                if (dh->safeclose)
                    printf("%s:%d\tDolphin Instance Closed safely\n", FILENM, __LINE__);
                else
                {
                    fprintf(stderr, "%s:%d\t--ERROR:Dolphin failed to close safely\n", FILENM, __LINE__);
                    term = true;
                    break;
                }
                dh->~DolphinHandle();
                // Remove the handler, calling the destructor
                _Dhandles.erase(_Dhandles.begin() + i);

                if (!readyToCull)
                {
                    // push a new one
                    DolphinHandle* dh = new DolphinHandle(vs);
                    lk.unlock();
                    cv.notify_all();
                    printf("%s:%d\tStarting(1) Dolphin Instance %d\n", FILENM, __LINE__, i);
                    if (!dh->StartDolphin(usernum++))
                    {
                        fprintf(stderr, "%s:%d\t--ERROR:Dolphin Failed to start(1)", FILENM, __LINE__);
                        term = true;
                        break;
                    }
                    printf("%s:%d\tDolphin Instance %d Started(1)\n", FILENM, __LINE__, i);
                    _Dhandles.push_back(dh);
                    cv.notify_all();
                    lk.lock();
                }
            }
        }

        if (term)
            break;

        if (readyToCull && !_Dhandles.size())
        {
            term = !GenerationManager::CullTheWeak();

            // We can now load the new model!
            if (predictionType == NEW_MODEL)
                predictionType = LOAD_MODEL;

            // Purge the old, in with the new
            char buff[256];
            memset(buff, 0, 256);
            sprintf(buff, "rm -rf %s/ssbm*", Trainer::userDir.c_str());
            system(buff);
        }

        printf("%s:%d\tWaiting for notification\n", FILENM, __LINE__);
        // Lock the mutex and wait for the condition variable
        cv.wait_for(lk, std::chrono::seconds(5));
    }
}

Trainer::Trainer()
{
    if (ssbmOverride.size())
    {
        if (!file_exists(ssbmOverride.c_str()))
        {
            fprintf(stderr, "%s:%d: %s\n", FILENM, __LINE__,
                "--ERROR:GCM not found");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s:%d\tUsing GCM:\n\t%s\n", FILENM, __LINE__, 
                ssbmOverride.c_str());
        }
    }
    else
    {

        size_t n = sizeof(_ssbmisoLocs) / sizeof(_ssbmisoLocs[0]);
        printf("%s:%d\tChecking %lu Locations\n", FILENM, __LINE__, n);
        do {
            _isoidx++;
            if (_isoidx == n)
            {
                fprintf(stderr, "%s:%d: %s\n", FILENM, __LINE__,
                    "--ERROR:GCM not found");
                exit(EXIT_FAILURE);
            }
            printf("%s:%d\tTesting for GCM:\n\t%s\n", FILENM, __LINE__, _ssbmisoLocs[_isoidx].c_str());
        } while (!file_exists(_ssbmisoLocs[_isoidx].c_str()));
        printf("%s:%d\tGCM Found: %s\n", FILENM, __LINE__, _ssbmisoLocs[_isoidx].c_str());

    }
    if (!cfg)
    {
        printf("%s:%d\tCreating Config\n", FILENM, __LINE__);
        cfg = new Config(vs);
        memoryCount = cfg->getMemlocationLines();
    }

    initialized = true;
    _inst = this;
}

Trainer::~Trainer()
{
    printf("%s:%d\tDestroying Trainer\n", FILENM, __LINE__);
    if (cfg)
        cfg->~Config();

    printf("%s:%d\tClosing Handles\n", FILENM, __LINE__);
    // Call each destructor
    while (_Dhandles.size() > 0)
    {
        DolphinHandle* dh = _Dhandles[0];
        dh->~DolphinHandle();
        _Dhandles.pop_back();
    }
}
