#include "TensorHandler.h"
#include "Trainer.h"
#include <sys/types.h> /* pid_t */
#include <cstring>
#include <sys/wait.h>
#include <stdio.h>

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

float TensorHandler::finalDest[2] = { -16.4, 18.87 };
float TensorHandler::cptFalcon[2] = { 17.4f, 17.0f };

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
    if (pipe(pipeErrorFromPy) == -1)
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
}

void TensorHandler::dumpErrorPipe()
{
    printf("%s:%d\tDumping Error from Pipe\n", FILENM, __LINE__);
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    std::string output = "";
    int ret = 0, offset = 0;
    while (ret >= BUFF_SIZE) // Read might be capped
    {
        // Get the current pipe buffer
        if ((ret = read(pipeFromPy[0], buff, BUFF_SIZE)) == -1)
        {
            fprintf(stderr, "ERROR: pipe read failed\n");
            exit(EXIT_FAILURE);
        }

        // Print what was read
        printf("PyError%d: %s\n", ret, buff);
    }
}

void TensorHandler::SendToPipe(Player ai, Player enemy)
{
    printf("%s:%d\tSending Current Data\n", FILENM, __LINE__);
    // Send input
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    sprintf(buff, "%u %d %f %f %u %d %f %f\n",
        ai.health, ai.dir, ai.pos_x, ai.pos_y,
        enemy.health, enemy.dir, enemy.pos_x, enemy.pos_y);
    int bytesWritten = strlen(buff);
    if (write(pipeToPy[1], buff, bytesWritten) != bytesWritten)
    {
        fprintf(stderr, "%s:%d\t%s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
    }

    printf("%s:%d\tSent: %s\n", FILENM, __LINE__, buff);
}

// Scrub through the pipe until we reach the identifier, return that
std::string TensorHandler::ReadFromPipe()
{
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    std::string output = "";
    int ret = 0, offset = 0;
    while (true)
    {
        // Get the current pipe buffer
        if ((ret = read(pipeFromPy[0], buff, BUFF_SIZE)) == -1)
        {
            fprintf(stderr, "ERROR: pipe read failed\n");
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
        }

        //We didn't find the droids we're looking for
        // T.T
    }
}

bool TensorHandler::handleController(std::string tensor)
{
    printf("%s:%d\tParsing Tensor Output\n", FILENM, __LINE__);
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

    return handleController(ret);
}

bool TensorHandler::SelectCharacter(MemoryWatcher* mem)
{
    Player p = mem->GetPlayer(ctrl->player);
    float sx, sy;
    int ba = 0, bb = 0, by = 0, bz = 0, bl = 0;

    // Get distance
    float disx = cptFalcon[0] - p.cursor_x;
    float disy = cptFalcon[1] - p.cursor_y;

    // Get Abs
    float absDis = disx < 0 ? -disx : disx;
    // Within Error?
    if (absDis < 0.1)
        sx = 0.5f;
    else
        sx = disx > 0 ? 0.75f : 0.25;

    // Get Abs
    absDis = disy < 0 ? -disy : disy;
    // Within Error?
    if (absDis < 0.1)
        sy = 0.5f;
    else
        sy = disy > 0 ? 0.75f : 0.25;

    if (sx == 0.5f && sy == 0.5f)
        ba = 1;

    printf("%s:%d\tSending Controls to Controller\n", FILENM, __LINE__);
    ctrl->setControls({ sx, sy, ba, bb, by, bz, bl });

    return ba;
}

bool TensorHandler::SelectStage(MemoryWatcher* mem)
{
    Player p = mem->GetPlayer(ctrl->player);
    float sx, sy;
    int ba = 0, bb = 0, by = 0, bz = 0, bl = 0;

    // Get distance
    float disx = finalDest[0] - p.cursor_x;
    float disy = finalDest[1] - p.cursor_y;

    // Get Abs
    float absDis = disx < 0 ? -disx : disx;
    // Within Error?
    if (absDis < 0.1)
        sx = 0.5f;
    else
        sx = disx > 0 ? 0.75f : 0.25;

    // Get Abs
    absDis = disy < 0 ? -disy : disy;
    // Within Error?
    if (absDis < 0.1)
        sy = 0.5f;
    else
        sy = disy > 0 ? 0.75f : 0.25;

    if (sx == 0.5f && sy == 0.5f)
        ba = 1;

    printf("%s:%d\tSending Controls to Controller\n", FILENM, __LINE__);
    ctrl->setControls({ sx, sy, ba, bb, by, bz, bl });

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

    // close the pipes
    printf("%s:%d\tShutting Down pipes\n", FILENM, __LINE__);
    close(pipeFromPy[0]);
    close(pipeToPy[1]);
    close(pipeErrorFromPy[0]);

    printf("%s:%d\tTensor Handler Destroyed\n", FILENM, __LINE__);
}
