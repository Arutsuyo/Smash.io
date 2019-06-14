#pragma once

#ifndef _HANDLER_
#define _HANDLER_

#include "Config.h"
#include "DolphinHandle.h"
#include "GenerationManager.h"
#include <vector>
#include <mutex>
#include <condition_variable>

// Signal handlers for CTRL^C
void sigint_handle(int val);
bool createSigIntAction();

bool file_exists(const char* path);
bool dir_exists(const char* path);

enum Pred_Modes
{
    LOAD_MODEL = 0,
    NEW_MODEL = 1,
    PREDICTION_ONLY = 2,
    NEW_PREDICTION = 3,
};

class Trainer
{
    std::vector<DolphinHandle*> _Dhandles;
    static std::vector<int> killpids;
public:
    static Trainer* _inst;

    static bool term;
    bool initialized;
    static Config* cfg;

    static int memoryCount;


    // ISO info
    static std::string _ssbmisoLocs[];
    static unsigned int _isoidx;

    // Directory info
    static std::string userDir;
    static std::string dolphinDefaultUser;
    
    // Tensor Info
    static VsType vs;
    static std::string PythonCommand;
    static std::string modelName;
    static int predictionType;
    static void GetVersionNumber(std::string& parsed);

    // Threading Info
    static unsigned Concurent;
    static std::mutex mut;
    static std::condition_variable cv;

    void KillDolphinHandles();
    void runTraining();
    Trainer();
    ~Trainer();
};

#endif
