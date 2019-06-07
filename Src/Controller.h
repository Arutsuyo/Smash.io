#pragma once
#ifndef _CONTROLLER_
#define _CONTROLLER_

#include <string>
#include <stdio.h>
#include "Types.h"
#include <chrono>
#define FPS 60
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TPoint;

int nsleep(long miliseconds);

struct Controls
{
    // Piped input sticks go from 0 to 1. 0.5 is centered
    float stick[2];
    bool buttons[_NUM_BUTTONS];
};

class Controller
{
    // The order must match enum Button
    static char _ButtonNames[];

    // Pipe Information
    std::string pipePath;
    bool pipeOpen = true;
    int fifo_fd;

    // Control Information
    // pipe delay in seconds, used for chrono calcuolations
    double pipeDelay;
    Controls ct;

    bool printfifo = true;

    bool sendtofifo(char fifocmd[], int limit);

public:
    // Timing info, used to not flood the pipe. We should be waiting at least a 
    // frame before sending more input to allow dolphin to consume the pipe
    TPoint lastSent;

    // false:Player 1 True:Player2
    bool player;

    // Call before opening, will save path
    bool CreateFifo(std::string inPipePath, int pipe_count);
    bool OpenController();

    bool IsPipeOpen();
    std::string GetControllerPath();

    // Main function to send controls to FIFO. This function will not send a new 
    // input if the timespan between pipe writes is too small. 
    // Returns false if it failed to send. if false, check if the pipe is open, 
    // otherwise the input was called before the delay elapsed.
    bool setControls(Controls inCt);

    // This function will pause long enough for dolphin to consume the pipe. 
    // Do not assume this function can be spammed
    bool ButtonPressRelease(std::string btn);

    Controller(bool plyr, int frameDelay = 1);
    ~Controller();

};


#endif