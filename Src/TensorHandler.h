#pragma once
#include "MemoryWatcher.h"
#include "Controller.h"

bool exists_test(const std::string& name);

class TensorHandler
{
    static float finalDest[2];
    static float battlefield[2];
    static float cptFalcon[2];

    int pid;

    // Pipes: 0 is read, 1 is write
    int pipeToPy[2];
    int pipeFromPy[2];
    int pipeErrorFromPy[2];

    Controller* ctrl;

    void dumpErrorPipe();

    // Pipe interactions
    void SendToPipe(Player ai, Player enemy);
    std::string ReadFromPipe();
    bool handleController(std::string tensor);

public:
    // Must be called before exchanges can be made
    bool CreatePipes(Controller* ai);

    // Returns the output from the model in the following format: 
    // 
    // If there's an error, returns ""
    bool MakeExchange(MemoryWatcher*mem);

    bool SelectLocation(MemoryWatcher* mem, bool charStg);

    TensorHandler();
    ~TensorHandler();
};