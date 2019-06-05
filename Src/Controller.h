#pragma once
#ifndef _CONTROLLER_
#define _CONTROLLER_

#include <string>
#include <stdio.h>
#include "Types.h"

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

    std::string pipePath;
    bool pipeOpen = true;
    int fifo_fd;
    Controls ct;

    bool sendtofifo(char fifocmd[], int limit);

public:
    // false:Player 1 True:Player2
    bool player;

    // Call before opening, will save path
    bool CreateFifo(std::string inPipePath, int pipe_count);
    bool OpenController();

    bool IsPipeOpen();
    std::string GetControllerPath();

    // Main function to send controls to FIFO
    bool setControls(Controls inCt);
    bool PressStart();

    Controller(bool plyr);
    ~Controller();

};


#endif