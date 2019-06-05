#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>
#include <stdint.h>
#include "MemoryWatcher.h"
#include "GameState.h"
#include <algorithm>
#define FILENM "MS"

/* Memory Watcher
 * This file is sourced from another Dolphin Super Smash Bros bot, altf4/Smashbot
 * https://github.com/altf4/SmashBot/blob/dfa904883a11ee03cee2e4d7495d96056ecc05aa/Util/MemoryWatcher.cpp
 *
 * We are using the source file as a template to parse memory locations which can
 * sometimes be quite frustrating to parse. Using this source helped cut down a
 * lot of time and allowed us to train. I have commented out a lot of the
 * functionality that we are not currently tracking. We may add this back in
 * later.
 *
 * Main functions used:
 * bool MemoryWatcher::ReadMemory() -> bool MemoryWatcher::UpdatedFrame(bool prin)
 **/


char* menus[] =
{
    "CHARACTER_SELECT",
    "STAGE_SELECT",
    "IN_GAME",
    "POSTGAME_SCORES",
};

char* stages[] =
{
    "BATTLEFIELD",
    "FINAL_DESTINATION",
    "DREAMLAND",
    "FOUNTAIN_OF_DREAMS",
    "POKEMON_STADIUM",
    "YOSHI_STORY",
};
MemoryWatcher::MemoryWatcher(std::string inUserDir)
{
    /*initialize structs to default/trash values*/
    p1.health = 1000; p2.health = 1000;
    p1.dir = 10; p2.dir = 10;
    p1.pos_y = -1024; p1.pos_x = -1024;
    p2.pos_y = -1024; p2.pos_x = -1024;

    current_stage = -1;

    userPath = inUserDir;
    success = init_socket();

    printf("%s:%d\tMemory Scanner Created\n", FILENM, __LINE__);
}

MemoryWatcher::~MemoryWatcher() {

    printf("%s:%d\tDestroying MemoryWatcher\n", FILENM, __LINE__);
    /*shutdown the socket*/
    if (shutdown(this->socketfd, 2) < 0)
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:shutdown", strerror(errno));
    printf("%s:%d\tSocket Closed\n", FILENM, __LINE__);
}

Player MemoryWatcher::GetPlayer(bool pl)
{
    return !pl ? p1 : p2;
}

bool MemoryWatcher::print()
{

    /*quick check for all values to be updated before being sent to model*/
    if (p1.dir == 10 || p2.dir == 10)
    {
        printf("%s:%d\t--Invalid Data: %d:%d\n", FILENM, __LINE__,
            p1.dir, p2.dir);
        return false;
    }
    if (p1.pos_x == -1024 || p1.pos_y == -1024)
    {
        printf("%s:%d\t--Invalid Data: %f:%f\n", FILENM, __LINE__,
            p1.pos_x, p1.pos_y);
        return false;
    }
    if (p2.pos_x == -1024 || p2.pos_y == -1024)
    {
        printf("%s:%d\t--Invalid Data: %f:%f\n", FILENM, __LINE__,
            p2.pos_x, p2.pos_y);
        return false;
    }

    printf("%s:%d\tMemory Scan\n"
        "\tP1:%u P1:%d P1:%f P1:%f\n", FILENM, __LINE__,
        p1.health, p1.dir, p1.pos_x, p1.pos_y);
    printf("\tP2:%u P2:%d P2:%f P2:%f\n",
        p2.health, p2.dir, p2.pos_x, p2.pos_y);
    return true;
}

