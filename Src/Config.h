#pragma once
#ifndef _CONFIG_
#define _CONFIG_

#include <string>
#include "Types.h"

class Config
{
    bool initialized;
    bool _dual_core;
    bool _gfx;
    bool _fullscreen;

    VsType _vs;

public:
    std::string getConfig();
    std::string getPlayerPipeConfig(int player);
    std::string getAIPipeConfig(int player, int pipe_count, std::string id);

    // MemoryScanner Functions
    std::string getLocations();
    int getMemlocationLines();

    bool IsInitialized();
    Config(VsType vType = VsType::Self);
    ~Config();
};


#endif