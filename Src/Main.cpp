// Main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Trainer.h"

// Included for PW
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#define FILENM "Main"

void ParseArgs(int argc, char* argv[])
{
    printf("%s:%d Parsing Arguments\n", FILENM, __LINE__);
    for (int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        unsigned long size = 0;
        
        // Number of threads to spawn
        if ((size = arg.find("-t:")) != std::string::npos)
        {
            // is it one argument or 2?
            if (arg.size() > size)
            {
                Trainer::Concurent =
                    std::stoi(arg.substr(size));
                printf("%s:%d Concurrent Threads: %d\n", FILENM, __LINE__,
                    Trainer::Concurent);
            }
            else
            {
                i++; 
                Trainer::Concurent =
                    std::stoi(argv[i]);
                printf("%s:%d Concurrent Threads: %d\n", FILENM, __LINE__,
                    Trainer::Concurent);
            }
            continue;
        }

        //

    }
}

int main(int argc, char *argv[])
{
    printf("%s:%d Initializing statics\n", FILENM, __LINE__);
    // Init the static user dir
    struct passwd* pw = getpwuid(getuid());
    Trainer::userDir = pw->pw_dir;
    Trainer::dolphinDefaultUser = Trainer::userDir + "/.local/share/dolphin-emu/";
    Trainer::Concurent =
        std::thread::hardware_concurrency() / 3;
    Trainer::term = false;

    ParseArgs(argc, argv);

    printf("%s:%d Creating Trainer\n", FILENM, __LINE__);
    Trainer trainer(VsType::Human);
    if (!trainer.initialized)
        exit(EXIT_FAILURE);

    printf("%s:%d Running Training Loop\n", FILENM, __LINE__);
    trainer.runTraining();

    printf("%s:%d Closing Everything\n", FILENM, __LINE__);
    trainer.~Trainer();

    printf("%s:%d Shutting Down\n", FILENM, __LINE__);
    exit(EXIT_SUCCESS);
}