/*initializes unix socket in default path.... later maybe add custom path support/env var*/
bool MemoryWatcher::init_socket() {

    printf("%s:%d\tCreating Socket\n", FILENM, __LINE__);
    std::string sock_path = userPath + "MemoryWatcher/MemoryWatcher";
    printf("%s:%d\tSocket Path: %s\n", FILENM, __LINE__, sock_path.c_str());


    /*set up socket*/
    if ((socketfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:socket", strerror(errno));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    unlink(sock_path.c_str());

    strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(socketfd, (struct sockaddr*) & addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:bind", strerror(errno));
        return false;
    }
    return true;
}

bool MemoryWatcher::UpdatedFrame(bool prin)
{
    char buf[128];
    memset(buf, '\0', 128);

    struct sockaddr remaddr;
    socklen_t addr_len;
    memset(&addr_len, '\0', sizeof(addr_len));
    recvfrom(m_file, buf, sizeof(buf), 0, &remaddr, &addr_len);
    std::stringstream ss(buf);
    std::string region, value;

    std::getline(ss, region, '\n');
    std::getline(ss, value, '\n');
    unsigned int value_int;

    //Is this a followed pointer?
    std::size_t found = region.find(" ");
    if (found != std::string::npos)
    {
        std::string ptr = region.substr(found + 1);
        std::string base = region.substr(0, found);
        unsigned int ptr_int = std::stoul(ptr.c_str(), nullptr, 16);
        unsigned int base_int = std::stoul(base.c_str(), nullptr, 16);

        ////Player one
        //if (base_int == 0x453130)
        //{
        //    switch (ptr_int)
        //    {
        //        //Action
        //    case 0x70:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        m_state->m_memory->player_one_action = value_int;
        //        break;
        //    }
        //    //Action counter
        //    case 0x20CC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        m_state->m_memory->player_one_action_counter = value_int;
        //        break;
        //    }
        //    //Action frame
        //    case 0x8F4:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_action_frame = x;
        //        break;
        //    }
        //    //Invulnerable
        //    case 0x19EC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        if (value_int == 0)
        //        {
        //            m_state->m_memory->player_one_invulnerable = false;
        //        }
        //        else
        //        {
        //            m_state->m_memory->player_one_invulnerable = true;
        //        }
        //        break;
        //    }
        //    //Hitlag
        //    case 0x19BC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_hitlag_frames_left = x;
        //        break;
        //    }
        //    //Hitstun
        //    case 0x23a0:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_hitstun_frames_left = x;
        //        break;
        //    }
        //    //Is charging a smash?
        //    case 0x2174:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        m_state->m_memory->player_one_action = value_int;
        //        if (value_int == 2)
        //        {
        //            m_state->m_memory->player_one_charging_smash = true;
        //        }
        //        else
        //        {
        //            m_state->m_memory->player_one_charging_smash = false;
        //        }
        //        break;
        //    }
        //    //Jumps remaining
        //    case 0x19C8:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        value_int = value_int >> 24;
        //        //TODO: This won't work for characters with multiple jumps
        //        if (value_int > 1)
        //        {
        //            value_int = 0;
        //        }
        //        m_state->m_memory->player_one_jumps_left = value_int;
        //        break;
        //    }
        //    //Is on ground?
        //    case 0x140:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        if (value_int == 0)
        //        {
        //            m_state->m_memory->player_one_on_ground = true;
        //        }
        //        else
        //        {
        //            m_state->m_memory->player_one_on_ground = false;
        //        }
        //        break;
        //    }
        //    //X air speed self
        //    case 0xE0:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_speed_air_x_self = x;
        //        break;
        //    }
        //    //Y air speed self
        //    case 0xE4:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_speed_y_self = x;
        //        break;
        //    }
        //    //X attack
        //    case 0xEC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_speed_x_attack = x;
        //        break;
        //    }
        //    //Y attack
        //    case 0xF0:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_speed_y_attack = x;
        //        break;
        //    }
        //    //x ground self
        //    case 0x14C:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_one_speed_ground_x_self = x;
        //        break;
        //    }
        //    default:
        //    {
        //        std::cout << "WARNING: Got an unexpected memory pointer: " << ptr_int << std::endl;
        //    }
        //    }
        //}
        ////Player two
        //else if (base_int == 0x453FC0)
        //{
        //    switch (ptr_int)
        //    {
        //        //Action
        //    case 0x70:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        m_state->m_memory->player_two_action = value_int;
        //        break;
        //    }
        //    //Action counter
        //    case 0x20CC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        m_state->m_memory->player_two_action_counter = value_int;
        //        break;
        //    }
        //    //Action frame
        //    case 0x8F4:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_action_frame = x;
        //        break;
        //    }
        //    //Invulnerable
        //    case 0x19EC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        if (value_int == 0)
        //        {
        //            m_state->m_memory->player_two_invulnerable = false;
        //        }
        //        else
        //        {
        //            m_state->m_memory->player_two_invulnerable = true;
        //        }
        //        break;
        //    }
        //    //Hitlag
        //    case 0x19BC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_hitlag_frames_left = x;
        //        break;
        //    }
        //    //Hitstun
        //    case 0x23a0:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_hitstun_frames_left = x;
        //        break;
        //    }
        //    //Is charging a smash?
        //    case 0x2174:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        m_state->m_memory->player_two_action = value_int;
        //        if (value_int == 2)
        //        {
        //            m_state->m_memory->player_two_charging_smash = true;
        //        }
        //        else
        //        {
        //            m_state->m_memory->player_two_charging_smash = false;
        //        }
        //        break;
        //    }
        //    //Jumps remaining
        //    case 0x19C8:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        value_int = value_int >> 24;
        //        //TODO: This won't work for characters with multiple jumps
        //        if (value_int > 1)
        //        {
        //            value_int = 0;
        //        }
        //        m_state->m_memory->player_two_jumps_left = value_int;
        //        break;
        //    }
        //    //Is on ground?
        //    case 0x140:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        if (value_int == 0)
        //        {
        //            m_state->m_memory->player_two_on_ground = true;
        //        }
        //        else
        //        {
        //            m_state->m_memory->player_two_on_ground = false;
        //        }
        //        break;
        //    }
        //    //X air speed self
        //    case 0xE0:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_speed_air_x_self = x;
        //        break;
        //    }
        //    //Y air speed self
        //    case 0xE4:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_speed_y_self = x;
        //        break;
        //    }
        //    //X attack
        //    case 0xEC:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_speed_x_attack = x;
        //        break;
        //    }
        //    //Y attack
        //    case 0xF0:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_speed_y_attack = x;
        //        break;
        //    }
        //    //x ground self
        //    case 0x14C:
        //    {
        //        value_int = std::stoul(value.c_str(), nullptr, 16);
        //        unsigned int* val_ptr = &value_int;
        //        float x = *((float*)val_ptr);
        //        m_state->m_memory->player_two_speed_ground_x_self = x;
        //        break;
        //    }
        //    default:
        //    {
        //        std::cout << "WARNING: Got an unexpected memory pointer: " << ptr_int << std::endl;
        //    }
        //    }
        //}
        //else
        //{
        std::cout << "WARNING: Got an unexpected memory base pointer: " << base_int << std::endl;
        //}
    }
    //If not, it's a direct pointer
    else
    {
        unsigned int region_int = std::stoul(region.c_str(), nullptr, 16);
        switch (region_int)
        {
            //Frame
        /*case 0x479D60:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            m_state->m_memory->frame = value_int;
            break;
        }*/
        //Player 1 percent
        case 0x4530E0:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            value_int = value_int >> 16;
            p1.health = value_int;
            //m_state->m_memory->player_one_percent = value_int;
            printf("%s:%d\tP1.health = %u\n", FILENM, __LINE__, value_int);
            break;
        }
        //Player 2 percent
        case 0x453F70:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            value_int = value_int >> 16;
            p2.health = value_int;
            //m_state->m_memory->player_two_percent = value_int;
            printf("%s:%d\tP2.health = %u\n", FILENM, __LINE__, value_int);
            break;
        }
        ////Player 1 stock
        //case 0x45310E:
        //{
        //    value_int = std::stoul(value.c_str(), nullptr, 16);
        //    value_int = value_int >> 24;
        //    m_state->m_memory->player_one_stock = value_int;

        //    break;
        //}
        ////Player 2 stock
        //case 0x453F9E:
        //{
        //    value_int = std::stoul(value.c_str(), nullptr, 16);
        //    value_int = value_int >> 24;
        //    m_state->m_memory->player_two_stock = value_int;
        //    break;
        //}
        //Player 1 facing
        case 0x4530C0:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            bool facing = value_int >> 31;
            p1.dir = facing ? -1 : 1;
            //m_state->m_memory->player_one_facing = !facing;
            printf("%s:%d\tP1.dir = %f\n", FILENM, __LINE__, p1.dir);
            break;
        }
        //Player 2 facing
        case 0x453F50:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            bool facing = value_int >> 31;
            p2.dir = facing ? -1 : 1;
            //m_state->m_memory->player_two_facing = !facing;
            printf("%s:%d\tP1.dir = %f\n", FILENM, __LINE__, p2.dir);
            break;
        }
        //Player 1 x
        case 0x453090:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p1.pos_x = x;
            //m_state->m_memory->player_one_x = x;
            printf("%s:%d\tP1.pos_x = %f\n", FILENM, __LINE__, p1.pos_x);
            break;
        }
        //Player 2 x
        case 0x453F20:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p2.pos_x = x;
            //m_state->m_memory->player_two_x = x;
            printf("%s:%d\tP2.pos_x = %f\n", FILENM, __LINE__, p2.pos_x);
            break;
        }
        //Player 1 y
        case 0x453094:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p1.pos_y = x;
            //m_state->m_memory->player_one_y = x;
            printf("%s:%d\tP1.pos_y = %f\n", FILENM, __LINE__, p1.pos_y);
            break;
        }
        //Player 2 y
        case 0x453F24:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p2.pos_y = x;
            //m_state->m_memory->player_two_y = x;
            printf("%s:%d\tP2.pos_y = %f\n", FILENM, __LINE__, p2.pos_y);
            break;
        }
        ////Player one character
        //case 0x3F0E0A:
        //{
        //    value_int = std::stoul(value.c_str(), nullptr, 16);
        //    value_int = value_int >> 24;
        //    m_state->m_memory->player_one_character = value_int;
        //    break;
        //}
        ////Player two character
        //case 0x3F0E2E:
        //{
        //    value_int = std::stoul(value.c_str(), nullptr, 16);
        //    value_int = value_int >> 24;
        //    m_state->m_memory->player_two_character = value_int;
        //    break;
        //}
        //Menu state
        case 0x479d30:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            value_int &= 0x000000FF;
            current_Menu = value_int;
            //m_state->m_memory->menu_state = value_int;
            printf("%s:%d\tCurrent Menu = %s\n", FILENM, __LINE__, 
                menus[current_Menu]);
            break;
        }
        //Stage
        case 0x4D6CAD:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            value_int = value_int >> 16;
            current_stage = value_int;
            //m_state->m_memory->stage = value_int;
            switch (current_stage)
            {
            case 0x18:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    stages[0]);
                break;
            case 0x19:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    stages[1]);
                break;
            case 0x1a:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    stages[2]);
                break;
            case 0x8:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    stages[3]);
                break;
            case 0x12:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    stages[4]);
                break;
            case 0x6:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    stages[5]);
                break;
            default:
                printf("%s:%d\tCurrent Stage = %s\n", FILENM, __LINE__,
                    "UNKNOWN");
                break;
            }
            break;
        }
        //p1 cursor x
        case 0x01118DEC:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p1.cursor_x = x;
            //m_state->m_memory->player_two_pointer_x = x;
            printf("%s:%d\tP1.cursor_x = %f\n", FILENM, __LINE__, p1.cursor_x);
            break;
        }
        //p1 cursor y
        case 0x01118DF0:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p1.cursor_y = x;
            //m_state->m_memory->player_two_pointer_y = x;
            printf("%s:%d\tP1.cursor_y = %f\n", FILENM, __LINE__, p1.cursor_y);
            break;
        }
        //p2 cursor x
        case 0x0111826C:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p2.cursor_x = x;
            //m_state->m_memory->player_two_pointer_x = x;
            printf("%s:%d\tP2.cursor_x = %f\n", FILENM, __LINE__, p2.cursor_x);
            break;
        }
        //p2 cursor y
        case 0x01118270:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            unsigned int* val_ptr = &value_int;
            float x = *((float*)val_ptr);
            p2.cursor_y = x;
            //m_state->m_memory->player_two_pointer_y = x;
            printf("%s:%d\tP2.cursor_y = %f\n", FILENM, __LINE__, p2.cursor_y);
            break;
        }
        case 0x003F0E08:
        {
            value_int = std::stoul(value.c_str(), nullptr, 16);
            std::cout << std::hex << "P1: " << value_int << std::endl;
            break;
        }
        }
    }

    if (region == "00479D60")
    {
        return true;
    }
    return false;
}

