// Main.cpp : This file contains the 'main' function. 
// Program execution begins and ends there.
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
    bool getVer = false;
    printf("%s:%d Parsing Arguments\n", FILENM, __LINE__);
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        unsigned long size = 0;

        // Help
        if (arg.find("-h") != std::string::npos)
        {
            printf(
// Indented to replicate how it will show on the console
"%s:%d Usage: %s <Argument...Array>\n"
"\t-vs <S(elf)/H(human)> Specifies Instance Type. Default is Self Training.\n"
"\t\tIf Human is selected, it will only spawn 1 instance.\n"
//"\t-gfx <0|1> Turn rendering of dolphin on or off.\n"
//"\t\tTensorflow uses the GPU to do it's calculations, so turning off the\n"
//"\t\trendering thread of Dolphin can be a good improvement during training.\n"
"\t-t <num_threads> : Specifies the number of dolphin instances\n"
"\t\tNormal Load: consists of Main + 5(4 if vs human) active threads\n"
"\t\tOnly increase this if your system can handle it. \n"
"\t-m <Filename>\n"
"\t\t<Filename>: Specify a Model to load into Tensorflow. \n\t\t\tFile version and .h5 will be added in %s\n"
"\t-pr Predict only mode. This will load the latest set of parent AI's\n"
"\t-u </directory> Specify a custom directory to fill with instance folders\n"
"\t-du </directory> Specify the Default dolphin-emu dir to copy as a template\n"
"\t-p <python_alias> Specifies command to use then launching tensorflow/keras\n"
"\n"
"Human Controls (Key:GCInput): V:A C:B X:X Z:Z ENTER:Start SPACE:L Arrows:Stick\n"
"\n"
                , FILENM, __LINE__, argv[0], argv[0]);
            exit(EXIT_SUCCESS);
        }

        // Vs Type
        if ((size = arg.find("-vs:")) != std::string::npos
            || arg.find("-vs") != std::string::npos)
        {
            std::string parsed;
            // is it one argument or 2?
            if (arg.size() > size)
            {
                parsed = arg.substr(size);
            }
            else
            {
                i++;
                if(argc == i)
                {
                    fprintf(stderr, "%s:%d VS Override failed to parse: %s\n", FILENM, __LINE__, "Not enough arguments");
                    exit(EXIT_FAILURE);
                }
                parsed = argv[i];
            }

            if (((parsed.find("S") != std::string::npos
                || parsed.find("s") != std::string::npos)
                && parsed.size() == 1)
                ||
                ((parsed.find("Self") != std::string::npos
                    || parsed.find("self") != std::string::npos)
                    && parsed.size() == 4))
            {
                Trainer::vs = VsType::Self;
                parsed = "Self";
            }
            else if (((parsed.find("H") != std::string::npos
                || parsed.find("h") != std::string::npos)
                && parsed.size() == 1)
                ||
                ((parsed.find("Human") != std::string::npos
                    || parsed.find("human") != std::string::npos)
                    && parsed.size() == 5))
            {
                Trainer::vs = VsType::Human;
                parsed = "Human";
            }
            else
            {
                fprintf(stderr, "%s:%d VS Override failed to parse: %s\n", FILENM, __LINE__, parsed.c_str());
                exit(EXIT_FAILURE);
            }

            printf("%s:%d --Override: Vs Type: %s\n", FILENM, __LINE__,
                parsed.c_str());
            continue;
        }

        // Model File
        if ((size = arg.find("-m:")) != std::string::npos
            || arg.find("-m") != std::string::npos)
        {
            std::string parsed;
            // is it one argument or 2?
            if (arg.size() > size)
            {
                parsed = arg.substr(size);
            }
            else
            {
                i++;
                if (argc == i)
                {
                    fprintf(stderr, "%s:%d Model override failed to parse: %s\n", FILENM, __LINE__, "Not enough arguments");
                    exit(EXIT_FAILURE);
                }
                parsed = argv[i];
            }

            Trainer::GetVersionNumber(parsed);
            getVer = true;

            if (!file_exists(parsed.c_str()))
            {
                fprintf(stderr, "%s:%d Model Override failed to parse: %s does not exist.\n", FILENM, __LINE__, parsed.c_str());
                exit(EXIT_FAILURE);
            }

            continue;
        }

        // Prediction Mode
        if ((size = arg.find("-pr:")) != std::string::npos
            || arg.find("-pr") != std::string::npos)
        {
            printf("%s:%d --Override: Launching Tensor in Predict mode\n", FILENM, __LINE__);
            Trainer::predictionType += 2;
            continue;
        }
        
        // Number of threads to spawn (Don't go over CPU_CORES / 2) 
        // While Dolphin says it's dual core, it loads a small amount onto a third 
        // core, which is probably Dolphins window.
        if ((size = arg.find("-t:")) != std::string::npos
            || arg.find("-t") != std::string::npos)
        {
            // is it one argument or 2?
            if (arg.size() > size)
            {
                Trainer::Concurent = std::stoi(arg.substr(size));
                printf("%s:%d --Override: Concurrent Instances: %d\n", FILENM, __LINE__, Trainer::Concurent);
            }
            else
            {
                i++;
                if (argc == i)
                {
                    fprintf(stderr, "%s:%d VS Override failed to parse: %s\n", FILENM, __LINE__, "Not enough arguments");
                    exit(EXIT_FAILURE);
                }
                Trainer::Concurent = std::stoi(argv[i]);
                printf("%s:%d --WARNING::Override: Concurrent Instances: %d\n"
                    "\tTHIS CAN BE PROBLEMATIC FOR SYSTEMS WITH LOW CORE COUNT\n", FILENM, __LINE__, Trainer::Concurent);
            }
            continue;
        }

        // Override User Dir
        if ((size = arg.find("-u:")) != std::string::npos
            || arg.find("-u") != std::string::npos)
        {
            // is it one argument or 2?
            if (arg.size() > size)
            {
                Trainer::userDir = arg.substr(size);
                printf("%s:%d --Override: Custom Dir: %s#\n", FILENM, __LINE__,
                    Trainer::userDir.c_str());
            }
            else
            {
                i++;
                if (argc == i)
                {
                    fprintf(stderr, "%s:%d VS Override failed to parse: %s\n", FILENM, __LINE__, "Not enough arguments");
                    exit(EXIT_FAILURE);
                }
                Trainer::userDir = argv[i];
                printf("%s:%d --Override: Custom Dir: %s#\n", FILENM, __LINE__,
                    Trainer::userDir.c_str());
            }
            continue;
        }

        // Override User Dir
        if ((size = arg.find("-du:")) != std::string::npos
            || arg.find("-du") != std::string::npos)
        {
            // is it one argument or 2?
            if (arg.size() > size)
            {
                Trainer::dolphinDefaultUser = arg.substr(size);
                printf("%s:%d --Override: Custom Default Dir: %s#\n", FILENM, __LINE__,
                    Trainer::dolphinDefaultUser.c_str());
            }
            else
            {
                i++;
                if (argc == i)
                {
                    fprintf(stderr, "%s:%d VS Override failed to parse: %s\n", FILENM, __LINE__, "Not enough arguments");
                    exit(EXIT_FAILURE);
                }
                Trainer::dolphinDefaultUser = argv[i];
                printf("%s:%d --Override: Custom Default Dir: %s#\n", FILENM, __LINE__,
                    Trainer::dolphinDefaultUser.c_str());
            }
            continue;
        }

        // Override Python Command
        if ((size = arg.find("-p:")) != std::string::npos
            || arg.find("-p") != std::string::npos)
        {
            // is it one argument or 2?
            if (arg.size() > size)
            {
                Trainer::PythonCommand = arg.substr(size);
                printf("%s:%d --Override: Custom Python: %s#\n", FILENM, __LINE__,
                    Trainer::PythonCommand.c_str());
            }
            else
            {
                i++;
                Trainer::PythonCommand = argv[i];
                printf("%s:%d --Override: Custom Python: %s#\n", FILENM, __LINE__,
                    Trainer::PythonCommand.c_str());
            }
            continue;
        }
    }

    // Update and check for conflicts
    if(!getVer)
        Trainer::GetVersionNumber(Trainer::modelName);
}

