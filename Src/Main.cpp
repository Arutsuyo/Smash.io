// Main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Trainer.h"

// Included for PW
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#define FILENM "Main"

int main()
{
    // Parse Test
    /*std::string val = "3f800000";
    val
    unsigned int val_int = std::stoul(val.c_str(), nullptr, 16);
    printf("Test val:%d\n", val_int);
    exit(EXIT_SUCCESS);*/

    printf("%s:%d Initializing statics\n", FILENM, __LINE__);
    // Init the static user dir
    struct passwd* pw = getpwuid(getuid());
    Trainer::userDir = pw->pw_dir;
    Trainer::dolphinDefaultUser = Trainer::userDir + "/.local/share/dolphin-emu/";
    Trainer::concurentThreadsSupported =
        std::thread::hardware_concurrency() / 3;
    Trainer::term = false;

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
