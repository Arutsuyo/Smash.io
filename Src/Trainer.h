#pragma once

#ifndef _HANDLER_
#define _HANDLER_

#include "Config.h"
#include "DolphinHandle.h"
#include <vector>
#include <mutex>
#include <condition_variable>

// Signal handlers for CTRL^C
void sigint_handle(int val);
bool createSigIntAction();

class Trainer
{
    VsType _vs = VsType::Self;

    std::vector<DolphinHandle*> _Dhandles;
    static std::vector<int> killpids;

public:
    static bool term;
    bool initialized;
    static Config* cfg;

    // ISO info
    static std::string _ssbmisoLocs[];
    static int _isoidx;

    // Directory info
    static std::string userDir;
    static std::string dolphinDefaultUser;

    // Threading Info
    static unsigned concurentThreadsSupported;
    
    static std::mutex mut;
    static std::condition_variable cv;

    static void AddToKillList(int pid);
    static void KillAllpids();

    void runTraining();
    Trainer(VsType vs = VsType::Self);
    ~Trainer();
};

#endif