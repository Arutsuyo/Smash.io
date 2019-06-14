#include "TensorHandler.h"
#include "Trainer.h"
#include <sys/types.h> /* pid_t */
#include <cstring>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h> // Non blocking definitions
#include <unistd.h>

#ifdef WIN32
// Function definitions here are just because the 
// includes do not exist on windows, which is the platform 
// developed on.
size_t read(int fildes, void* buf, size_t nbytes) {};
size_t write(int fd, const void* buf, size_t count) {};
int pipe(int pipefd[2]) {};
typedef int pid_t;
pid_t fork(void) {};
int close(int fd) {};
int dup2(int fd, int fd1) {};
int execlp(const char* file, const char* arg, ...) {};
pid_t waitpid(pid_t pid, int* status, int options) {};
#define STDOUT_FILENO 0
#define STDIN_FILENO 1
#else
#include <unistd.h>
#endif
#define BUFF_SIZE 2048
#define FILENM "TH"

float TensorHandler::finalDest[2] = { 6.75, -8.74 };
float TensorHandler::battlefield[2] = { 0.745, -8.5 };
float TensorHandler::cptFalcon[2] = { 18.2, 18.29 };

bool TensorHandler::CreatePipes(Controller* ai)
{
    // Send initialization
    printf("%s:%d\tInitializing Tensor\n", FILENM, __LINE__);
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);

    std::string pModel = GenerationManager::GetParentFile();
    std::string cModel = GenerationManager::GetChildFile();

    printf("%s:%d\tCreating Write Pipe\n", FILENM, __LINE__);
    if (pipe(pipeToPy) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:Pipe", strerror(errno));
        return false;
    }

    // This pipe should block, as we want to guarantee that we read 
    printf("%s:%d\tCreating Read Pipe\n", FILENM, __LINE__);
    if (pipe(pipeFromPy) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:Pipe", strerror(errno));
        return false;
    }

    // This pipe shouldn't block
    printf("%s:%d\tCreating Error Pipe\n", FILENM, __LINE__);
    if (pipe2(pipeErrorFromPy, O_NONBLOCK) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:Pipe", strerror(errno));
        return false;
    }
    if ((pid = fork()) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:Fork", strerror(errno));
        return false;
    }

    else if (pid == 0) {
        // Child Process
        printf("%s:%d\tChild: Launching Tensor\n", FILENM, __LINE__);
        if (close(pipeToPy[1]) == -1) // Write end
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:close", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (close(pipeFromPy[0]) == -1) // Read end 
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:close", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (close(pipeErrorFromPy[0]) == -1) // Read end 
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:close", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* copy <oldfd> using <newfd> */
        if (dup2(pipeToPy[0], 0) == -1)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:dup2", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (dup2(pipeFromPy[1], 1) == -1)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:dup2", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (dup2(pipeErrorFromPy[1], 2) == -1)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:dup2", strerror(errno));
            exit(EXIT_FAILURE);
        }

        float dEpsilon = Trainer::predictionType > 1
            ? 0 : GenerationManager::GetEpsilon();

        /* Launch Python (EXE IS REQUIRED FOR WSL)*/
        execlp(
            Trainer::PythonCommand.c_str(),
            Trainer::PythonCommand.c_str(),
            "trainer.py",
            pModel.c_str(),
            cModel.c_str(),
            std::to_string(Trainer::Concurent * 2).c_str(),
            std::to_string(dEpsilon).c_str(),
            NULL);

        fprintf(stderr, "%s:%d\t%s(%d): %s\n", FILENM, __LINE__,
            "--ERROR:EXECLP", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (close(pipeToPy[0]) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:close", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (close(pipeFromPy[1]) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:close", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (close(pipeErrorFromPy[1]) == -1) // Read end 
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:close", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Send initialization
    printf("%s:%d\tInitializing Tensor\n", FILENM, __LINE__);
    memset(buff, 0, BUFF_SIZE);
    sprintf(buff, "0"); // This silences the debug printing
    int bytesWritten = strlen(buff);
    if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
        dumpErrorPipe();
        return false;
    }

    ctrl = ai;
    fprintf(stderr, " ");
    printf(" \n");
    dumpErrorPipe();
    return true;
}

