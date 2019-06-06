#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <stdio.h>
#include "util.h"
/* Dolphin Memory handling
 * Author: Narayana Emery
 * Description: I made this file to help myself understand how dolphin 
 *  parses it's locations.txt input. Here's an example:
 *  (Actually lines recieved are marked with #:)
 *  1:0045310E <- input line with hard address (HA)
 *  2:6f <- value
 *  
 *  3:00453130 140 <- input line with HA (returns value) 
 *      and adds the value from the HA to the ptr, then returns THAT value
 *  4:2f <- value
 *  
 *  This v line is special, it takes the HA, and follows the first ptr, then the second, etc. it "Chases Pointers"
 *  5:00453FC0 140 14C 19BC 19C8 19EC 20CC 2174 23a0 70 8F4 E0 E4 EC F0
 *  6:469 <- value 
 *  
 *  To see how it works, run this file!
 **/


// Address as stored in the file -> list of offsets to follow
std::map<std::string, std::vector<u32>> m_addresses;
// Address as stored in the file -> current value
std::map<std::string, u32> m_values;

void ParseLine(const std::string& line)
{
    // Location to save a single value
    m_values[line] = 0;
    // The pointer values to "chase" (aka add together)
    m_addresses[line] = std::vector<u32>();

    // Read the line
    std::stringstream offsets(line);
    // Output in hex
    offsets >> std::hex;
    // Location to save value
    u32 offset;

    // Parse each offset to add from the line. Separated with " "
    while (offsets >> offset)
        m_addresses[line].push_back(offset);
}

bool LoadAddresses(const std::string& path)
{
    // Open file
    std::fstream fs;
    fs.open(path.c_str(), std::fstream::in);
    if (!fs.is_open())
        return false;

    // Dump the file
    std::string line;
    while (std::getline(fs, line))
        ParseLine(line);

    // Make sure we read at LEAST 1 address line
    return !m_values.empty();
}

u32 ChasePointer(const std::string& line)
{
    // Initial memory address
    u32 value = 0;
    // Get each offset
    for (u32 offset : m_addresses[line])
        // getRand is actually Memory::Read_U32(value + offset); 
        // in the original code.
        value = getRand(0, 255) + value;
    return value;
}

std::string ComposeMessages()
{
    // Make a stream to push messages to
    std::stringstream message_stream;
    message_stream << std::hex;

    // For each value we watch
    for (auto& entry : m_values)
    {
        // Get the line from the text file
        std::string address = entry.first;
        // and the current value
        u32& current_value = entry.second;

        // chase down the pointers
        u32 new_value = ChasePointer(address);

        if (new_value != current_value)
        {
            // Update the value
            current_value = new_value;
            message_stream << address << '\n' << new_value << '\n';
        }
    }

    // return the whole message buffer
    return message_stream.str();
}


int main()
{
    // Seed Rand
    srand(static_cast<unsigned>(time(0)));

    std::string path = "loc.txt";
    if (!LoadAddresses(path))
    {
        printf("loc.txt is missing, writing file. . .\n");
        FILE* fd = fopen("loc.txt", "w");
        fwrite(loctext.c_str(), sizeof(char), loctext.size(), fd);
        fclose(fd);
        LoadAddresses(path);
    }

    std::string memdump = ComposeMessages();

    printf("Message from dolphin:\n%s\n", memdump.c_str());
}