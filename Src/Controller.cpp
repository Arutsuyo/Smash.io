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
#define BUFF_SIZE 1024



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

bool Controller::IsPipeOpen()
{
    return pipeOpen;
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

bool Controller::sendtofifo(char fifocmd[], int limit)
{
    int ret = 0, offset = 0;
    while (offset < limit)
    {
        int towrite = strlen(fifocmd + offset);
#if CTRL_OUTPUT
        printf("%s:%d To FIFO: %s", FILENM, __LINE__, fifocmd + offset);
#endif
        if (towrite + offset > limit)
            fprintf(stderr, "%s:%d Cannot make the next write: total:%d limit %d\n", FILENM, __LINE__, towrite + offset, limit);

        if ((ret = write(fifo_fd, fifocmd + offset, towrite)) == -1)
        {
            fprintf(stderr, "%s:%d: %s: %s\n\t%s\n", FILENM, __LINE__,
                "--ERROR:write", strerror(errno), pipePath.c_str());
            pipeOpen = false;
            return false;
        }
        offset += ret + 1;
    }
    return true;
}

bool Controller::setControls(Controls inCt)
{
    char buff[BUFF_SIZE];
    memset(buff, 0, BUFF_SIZE);
    int offset = 0, ret = 0;

    std::chrono::duration<double> elapsed =
        std::chrono::high_resolution_clock::now()
        - lastSent;
    if (elapsed.count() < pipeDelay)
    {
#if CTRL_OUTPUT
        printf("%s:%d\tDelay hasn't elapsed\n",
            FILENM, __LINE__);
#endif
        return false;
    }

    // Main Stick
    float disx = ct.stick[0] - inCt.stick[0],
        disy = ct.stick[1] - inCt.stick[1];
    // Get Abs
    float absDisx = disx < 0 ? -disx : disx,
        absDisy = disy < 0 ? -disy : disy;
    if (absDisx > 0.01 || absDisy > 0.01)
    {
        ret = sprintf(buff, "SET MAIN %4.4f %4.4f\n", 
            inCt.stick[0], inCt.stick[1]);
        ct.stick[0] = inCt.stick[0];
        ct.stick[1] = inCt.stick[1];
        offset += ret + 1;
    }
    // buttons
    for (unsigned int i = 0; i < _NUM_BUTTONS; i++)
    {
        // Don't send repeated state
        if (ct.buttons[i] == inCt.buttons[i])
            continue;

        ret = sprintf(buff + offset, "%s %c\n",
            inCt.buttons[i] ? "PRESS" : "RELEASE",
            _ButtonNames[i]);
        ct.buttons[i] = inCt.buttons[i];
        offset += ret + 1;
    }

    if (!sendtofifo(buff, offset))
        pipeOpen = false;

    return pipeOpen;
}

bool Controller::ButtonPressRelease(std::string btn)
{
    char buff[BUFF_SIZE];
    int ret = 0;
    bool ogfifo = printfifo;
    printfifo = false;
    ret = sprintf(buff, "%s %s\n", "PRESS", btn.c_str());
    if (!sendtofifo(buff, ret))
        return false;

    // delay. pipeDelay needs to be converted to ms from seconds
    nsleep(pipeDelay * 1000);

    ret = sprintf(buff, "%s %s\n", "RELEASE", btn.c_str());
    if (!sendtofifo(buff, ret))
        return false;
    
    printfifo = ogfifo;
    return true;
}

Controller::Controller(bool plyr, int frameDelay) :
    ct{ 0.5f, 0.5f, false, false, false, false, false },
    player(plyr)
{
    // calculate frametime (ms)
    double frametime = FPS / 1000.0;
    // convert to seconds
    frametime /= 1000;
    pipeDelay = frametime * frameDelay;

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
