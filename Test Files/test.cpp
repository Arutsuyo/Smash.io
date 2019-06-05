#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
//#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>

// Random
#include <cstdlib>
// Seed
#include <ctime>
#define BUFF_SIZE 2048

pid_t pid = 0;
int pipeFromPy[2]{-1,-1};
int pipeToPy[2]{ -1,-1 };;

void sigint_handler(int val)
{
    if (val != SIGINT)
    {
        fprintf(stderr, "--ERROR:Wrong Signal Caught");
        return;
    }

    int status;
    printf("SIGINT - Killing Children\n");
    kill(pid, 9);
    waitpid(pid, &status, 0);
    printf("Child dead(%d)\n", status);
}

void sigpipe_handler(int val)
{
    if (val != SIGPIPE)
    {
        fprintf(stderr, "--ERROR:Wrong Signal Caught");
        return;
    }

    int status;
    printf("SIGPIPE - Child closed pipe\n");
    waitpid(pid, &status, 0);
    printf("Child dead(%d)\n", status);
}

bool createSigINTAction()
{
    // Create signal action
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "--ERROR:sigaction(SIGINT");
        return false;
    }
    sa.sa_flags = 0;
    sa.sa_handler = sigpipe_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        fprintf(stderr, "--ERROR:sigaction(SIGPIPE");
        return false;
    }
    return true;
}

// Returns rand in the interval (inclusive). Default 0-1
// https://stackoverflow.com/a/686373/2939859
float getRand(int lo = 0, int high = 1)
{
    return lo + static_cast<float>(rand()) 
        / static_cast <float> (RAND_MAX /(high - lo));
}

std::string ReadFromPipe()
{
    // Flushing read
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    std::string output = "";
    int ret = 0, offset = 0;
    do {
        if ((ret = read(pipeFromPy[0], buff, BUFF_SIZE)) == -1)
        {
            fprintf(stderr, "ERROR: pipe read failed\n");
            sigint_handler(SIGINT);
            exit(EXIT_FAILURE);
        }

        // Print what was read
        //printf("Read%d: %s\n", ret, buff);
        
        // step through the gunk until we find the prediction
        for (int i = 0; i < ret; i += output.size()+1)
        {
            output = &buff[i];
            if ((offset = output.find("pred: ")) != std::string::npos)
                return output.substr(offset);
        }

        output += buff;
    } while (true);
}

int main(int argc, char** argv)
{
    char msg[BUFF_SIZE];
    memset(msg, 0, BUFF_SIZE);

    createSigINTAction();
    // Seed Rand
    srand(static_cast<unsigned>(time(0)));

    pipe(pipeFromPy);
    pipe(pipeToPy);
    pid = fork();
    if (pid == 0)
    {
        printf("Child: Closing unused pipe ends\n");
        close(pipeToPy[1]);
        close(pipeFromPy[0]);

        printf("Child: Dupping pipes and launching trainer\n");
        // Child
        dup2(pipeToPy[0], STDIN_FILENO);
        dup2(pipeFromPy[1], STDOUT_FILENO);
        dup2(pipeFromPy[1], STDERR_FILENO);

        //ask kernel to deliver SIGTERM in case the parent dies
       // prctl(PR_SET_PDEATHSIG, SIGTERM);

        //replace tee with your process
#if 1
        execlp("python.exe", "python.exe", "trainer.py", NULL);
#else
        execlp("python3", "python3", "test.py", NULL);
#endif
        // Nothing below this line should be executed by child process. If so, 
        // it means that the execlp function wasn't successful, so lets exit:
        fprintf(stderr, "ERROR-EXECLP: WE GOT FUCKED\n");
        exit(1);
    }
    // The code below will be executed only by parent. You can write and read
    // from the child using pipe fd descriptors, and you can send signals to 
    // the process using its pid by kill() function. If the child process will
    // exit unexpectedly, the parent process will obtain SIGCHLD signal that
    // can be handled (e.g. you can respawn the child process).

    //close unused pipe ends
    close(pipeToPy[0]);
    close(pipeFromPy[1]);

    // Init the trainer
    printf("--Writing 0 for silencing output--\n");
    sprintf(msg, "0");
    write(pipeToPy[1], msg, strlen(msg));

    printf("--Writing 0 for new model--\n");
    sprintf(msg, "0");
    write(pipeToPy[1], msg, strlen(msg));

    // Player infos
    unsigned int p1hp, p2hp;
    int p1dir, p2dir;
    float p1pos_x, p2pos_x,
          p1pos_y, p2pos_y;

    // Send 10 spoof things
    int loops = 5;
    while (loops--)
    {
        // Get random values
        p1hp = getRand(0, 400);
        p2hp = getRand(0, 400);
        p1dir = getRand() >= 0.5f ? 1 : -1;
        p2dir = getRand() >= 0.5f ? 1 : -1;
        p1pos_x = getRand(0, 100);
        p2pos_x = getRand(0, 100);
        p1pos_y = getRand(0, 100);
        p2pos_y = getRand(0, 100);
        
        // Insert rands into buffers
        printf("%d: sending fake values:\n"
            "\t%u %d %f %f\n\t%u %d %f %f\n", loops,
            p1hp, p1dir, p1pos_x, p1pos_y,
            p2hp, p2dir, p2pos_x, p2pos_y);
        sprintf(msg, "%u %d %f %f %u %d %f %f\n",
            p1hp, p1dir, p1pos_x, p1pos_y,
            p2hp, p2dir, p2pos_x, p2pos_y);

        // Write to pipe
        write(pipeToPy[1], msg, strlen(msg));

        // Read back the prediction
        printf("%s\n", ReadFromPipe().c_str());
    }

    // Close down
    printf("Sending python: -1 -1\n");
    sprintf(msg, "-1 -1\n");
    // Write to pipe
    write(pipeToPy[1], msg, strlen(msg));
    // Read back close
    std::string readbuf;
    do 
    {
        readbuf = ReadFromPipe();
    } while (readbuf.find("-1 -1") == std::string::npos);

    printf("Waiting for python to exit\n");
    int status;
    waitpid(pid, &status, 0);
    printf("Child dead(%d)\n", status);
}