void TensorHandler::dumpErrorPipe()
{
    if (pid == -1)
        return;

    char buff[BUFF_SIZE * 2];
    memset(buff, 0, BUFF_SIZE * 2);
    std::string output = "";
    int ret = 0;
    int status;
    ret = waitpid(pid, &status, WNOHANG);
    if (ret == pid || pid == -1)
    {
        fprintf(stderr, "%s:%d\t%s(%d)\n", FILENM, __LINE__,
            "--ERROR:NO NO TENSORFLOW!!! (Tensor Crashed)", status);
        pid = -1;
    }

    while (true)
    {
        // Get the current pipe buffer
        if ((ret = read(pipeErrorFromPy[0], buff, BUFF_SIZE * 2)) == -1)
        {
            // Check if the socket is just empty
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:Error pipe read failed", strerror(errno));
            return;
        }
        if (!ret)
            break;

        // step through the gunk until we find the prediction
        for (int i = 0; i < ret; i += (int)(output.size() + 1))
        {
            output = &buff[i];

#if TENSOR_ERR_PRINT
            fprintf(stderr, "%s:%d\tPyErr(%lu): ",
                FILENM, __LINE__, output.size());
#endif
            for (unsigned int j = 0; j < output.size(); j += strlen(output.c_str()) + 1)
            {
#if TENSOR_ERR_PRINT
                fprintf(stderr, "%s", output.c_str() + j);
#endif
            }
#if TENSOR_ERR_PRINT
            fprintf(stderr, "\n");
#endif
        }

    }
}

void TensorHandler::SendToPipe(Player ai, Player enemy)
{
    if (pid == -1)
        return;

#if CTRL_OUTPUT
    printf("%s:%d\tSending Current Data\n", FILENM, __LINE__);
#endif

    // Send input
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    sprintf(buff, "%u %d %f %f %u %f %u %d %f %f %u %f\n",
        ai.health, ai.dir, ai.pos_x, ai.pos_y, ai.action, ai.action_frame,
        enemy.health, enemy.dir, enemy.pos_x, enemy.pos_y, enemy.action, enemy.action_frame);
    int bytesWritten = strlen(buff);
    if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
        dumpErrorPipe();
    }

#if CTRL_OUTPUT
    printf("%s:%d\tSent: %s", FILENM, __LINE__, buff);
#endif

    dumpErrorPipe();
}

// Scrub through the pipe until we reach the identifier, return that
std::string TensorHandler::ReadFromPipe()
{
    dumpErrorPipe();
    char buff[BUFF_SIZE * 2];
    memset(buff, 0, BUFF_SIZE * 2);
    std::string output = "";
    int ret = 0;
    unsigned int offset = 0;

    while (true)
    {
        // Get the current pipe buffer
        if ((ret = read(pipeFromPy[0], buff, BUFF_SIZE * 2)) == -1)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:pipe read failed", strerror(errno));
            dumpErrorPipe();
            return "";
        }
        if (!ret)
            break;

        // Print what was read
        //printf("Read%d: %s\n", ret, buff);

        // step through the gunk until we find the prediction
        for (int i = 0; i < ret; i += output.size() + 1)
        {
            // Check for leading 0, message can be hiding beyond. . .
            if (buff[i] == '\0')
            {
                output = "";
                continue;
            }

            output = &buff[i];
            if ((offset = output.find("pred: ")) != std::string::npos)
                return output.substr(offset); // Add 6 to drop the "pred: "
            else
            {
                // Shouldn't trigger. . .
                if (!output.size())
                    continue;

                fprintf(stderr, "%s:%d\t%s(%lu): %s\n", FILENM, __LINE__,
                    "--TENSOR", output.size(), output.c_str());

                dumpErrorPipe();
                return ""; // Tensor probably crashed
            }
        }
    }
    return ""; // nothing read. . . probably crashed
}

bool TensorHandler::handleController(std::string tensor)
{
#if CTRL_OUTPUT
    printf("%s:%d\tParsing Tensor Output:\n\t%s\n",
        FILENM, __LINE__, tensor.c_str());
#endif

    float sx, sy;
    int ba, bb, by, bz, bl;
    // pred: 0.0 1.0 0 0 1 0 0
    if (sscanf(tensor.c_str(), "pred: %f %f %d %d %d %d %d",
        &sx, &sy, &ba, &bb, &by, &bz, &bl) < 7)
    {
        fprintf(stderr, "%s:%d\t%s\n\t%s\n", FILENM, __LINE__,
            "--ERROR:sscanf failed to parse tensor output", tensor.c_str());

        dumpErrorPipe();
        return false;
    }

#if CTRL_OUTPUT
    printf("%s:%d\tSending Controls to Controller\n", FILENM, __LINE__);
#endif
    return ctrl->setControls({ sx, sy, ba, bb, by, bz, bl });
}

