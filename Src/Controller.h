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

    bool sendtofifo(std::string fifocmd);

public:
    // false:Player 1 True:Player2
    bool player;

    bool CreateFifo(std::string inPipePath, int pipe_count);
    std::string GetControllerPath();
    bool OpenController();

    // This should only be explicitly called for debug use, 
    // otherwise it's use internally to send the state
    std::string GetState();

    // Primary call the AI will use to send the state to Dolphin.
    bool SendState();

    // Sets only the designated button to true, 
    // all other buttons will be set to false
    void setButton(Button btn = Button::None);
    
    /* setStick
     * float valX/Y:
     *  Must be between 0-1. 0.5 is default resting position
     */
    void setSticks(float valX = 0.5f, float valY = 0.5f);

    void setControls(Controls inCt);

    bool IsPipeOpen();
    bool PressStart();

    Controller(bool plyr);
    ~Controller();

};


#endif