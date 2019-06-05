#include "Controller.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <signal.h>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <time.h>
#define FILENM "CTRL"

char Controller::_ButtonNames[] = {
        'A',
        'B',
        'X',
        'Z',
        'L'
};

int nsleep(long miliseconds)
{
    struct timespec req, rem;

    if (miliseconds > 999)
    {
        req.tv_sec = (int)(miliseconds / 1000);                            /* Must be Non-Negative */
        req.tv_nsec = (miliseconds - ((long)req.tv_sec * 1000)) * 1000000; /* Must be in range of 0 to 999999999 */
    }
    else
    {
        req.tv_sec = 0;                         /* Must be Non-Negative */
        req.tv_nsec = miliseconds * 1000000;    /* Must be in range of 0 to 999999999 */
    }

    return nanosleep(&req, &rem);
}

void sigpipe_handler(int val)
{
    if (val != SIGPIPE)
    {
        fprintf(stderr, "%s:%d: %s\n", FILENM, __LINE__,
            "--ERROR:Wrong Signal Caught");
        return;
    }

    printf("%s:%d A pipe was closed\n", FILENM, __LINE__);
}

bool createSigPipeAction()
{
    // Create signal action
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sigpipe_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:sigaction", strerror(errno));
        return false;
    }
    return true;
}

bool Controller::sendtofifo(std::string fifocmd)
{
    printf("%s:%d Writing to FIFO\n", FILENM, __LINE__);
    unsigned int buff_sz = strlen(fifocmd.c_str());
    if (write(fifo_fd, fifocmd.c_str(), buff_sz) < buff_sz)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:write", strerror(errno));
        pipeOpen = false;
        return false;
    }
    return true;
}

std::string Controller::GetState()
{
    char buff[256];
    std::string output = std::string();

    // Main Stick
    sprintf(buff, "SET MAIN %.2f %.2f\n", ct.stick[0], ct.stick[0]);
    output += buff;

    // buttons
    for (unsigned int i = 0; i < _NUM_BUTTONS; i++)
    {
        sprintf(
            buff,
            "%s %c \n",
            ct.buttons[i] ? "PRESS" : "RELEASE",
            _ButtonNames[i]);
        output += buff;
    }

    return output;
}

bool Controller::SendState()
{
    if (!pipeOpen)
    {
        fprintf(stderr, "%s:%d: %s\n", FILENM, __LINE__,
            "--ERROR:Cannot send input, please open pipe");
        return false;
    }

    return sendtofifo(GetState());
}

void Controller::setButton(Button btn)
{
    for (int i = 0; i < _NUM_BUTTONS; i++)
        ct.buttons[i] = i == btn ? true : false;

}

std::string getFileName(const std::string& s) {

    char sep = '/';

#ifdef _WIN32
    sep = '\\';
#endif

    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return(s.substr(i + 1, s.length() - i));
    }

    return("");
}

bool Controller::PressStart()
{
    printf("%s:%d Pressing Start\n", FILENM, __LINE__);
    char buff[256];
    std::string output;

    sprintf(buff, "%s %s\n", "PRESS", "Start");
    output = buff;
    printf("%s:%d %s %s\n", FILENM, __LINE__,
        getFileName(pipePath).c_str(), output.c_str());
    if (!sendtofifo(output))
        return false;
    
    // Sleep for half a second
    nsleep(500);

    sprintf(buff, "%s %s\n", "PRESS", "Start");
    output = buff;
    printf("%s:%d %s %s\n", FILENM, __LINE__,
        getFileName(pipePath).c_str(), output.c_str());

    return sendtofifo(output);
}

void Controller::setSticks(float valX, float valY)
{
    ct.stick[0] = valX;
    ct.stick[1] = valY;
}

void Controller::setControls(Controls inCt)
{
    ct = inCt;
}

std::string Controller::GetControllerPath()
{
    return pipePath;
}

bool Controller::CreateFifo(std::string inPipePath, int pipe_count)
{
    printf("%s:%d Creating pipe\n", FILENM, __LINE__);
    // Make the pipe
    pipePath = inPipePath + std::to_string(pipe_count);
    if (mkfifo(pipePath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:mkfifo", strerror(errno));
        return false;
    }
    printf("%s:%d Pipe Created: %s\n", FILENM, __LINE__, pipePath.c_str());
    return true;
}

bool Controller::OpenController()
{
    printf("%s:%d Creating pipe signal handler\n", FILENM, __LINE__);
    if (!createSigPipeAction())
        printf("CTRL: Could not create fifo sighandler\n");

    // Blocks if no reader
    printf("%s:%d Opening pipe\n", FILENM, __LINE__);
    if ((fifo_fd = open(pipePath.c_str(), O_WRONLY)) < 0)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:open", strerror(errno));
        pipeOpen = false;
        return false;
    }
    pipeOpen = true;

    printf("%s:%d Controller ready for input\n", FILENM, __LINE__);
    return true;
}

bool Controller::IsPipeOpen()
{
    return pipeOpen;
}

Controller::Controller(bool plyr) :
    player(plyr),
    ct{ 0.5f, 0.5f, false, false, false, false, false }
{
    printf("%s:%d Controller Created\n", FILENM, __LINE__);
}

Controller::~Controller()
{
    printf("%s:%d Destroying Controller\n", FILENM, __LINE__);
    if (pipeOpen)
    {
        printf("%s:%d Closing pipe\n", FILENM, __LINE__);
        close(fifo_fd);
    }
    printf("%s:%d deleting pipe\n", FILENM, __LINE__);
    if (remove(pipePath.c_str()) != 0)
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:remove", strerror(errno));
    
    printf("%s:%d deleted pipe\n", FILENM, __LINE__);
}