bool TensorHandler::MakeExchange(MemoryScanner* mem)
{
    if (pid == -1)
        return false;

#if CTRL_OUTPUT
    printf("%s:%d\tMaking Exchange\n", FILENM, __LINE__);
#endif

    SendToPipe(mem->GetPlayer(ctrl->player), mem->GetPlayer(!ctrl->player));

    std::string ret = ReadFromPipe();
    if (ret.size() == 0)
    {
        dumpErrorPipe();
        return false;
    }

    dumpErrorPipe();
    return handleController(ret);
}

// Send false for character select and true for stage
bool TensorHandler::SelectLocation(MemoryScanner* mem, bool charStg)
{
    Player p = mem->GetPlayer(ctrl->player);
    float sx, sy;
    bool ba = false, bb = false, by = false, bz = false, bl = false;

    // Get distance
    float disx, disy;
    if (!charStg)
    {
        disx = cptFalcon[0] - p.cursor_x;
        disy = cptFalcon[1] - p.cursor_y;
    }
    else
    {
        disx = finalDest[0] - p.cursor_x;
        disy = finalDest[1] - p.cursor_y;
    }

#if CTRL_OUTPUT
    printf("%s:%d\tCursor Pos P%d: %4.3f %4.3f dist:%4.3f %4.3f\n", FILENM, __LINE__, ctrl->player ? 2 : 1, p.cursor_x, p.cursor_y, disx, disy);
#endif

    // Normalize
    if (disx > 1)
        disx = 1;
    else if (disx < -1)
        disx = -1;

    // Scale
    disx = disx / 2;
    disx += 0.5;

    // Normalize
    if (disy > 1)
        disy = 1;
    else if (disy < -1)
        disy = -1;

    // Scale
    disy = disy / 2;
    disy += 0.5;

    sx = disx;
    sy = disy;

    // Get absolute distance from mark
    float absx = sx - 0.5, absy = sy - 0.5;
    absx = absx < 0 ? -absx : absx;
    absy = absy < 0 ? -absy : absy;

    // Are we in a good deltax?
    if (absx > 0.5 && absx < 0.5)
    {
        // Move x then y (helps stage selection)
        sy = 0.5;
    }

    if (absx < 0.35 && absy < 0.35)
        ba = true;

    //printf("%s:%d\tSending Controls to Controller\n", FILENM, __LINE__);
    if (!ctrl->setControls({ sx, sy, ba, bb, by, bz, bl }) && !ctrl->IsPipeOpen())
        return false;

    return ba;
}

TensorHandler::TensorHandler() :
    pid(-1),
    pipeToPy{ -1,-1 },
    pipeFromPy{ -1,-1 }
{
}

TensorHandler::~TensorHandler()
{
    printf("%s:%d\tClosing TensorHandler\n", FILENM, __LINE__);
    // Write closing line to Python
    int status, ret;
    ret = waitpid(pid, &status, WNOHANG);
    if (ret == pid)
    {
        fprintf(stderr, "%s:%d\t%s(%d)\n", FILENM, __LINE__,
            "--ERROR:Tensor Crashed", status);
        dumpErrorPipe();
        pid = -1;
    }
    else if(pid != -1)
    {
        printf("%s:%d\tWriting Close\n", FILENM, __LINE__);
        char buff[BUFF_SIZE];
        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "exit please\n");
        int bytesWritten = strlen(buff);
        if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:write", strerror(errno));
            dumpErrorPipe();
        }

        // read from pipe
        printf("%s:%d\tWaiting for Tensor to signal closing\n", FILENM, __LINE__);
        // Read back close
        std::string readbuf;
        do
        {
            readbuf = ReadFromPipe();
        } while (readbuf.find("-1 -1") == std::string::npos);


        printf("%s:%d\tWaiting for Tensor to close\n", FILENM, __LINE__);
        int status;
        waitpid(pid, &status, 0);
        printf("%s:%d\tTensor(%d) closed\n", FILENM, __LINE__, status);
    }
    else
    {
        printf("%s:%d\tTensor already closed\n", FILENM, __LINE__);
    }

    // close the pipes
    dumpErrorPipe();
    printf("%s:%d\tShutting Down pipes\n", FILENM, __LINE__);
    close(pipeFromPy[0]);
    close(pipeToPy[1]);
    close(pipeErrorFromPy[0]);

    printf("%s:%d\tTensor Handler Destroyed\n", FILENM, __LINE__);
}
