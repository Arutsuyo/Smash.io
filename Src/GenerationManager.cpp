#include "GenerationManager.h"
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>
#include <ios>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <cmath>
#define FILENM "GM"

// Used in culling
typedef std::pair<int, int> score;

int GenerationManager::childCount = -1;
int GenerationManager::childDirCount = -1;
int GenerationManager::parentCount = -1;

std::string GenerationManager::curParentDir = "";
std::string GenerationManager::curChildDir = "";
std::string GenerationManager::modelPath = "";

std::mutex GenerationManager::mut;
bool GenerationManager::initialized = false;

float GenerationManager::GetEpsilon()
{
    if(parentCount > 0)
        return pow(0.995f, parentCount);
    return 0.995f;
}

std::string GenerationManager::GetParentFile()
{
    if (!initialized)
    {
        fprintf(stderr, "%s:%d\t--ERROR: GenerationManager MUST BE INITIALIZED\n", FILENM, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::unique_lock<std::mutex> lk(mut);

    // Check mode
    if (Trainer::predictionType == Pred_Modes::NEW_MODEL
        || Trainer::predictionType == Pred_Modes::NEW_PREDICTION)
        return "";

    // We want to evenly distribute the matches throughout the generation
    std::string pName;
    if (childCount % 2)
    {
        pName = curParentDir + std::to_string((childCount / SIZE_CULL_TO) % SIZE_CULL_TO) + ".h5";
    }
    else
    {
        pName = curParentDir + std::to_string(childCount / (SIZE_CULL_TO * SIZE_CULL_TO)) + ".h5";
    }
    printf("%s:%d\tRetrieved: %s\n",
        FILENM, __LINE__, pName.c_str());
    return pName;
}

std::string GenerationManager::GetChildFile()
{
    if (!initialized)
    {
        fprintf(stderr, "%s:%d\t--ERROR: GenerationManager MUST BE INITIALIZED\n", FILENM, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::unique_lock<std::mutex> lk(mut);

    // Check mode
    if (Trainer::predictionType == Pred_Modes::PREDICTION_ONLY
        || Trainer::predictionType == Pred_Modes::NEW_PREDICTION)
        return "";

    // Parse the child file based on the model path, dirCount, and ChildCount
    std::string childFile;
    childCount++;
    childFile = curChildDir + std::to_string(childCount);
    childFile += ".h5";
    
    //Safety check
    if (childCount == SIZE_GEN)
        childCount--;

    if (file_exists(childFile.c_str()))
    {
        fprintf(stderr, "%s:%d\t--ERROR: Child file already exists, overwriting: %s\n", FILENM, __LINE__, childFile.c_str());
    }
    printf("%s:%d\tRetrieved: %s\n",
        FILENM, __LINE__, childFile.c_str());
    return childFile;
}

bool GenerationManager::GenerationReady()
{
    if (!initialized)
    {
        fprintf(stderr, "%s:%d\t--ERROR: GenerationManager MUST BE INITIALIZED\n", FILENM, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::unique_lock<std::mutex> lk(mut);

    // Check if there's enough children to cull
    printf("%s:%d\tChild Count:%s %d/%d\n",
        FILENM, __LINE__, curChildDir.c_str(), childCount+1, SIZE_GEN);
    return childCount >= SIZE_GEN-1;
}

bool GenerationManager::CullTheWeak()
{
    if (!initialized)
    {
        fprintf(stderr, "%s:%d\t--ERROR: GenerationManager MUST BE INITIALIZED\n", FILENM, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::unique_lock<std::mutex> lk(mut);

    // Special Case
    if (Trainer::predictionType > 1)
    {
        childCount = -1;

        return true;
    }

    // Warn the user we are going to cull, don't kill the program, and yes, it will be LOUD
    char buff[256];
    memset(buff, 0, 256);
    sprintf(buff, "%s:%d\tWARNING::%s IS PERFORMING A CULLING, DO NOT CLOSE PROGRAM\n", FILENM, __LINE__, FILENM);
    printf("%s", buff); printf("%s", buff); printf("%s", buff); printf("%s", buff); printf("%s", buff);
    printf("%s", buff); printf("%s", buff); printf("%s", buff); printf("%s", buff); printf("%s", buff);
    fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff);
    fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff); fprintf(stderr, "%s", buff);

    // Get the top SIZE_CULL_TO
    //                    ID   Score
    std::vector<score> top8;
    int val, i = 0;
    bool exists = true;
    while (exists)
    {
        // Read the Child Scores
        sprintf(buff, "%s%d.h5.txt", curChildDir.c_str(), i);
        std::fstream fs;
        fs.open(buff, std::fstream::in);
        if (fs.fail())
        {
            fprintf(stderr, "%s:%d --Error: Could not open score: %s\n",
                FILENM, __LINE__, buff);
            // reached the end of the scores
            break;
        }
        fs >> val;
        fs.close();

        if (top8.size() < SIZE_CULL_TO)
        {
            top8.push_back(score(i++, val));
            continue;
        }

        for (unsigned int j = 0; j < SIZE_CULL_TO; j++)
        {
            if (val > top8[j].second)
            {
                top8[j].first = i;
                top8[j].second = val;
                break;
            }
        }
        i++;
    }

    printf("%s:%d\tIdentified Top %d\n",
        FILENM, __LINE__, SIZE_CULL_TO);

    // Make Parent Dir
    parentCount++;
    curParentDir = DIR_AI_BASE;
    curParentDir += modelPath + DIR_PARENT_BASE;
    sprintf(buff, curParentDir.c_str(), parentCount);
    curParentDir = buff;
    sprintf(buff, "mkdir -p ./%s", curParentDir.c_str());
    if (system(buff) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:system", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Copy top SIZE_CULL_TO into parent
    for (int i = 0; i < SIZE_CULL_TO; i++)
    {
        // Parse the child file based on the model path, dirCount, and ChildCount
        char childName[256];
        memset(childName, 0, 256);
        sprintf(childName, "%s%d.h5", curChildDir.c_str(), top8[i].first);

        // Copy the child to the parent
        sprintf(buff, "cp %s %s%d.h5", childName, curParentDir.c_str(), i);
        if (system(buff) == -1)
        {
            fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
                "--ERROR:system", strerror(errno));
            return false;
        }
        sprintf(buff, "cp %s.txt %s%d.h5.txt", childName, curParentDir.c_str(), i);
        if (system(buff) == -1)
        {
            fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
                "--ERROR:system", strerror(errno));
            return false;
        }
    }

    // Delete the bad children
    sprintf(buff, "rm %s*", curChildDir.c_str());
    if (system(buff) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:system", strerror(errno));
        return false;
    }
    // Copy the good ones back in
    sprintf(buff, "cp %s* %s*", curParentDir.c_str(), curChildDir.c_str());
    if (system(buff) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:system", strerror(errno));
        return false;
    }

    printf("%s:%d\tCopied to new parent dir\n",
        FILENM, __LINE__);
    // Make new child dir
    childDirCount++;

    // Parse the Next Child Folder
    std::string temp = DIR_AI_BASE;
    temp += modelPath + DIR_CHILD_BASE;
    sprintf(buff, temp.c_str(), childDirCount);
    curChildDir = buff;
    sprintf(buff, "mkdir -p ./%s", curChildDir.c_str());
    if (system(buff) == -1)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:system", strerror(errno));
        exit(EXIT_FAILURE);
    }


    printf("%s:%d\tReset Child Count\n",
        FILENM, __LINE__);
    // Make sure variables are set right
    childCount = -1;

    printf("%s:%d\tContinuing . . .\n",
        FILENM, __LINE__);
    return true;
}

bool GenerationManager::Initialize(std::string model_path)
{
    std::unique_lock<std::mutex> lk(mut);
    const unsigned int bufflen = 256;
    char buff[bufflen];
    memset(buff, 0, bufflen);
    std::string childFile;
    modelPath = model_path;

    printf("%s:%d\tInitilizing GenerationManager\n",
        FILENM, __LINE__);

    // Getting Child dir Count
    do
    {
        // Increment Child Dir Count
        childDirCount++;

        // Parse the Next Child Folder
        std::string temp = DIR_AI_BASE;
        temp += modelPath + DIR_CHILD_BASE;
        sprintf(buff, temp.c_str(), childDirCount);
        curChildDir = buff;
        // Test for existence
        if (!dir_exists(curChildDir.c_str()))
        {
            // Set the dir to the previous one
            if (childDirCount != 0)
            {
                // We may not have finished training a generation
                childDirCount--;
                sprintf(buff, temp.c_str(), childDirCount);
                curChildDir = buff;
            }
            break;
        }
    } while (true);

    if (childDirCount == 0)
    {
        printf("%s:%d\tFreash AI in: %s\n",
            FILENM, __LINE__, curChildDir.c_str());

        // Make the new folder
        sprintf(buff, "mkdir -p ./%s", curChildDir.c_str());
        if (system(buff) == -1)
        {
            fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
                "--ERROR:system", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    printf("%s:%d\tReading children in: %s\n",
        FILENM, __LINE__, curChildDir.c_str());

    // Count the current number of children. This allows us to persist
    do
    {
        childCount++;
        childFile = curChildDir + std::to_string(childCount);
        childFile += ".h5";

        printf("%s:%d\tChecking Child: %s\n",
            FILENM, __LINE__, childFile.c_str());
    } while (file_exists(childFile.c_str()));

    printf("%s:%d\tCurrent Child Count: %d\n",
        FILENM, __LINE__, childCount);

    // Parse the Parent Folder
    parentCount = childDirCount;
    curParentDir = DIR_AI_BASE;
    curParentDir += modelPath + DIR_PARENT_BASE;
    sprintf(buff, curParentDir.c_str(), parentCount);
    curParentDir = buff;
    if (childCount == SIZE_CULL_TO && dir_exists(buff))
    {
        // Count the number of parents
        int pCount = -1;
        do
        {
            pCount++;
            childFile = curParentDir + std::to_string(pCount);
            childFile += ".h5";

        } while (file_exists(childFile.c_str()));

        if (pCount != SIZE_CULL_TO)
        {
            fprintf(stderr, "%s:%d\t%s Not enough Parent Files, Copying the correct Children\n", FILENM, __LINE__, "--ERROR:GenerationManager");
            sprintf(buff, "cp %s* %s*", curChildDir.c_str(), curParentDir.c_str());
            if (system(buff) == -1)
            {
                fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
                    "--ERROR:system", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        printf("%s:%d\tFinished Training, ready for next gen!\n",
            FILENM, __LINE__);

        // Increment Child Dir Count
        childDirCount++;

        // Parse the Next Child Folder
        std::string temp = DIR_AI_BASE;
        temp += modelPath + DIR_CHILD_BASE;
        sprintf(buff, temp.c_str(), childDirCount);
        curChildDir = buff;

        // Make the new folder
        sprintf(buff, "mkdir -p ./%s", curChildDir.c_str());
        if (system(buff) == -1)
        {
            fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
                "--ERROR:system", strerror(errno));
            exit(EXIT_FAILURE);
        }
        childCount = -1;
    }
    else if (childCount < SIZE_GEN)
    {
        childCount--;
        printf("%s:%d\tDid not finish training, Continuing!\n",
            FILENM, __LINE__);
        // Set the parent dir
        parentCount--;
        if (parentCount >= 0)
        {
            curParentDir = DIR_AI_BASE;
            curParentDir += modelPath + DIR_PARENT_BASE;
            sprintf(buff, curParentDir.c_str(), parentCount);
            curParentDir = buff;
        }
        else
            Trainer::predictionType += 1;

        printf("%s:%d\tRunning Generation: %d\n",
            FILENM, __LINE__, childDirCount);

    }
    else
    {
        printf("%s:%d\tWE MUST CULL Generation: %s\n",
            FILENM, __LINE__, curChildDir.c_str());
        parentCount--;
    }

    srand(time(NULL));
    initialized = true;
    return initialized;
}
