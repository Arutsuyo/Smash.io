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

/* Helper Functions */
bool exists_test(const std::string & name) {
    if (FILE * file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    }
    else {
        return false;
    }
}

float TensorHandler::finalDest[2] = { 6.75, -8.74 };
float TensorHandler::battlefield[2] = { 0.745, -8.5 };
float TensorHandler::cptFalcon[2] = { 18.2, 18.29 };

bool TensorHandler::CreatePipes(Controller* ai)
{
    printf("%s:%d\tCreating Write Pipe\n", FILENM, __LINE__);
    if (pipe(pipeToPy) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:Pipe", strerror(errno));
        return false;
    }

    printf("%s:%d\tCreating Read Pipe\n", FILENM, __LINE__);
    if (pipe(pipeFromPy) == -1)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:Pipe", strerror(errno));
        return false;
    }

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

        /* Launch Python (EXE IS REQUIRED FOR WSL)*/
        execlp("python.exe", "python.exe", "trainer.py", NULL);

        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:EXECLP", strerror(errno));
        exit(EXIT_FAILURE);
    }

    Trainer::AddToKillList(pid);

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
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    sprintf(buff, "0");
    int bytesWritten = strlen(buff);
    if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
        return false;
    }

    if (exists_test("AI/ssbm.h5"))
        sprintf(buff, "1"); // Load existing file
    else
        sprintf(buff, "0"); // Make a new model
    bytesWritten = strlen(buff);
    if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
        return false;
    }

    ctrl = ai;

    dumpErrorPipe();
}

void TensorHandler::dumpErrorPipe()
{
    char buff[BUFF_SIZE * 2];
    memset(buff, 0, BUFF_SIZE * 2);
    std::string output = "";
    int ret = 0, offset = 0;

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
        for (int i = 0; i < ret; i += output.size() + 1)
        {
            output = &buff[i];
            fprintf(stderr, "%s:%d\tPyErr(%lu): ", 
                FILENM, __LINE__, output.size());
            for (int j = 0; j < output.size(); j+=strlen(output.c_str()) + 1)
            {
                fprintf(stderr, "%s", output.c_str() + j);
            }
            fprintf(stderr, "\n");
        }

    }
}

void TensorHandler::SendToPipe(Player ai, Player enemy)
{
    printf("%s:%d\tSending Current Data\n", FILENM, __LINE__);
    // Send input
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);

    sprintf(buff, "%hi %f %f %f %hi %f %f %f\n",
        ai.percent, ai.facing, ai.pos_x, ai.pos_y,
        enemy.percent, enemy.facing, enemy.pos_x, enemy.pos_y);

    int bytesWritten = strlen(buff);
    if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
    }

    printf("%s:%d\tSent: %s", FILENM, __LINE__, buff);

    dumpErrorPipe();
}

// Scrub through the pipe until we reach the identifier, return that
std::string TensorHandler::ReadFromPipe()
{
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    std::string output = "";
    int ret = 0, status, offset = 0;
    while (true)
    {
        ret = waitpid(pid, &status, WNOHANG);
        if (ret == pid)
        {
            fprintf(stderr, "%s:%d\t%s(%d)\n", FILENM, __LINE__,
                "--ERROR:NO NO TENSORFLOW!!! (Tensor Crashed)", status);
            dumpErrorPipe();
            return "";
        }

        // Get the current pipe buffer
        if ((ret = read(pipeFromPy[0], buff, BUFF_SIZE)) == -1)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:pipe read failed", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Print what was read
        //printf("Read%d: %s\n", ret, buff);

        // step through the gunk until we find the prediction
        for (int i = 0; i < ret; i += output.size() + 1)
        {
            output = &buff[i];
            if ((offset = output.find("pred: ")) != std::string::npos)
                return output.substr(offset); // Add 6 to drop the "pred: "
            else
            {
                if(!output.size())
                    continue;

                fprintf(stderr, "%s:%d\t%s(%lu): %s\n", FILENM, __LINE__,
                    "--TENSOR", output.size(), output.c_str());
                return ""; // Tensor probably crashed
            }
        }
    }
    dumpErrorPipe();
}

bool TensorHandler::handleController(std::string tensor)
{
    printf("%s:%d\tParsing Tensor Output:\n\t%s\n", 
        FILENM, __LINE__, tensor.c_str());
    float sx, sy;
    int ba, bb, by, bz, bl;
    // pred: 0.0 1.0 0 0 1 0 0
    if (sscanf(tensor.c_str(), "pred: %f %f %d %d %d %d %d",
        &sx, &sy, &ba, &bb, &by, &bz, &bl) < 7)
    {
        fprintf(stderr, "%s:%d\t%s\n\t%s\n", FILENM, __LINE__,
            "--ERROR:sscanf failed to parse tensor output", tensor.c_str());
        return false;
    }

    printf("%s:%d\tSending Controls to Controller\n", FILENM, __LINE__);
    return ctrl->setControls({ sx, sy, ba, bb, by, bz, bl });
}

bool TensorHandler::MakeExchange(MemoryWatcher* mem)
{
    printf("%s:%d\tMaking Exchange\n", FILENM, __LINE__);
    SendToPipe(mem->GetPlayer(ctrl->player), mem->GetPlayer(!ctrl->player));

    std::string ret = ReadFromPipe();
    if (ret.size() == 0)
        return false;

    dumpErrorPipe();

    return handleController(ret);
}

// Send false for character select and true for stage
bool TensorHandler::SelectLocation(MemoryWatcher* mem, bool charStg)
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

    //printf("%s:%d\tCursor Pos P%d: %4.3f %4.3f dist:%4.3f %4.3f\n", FILENM, __LINE__, ctrl->player ? 2 : 1, p.cursor_x, p.cursor_y, disx, disy);

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

    // Are we in a good delta?
    if (absx < 0.2 && absy < 0.2)
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
    }
    else
    {
        printf("%s:%d\tWriting Close\n", FILENM, __LINE__);
        char buff[BUFF_SIZE];
        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "-1 -1\n");
        int bytesWritten = strlen(buff);
        if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
        {
            fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
                "--ERROR:write", strerror(errno));
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

    // close the pipes
    dumpErrorPipe();
    printf("%s:%d\tShutting Down pipes\n", FILENM, __LINE__);
    close(pipeFromPy[0]);
    close(pipeToPy[1]);
    close(pipeErrorFromPy[0]);

    printf("%s:%d\tTensor Handler Destroyed\n", FILENM, __LINE__);
}