int main(int argc, char* argv[])
{

    printf("%s:%d Initializing statics\n", FILENM, __LINE__);
    // Init the static user dir
    struct passwd* pw = getpwuid(getuid());
    Trainer::userDir = pw->pw_dir;
    Trainer::dolphinDefaultUser = Trainer::userDir + "/.local/share/dolphin-emu";
    Trainer::term = false;

    ParseArgs(argc, argv);

    if (Trainer::dolphinDefaultUser.substr(
        Trainer::dolphinDefaultUser.size() - 1).c_str()[0]
        == '/')
    {
        if (!dir_exists(Trainer::dolphinDefaultUser.substr( 0,
            Trainer::dolphinDefaultUser.size() - 1).c_str()))
        {
            fprintf(stderr, "%s:%d %s%s\n", FILENM, __LINE__,
                "--ERROR:Please run dolphin-emu to generate the Default user file.\n"
                "You may also use -du <dir> to specify the default user dir.\n"
                "Tested: ", Trainer::dolphinDefaultUser.c_str());
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!dir_exists(Trainer::dolphinDefaultUser.c_str()))
        {
            fprintf(stderr, "%s:%d %s%s\n", FILENM, __LINE__,
                "--ERROR:Please run dolphin-emu to generate the Default user file.\n"
                "You may also use -du <dir> to specify the default user dir.\n"
                "Tested: ", Trainer::dolphinDefaultUser.c_str());
            exit(EXIT_FAILURE);
        }
        Trainer::dolphinDefaultUser += '/';
    }

    printf("%s:%d Creating Trainer\n", FILENM, __LINE__);
    Trainer trainer;
    if (!trainer.initialized)
        exit(EXIT_FAILURE);

    printf("%s:%d Running Training Loop\n", FILENM, __LINE__);
    trainer.runTraining();

    printf("%s:%d Closing Everything\n", FILENM, __LINE__);
    trainer.~Trainer();

    printf("%s:%d Shutting Down\n", FILENM, __LINE__);
    exit(EXIT_SUCCESS);
}
