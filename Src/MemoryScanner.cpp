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
#include "MemoryScanner.h"
#include "addresses.h"
#include <algorithm>
#define FILENM "MS"

/* Original Source: Memory Watcher
 * This file is sourced from another Dolphin Super Smash Bros bot, altf4/Smashbot
 * https://github.com/altf4/SmashBot/blob/dfa904883a11ee03cee2e4d7495d96056ecc05aa/Util/MemoryWatcher.cpp
 *
 * We are using the source file as a template to parse memory locations which can
 * sometimes be quite frustrating to parse. Using this source helped cut down a
 * lot of time and allowed us to train. Dolphin communicates with designated sockets
 * that have a specific format, thus leading to very similar patterns of parsing 
 * the messages received. Dolphins socket system can be seen here: 
 * https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/MemoryWatcher.cpp
 *
 * Main functions used:
 * bool MemoryWatcher::ReadMemory() -> int MemoryScanner::UpdateFrame()
 **/

MemoryScanner::MemoryScanner(std::string inUserDir)
{
    /*initialize structs to default/trash values*/
    p1.health = 0; p2.health = 0;
    p1.dir = 10; p2.dir = 10;
    p1.pos_x = 60; p2.pos_x = -60;
    p1.pos_y = 10; p2.pos_y = 10;

    userPath = inUserDir;
    success = init_socket();

    printf("%s:%d\tMemory Scanner Created\n", FILENM, __LINE__);
}

MemoryScanner::~MemoryScanner() {

    printf("%s:%d\tDestroying MemoryScanner\n", FILENM, __LINE__);
    /*shutdown the socket*/
    if (shutdown(socketfd, 2) < 0)
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:shutdown", strerror(errno));
    printf("%s:%d\tSocket Closed\n", FILENM, __LINE__);
}

Player MemoryScanner::GetPlayer(bool pl)
{
    return !pl ? p1 : p2;
}

bool MemoryScanner::print()
{
    
    /*quick check for all values to be updated before being sent to model*/
    if (p1.dir == 10 || p2.dir == 10)
        return false;

    if (p1.pos_x == -1024 || p1.pos_y == -1024)
        return false;

    if (p2.pos_x == -1024 || p2.pos_y == -1024)
        return false;

    printf("%s:%d\tMemory Scan\n"
        "\tP1:%u P1:%d P1:%f P1:%f\n", FILENM, __LINE__,
        p1.health, p1.dir, p1.pos_x, p1.pos_y);
    printf("\tP2:%u P2:%d P2:%f P2:%f\n",
        p2.health, p2.dir, p2.pos_x, p2.pos_y);
    return true;
}

/*initializes unix socket in default path.... later maybe add custom path support/env var*/
bool MemoryScanner::init_socket() {

    printf("%s:%d\tCreating Socket\n", FILENM, __LINE__);
    std::string sock_path = userPath + "MemoryWatcher/MemoryWatcher";
    printf("%s:%d\tSocket Path: %s\n", FILENM, __LINE__, sock_path.c_str());


    /*set up socket*/
    if ((socketfd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0)) == -1)
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

int MemoryScanner::UpdatedFrame() {

    if (socketfd < 0) {
        std::cout << "Error socket file descriptor is bad" << std::endl;
        return -1;
    }

    int ret = -1;
    char buffer[128];
    memset(buffer, '\0', 128);

    unsigned int val_int;

#if MEMORY_OUT
        printf("%s:%d\tReading Socket\n", FILENM, __LINE__);
#endif

    struct sockaddr recvsock;
    socklen_t sock_len;
    if ((ret = recvfrom(socketfd, 
        buffer, sizeof(buffer), 0, &recvsock, &sock_len)) == -1)
    {
        // Check if the socket is just empty
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
#if MEMORY_OUT
            printf("%s:%d\tBuffer was empty\n", FILENM, __LINE__);
#endif
            return 0;
        }

        // Nope, we error'd
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:recvfrom", strerror(errno));
        return -1;
    }

#if MEMORY_OUT
    std::string temp = buffer;
    temp = temp.substr(0, temp.find("\n")) + " " + temp.substr(temp.find("\n") + sizeof("\n"));
    printf("%s:%d\tParsing Buffer: %s\n", FILENM, __LINE__, temp.c_str());
#endif

    //puts(buffer);
    std::stringstream ss(buffer);
    /*strings to hold addresses*/
    std::string base, val;

    getline(ss, base, '\n');
    getline(ss, val, '\n');

    /* to fix any issues with cross platform memory reading, remove commas*/
    val.erase(std::remove(val.begin(), val.end(), ','), val.end());

    /*attempt to find any pointers, should be ' ' but may be a comma
    double check this*/
    size_t pointer_ref = base.find(" ");

    /*check until the end*/
    if ( pointer_ref != std::string::npos) 
    {
        std::string offset = base.substr(pointer_ref + 1);
        std::string ptr_base = base.substr(0, pointer_ref);

        /*convert to base16 number*/
        unsigned int ptr = std::stoul(offset.c_str(), nullptr, 16);
        unsigned int p_base = std::stoul(ptr_base.c_str(), nullptr, 16);

        /*check the base pointer*/
        switch( p_base)
        {   
            /*Player 1 offsets*/
            case Addresses::PLAYERS::PLAYER_ONE:
                switch(ptr)
                {
                    /*action*/
                    case Addresses::FRAMES::ACTION:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        // Check for stage end v2
                        current_stage = (p1.action == 0 && val_int == 9) ?
                            Addresses::MENUS::POSTGAME : current_stage;
                        p1.action = val_int;
#if MEMORY_OUT
                        printf("%s:%d\tP1 ACTION: %x \n", FILENM, __LINE__, p1.action);
#endif
                        break;
                    }
                    /*action frame*/
                    case Addresses::FRAMES::ACTION_FRAME:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        unsigned int* vp = &val_int;
                        float af = *((float*)vp);
                        p1.action_frame = af;
#if MEMORY_OUT
                        printf("%s:%d\tP1 ACTION_FRAME: %f\n", FILENM, __LINE__, p1.action_frame);
#endif
                        break;
                    }
                    /*action counter*/
                    case Addresses::FRAMES::ACTION_COUNTER:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        p1.action_count = val_int;
#if MEMORY_OUT
                        printf("%s:%d\tP1 ACTION_COUNTER: %x \n", FILENM, __LINE__, p1.action_count);
#endif
                        break;
                    }
                    /*invincibility frame*/
                    case Addresses::FRAMES::INVINCIBLE:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        p1.invincibility = val_int;
#if MEMORY_OUT
                        printf("%s:%d\tP1 INVINCIBLE: %x \n", FILENM, __LINE__, p1.invincibility);
#endif
                        break;
                    }
                    /*remaining frames left in hitlag*/
                    case Addresses::FRAMES::HITLAG:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        unsigned int* hpnt = &val_int;
                        float hl = *((float*)hpnt);
                        p1.hitlag_remaining = hl;
#if MEMORY_OUT
                        printf("%s:%d\tP1 HITLAG: %f\n", FILENM, __LINE__, p1.hitlag_remaining);
#endif
                        break;
                    }
                    /*remaining frames left in hitstun*/
                    case Addresses::FRAMES::HITSTUN:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        unsigned int* hpnt = &val_int;
                        float hs = *((float*)hpnt);
                        p1.hitstun_remaining = hs;
#if MEMORY_OUT
                        printf("%s:%d\tP1 HITSTUN: %f\n", FILENM, __LINE__, p1.hitstun_remaining);
#endif
                        break;
                    }
                    /*charging attack indicator*/
                    case Addresses::FRAMES::SMASH_CHARGE:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        p1.action = val_int;
                        p1.charging = val_int == 2 ? 1 : 0;
#if MEMORY_OUT
                        printf("%s:%d\tP1 Charging Action: %x \n"
                            , FILENM, __LINE__, val_int);
#endif
                        break;
                    }
                    default:
                    {
                        printf("%s:%d\tWARNING::P1 BASE FOUND: 0x%x as offset but not caught\n",
                            FILENM, __LINE__, val_int);
                    }
                }// Player one switch
                break;

            /*Player Two*/
            case Addresses::PLAYERS::PLAYER_TWO:

                /*found a value follow the pointer arithmetic*/
                switch(ptr)
                {
                    
                    /*action*/
                    case Addresses::FRAMES::ACTION:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        // Check for stage end v2
                        current_stage = (p2.action == 0 && val_int == 9) ?
                            Addresses::MENUS::POSTGAME : current_stage;
                        p2.action = val_int;
#if MEMORY_OUT
                        printf("%s:%d\tP2 ACTION: %x \n", FILENM, __LINE__, p2.action);
#endif
                        break;
                    }
                    /*action frame*/
                    case Addresses::FRAMES::ACTION_FRAME:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        unsigned int* vp = &val_int;
                        float af = *((float*)vp);
                        p2.action_frame = af;
#if MEMORY_OUT
                        printf("%s:%d\tP2 ACTION_FRAME: %f\n", FILENM, __LINE__, p2.action_frame);
#endif
                        break;
                    }
                    /*action counter*/
                    case Addresses::FRAMES::ACTION_COUNTER:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        p2.action_count = val_int;
#if MEMORY_OUT
                        printf("%s:%d\tP2 ACTION_COUNTER: %x \n", FILENM, __LINE__, p2.action_count);
#endif
                        break;
                    }
                    /*invincibility frame*/
                    case Addresses::FRAMES::INVINCIBLE:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        p2.invincibility = val_int;
#if MEMORY_OUT
                        printf("%s:%d\tP2 INVINCIBLE: %x \n", FILENM, __LINE__, p2.invincibility);
#endif
                        break;
                    }
                    /*remaining frames left in hitlag*/
                    case Addresses::FRAMES::HITLAG:
                    {
                        //should be correct
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        unsigned int* hpnt = &val_int;
                        float hl = *((float*)hpnt);
                        p2.hitlag_remaining = hl;
#if MEMORY_OUT
                        printf("%s:%d\tP2 HITLAG: %f\n", FILENM, __LINE__, p2.hitlag_remaining);
#endif
                        break;
                    }
                    /*remaining frames left in hitstun*/
                    case Addresses::FRAMES::HITSTUN:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        unsigned int* hpnt = &val_int;
                        float hs = *((float*)hpnt);
                        p2.hitstun_remaining = hs;
#if MEMORY_OUT
                        printf("%s:%d\tP2 HITSTUN: %f\n", FILENM, __LINE__, p2.hitstun_remaining);
#endif
                        break;
                    }
                    /*charging attack indicator*/
                    case Addresses::FRAMES::SMASH_CHARGE:
                    {
                        val_int = std::stoul(val.c_str(), nullptr, 16);
                        p2.action = val_int;
                        p2.charging = val_int == 2 ? 1 : 0;
#if MEMORY_OUT
                        printf("%s:%d\tP2 Charging Action: %x \n"
                            , FILENM, __LINE__, val_int);
#endif
                        break;
                    }
                    default:
                    {
                        printf("%s:%d\tWARNING::P2 BASE FOUND: 0x%x as offset but not caught\n",
                            FILENM, __LINE__, val_int);
                    }

                } // End Player Two switch
                break;
                /*pointer value read, but don't know what it is*/
            default: 
            {
                printf("%s:%d\tWARNING::value found, read as %s, but not caught\n",
                    FILENM, __LINE__, val.c_str());
            }                   
        } // End Base Switch
    } // End base.find(" ") != std::string::npos
    else
    { // Globals

        unsigned int player_val = std::stoul(base.c_str(), nullptr, 16);
        switch (player_val) {
            /*p1 health*/
        case Addresses::PLAYER_ATTRIB::P1_HEALTH: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            p1.health = val_int >> 16;
#if MEMORY_OUT
                printf("%s:%d\tP1_HEALTH: %ui\n", FILENM, __LINE__, p1.health);
#endif
            break;  }
        case Addresses::PLAYER_ATTRIB::P1_COORD_X: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* vx = &val_int;
            float x = *((float*)vx);
            p1.pos_x = x;
#if MEMORY_OUT
            printf("%s:%d\tP1_COORD_X: %f\n", FILENM, __LINE__, p1.pos_x);
#endif
            break; }
        case Addresses::PLAYER_ATTRIB::P1_COORD_Y: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* vy = &val_int;
            float y = *((float*)vy);
            p1.pos_y = y;
#if MEMORY_OUT
            printf("%s:%d\tP1_COORD_Y: %f\n", FILENM, __LINE__, p1.pos_y);
#endif
            break; }
        case Addresses::PLAYER_ATTRIB::P1_DIR: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* d1 = &val_int;
            float dir1 = *((float*)d1);
            p1.dir = dir1;
#if MEMORY_OUT
            printf("%s:%d\tP1_DIR: %f\n", FILENM, __LINE__, p1.dir);
#endif
            break; }
                                               /*P2 */
        case Addresses::PLAYER_ATTRIB::P2_HEALTH:
            val_int = std::stoul(val.c_str(), nullptr, 16);
            p2.health = (val_int >> 16);
#if MEMORY_OUT
            printf("%s:%d\tP2_HEALTH: %ui\n", FILENM, __LINE__, p2.health);
#endif
            break;
        case Addresses::PLAYER_ATTRIB::P2_COORD_X: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* vx = &val_int;
            float x = *((float*)vx);
            p2.pos_x = x;
#if MEMORY_OUT
            printf("%s:%d\tP2_COORD_X: %f\n", FILENM, __LINE__, p2.pos_x);
#endif
            break; }
        case Addresses::PLAYER_ATTRIB::P2_COORD_Y: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* vy = &val_int;
            float y = *((float*)vy);
            p2.pos_y = y;
#if MEMORY_OUT
            printf("%s:%d\tP2_COORD_Y: %f\n", FILENM, __LINE__, p2.pos_y);
#endif
            break; }
        case Addresses::PLAYER_ATTRIB::P2_DIR: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* d2 = &val_int;
            float dir2 = *((float*)d2);
            p2.dir = dir2;
#if MEMORY_OUT
            printf("%s:%d\tP2_DIR: %f\n", FILENM, __LINE__, p2.dir);
#endif
            break; }

        case Addresses::MENUS::MENU_STATE:
            unsigned int z; //z holds value we need

            sscanf(val.substr(val.size()-3).c_str(), "%x", &z);

            switch (z) {
            case Addresses::MENUS::IN_GAME:
                current_stage = Addresses::MENUS::IN_GAME;
#if MEMORY_OUT || 1
                printf("%s:%d\tMENU_STATE:IN_GAME:: 0x%x\n", 
                    FILENM, __LINE__, current_stage);
#endif
                break;
            case Addresses::MENUS::POSTGAME:
                current_stage = Addresses::MENUS::POSTGAME;
#if MEMORY_OUT || 1
                printf("%s:%d\tMENU_STATE:POSTGAME:: 0x%x\n",
                    FILENM, __LINE__, current_stage);
#endif
                break;
            case Addresses::MENUS::CHARACTER_SELECT:
                current_stage = Addresses::MENUS::CHARACTER_SELECT;
#if MEMORY_OUT
                printf("%s:%d\tMENU_STATE:CHARACTER_SELECT:: 0x%x\n",
                    FILENM, __LINE__, current_stage);
#endif
                break;
            case Addresses::MENUS::STAGE_SELECT:
                current_stage = Addresses::MENUS::STAGE_SELECT;
#if MEMORY_OUT
                printf("%s:%d\tMENU_STATE:STAGE_SELECT:: 0x%x\n",
                    FILENM, __LINE__, current_stage);
#endif
                break;
            //detecting menu transitions... should ideally not get here
            case Addresses::MENUS::ERROR_VS_2CHAR: //vs select to character 
                current_stage = Addresses::MENUS::ERROR_STAGE;
#if MEMORY_OUT            
                fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__, 
                    "-- Error: Detected select game-mode to character select");
#endif          
                break;
            case Addresses::MENUS::ERROR_CHAR_2VS: //character to vs select
                current_stage = Addresses::MENUS::ERROR_STAGE;
#if MEMORY_OUT
                fprintf(stderr, "%s:%d\t%s\n", FILENM, __LINE__, 
                    "-- Error: Detected exit from character select to select game-mode");
#endif
                break;
            default:
                fprintf(stderr, "%s:%d\t%s 0x%x\n", FILENM, __LINE__,
                    "--WARNING::Menu offset read, but returned unknown value", z);
                break;
            }
            break;

        case Addresses::PLAYER_ATTRIB::P1_CURSOR_X: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* cx = &val_int;
            float cursx = *((float*)cx);
            p1.cursor_x = cursx;
#if MEMORY_OUT
            printf("%s:%d\tP1_CURSOR_X: %f\n", FILENM, __LINE__, p1.cursor_x);
#endif
            break; }

        case Addresses::PLAYER_ATTRIB::P1_CURSOR_Y: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* cy = &val_int;
            float cursy = *((float*)cy);
            p1.cursor_y = cursy;
#if MEMORY_OUT
            printf("%s:%d\tP1_CURSOR_Y: %f\n", FILENM, __LINE__, p1.cursor_y);
#endif
            break; }

        case Addresses::PLAYER_ATTRIB::P2_CURSOR_X: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* cx = &val_int;
            float cursx = *((float*)cx);
            p2.cursor_x = cursx;
#if MEMORY_OUT
            printf("%s:%d\tP2_CURSOR_X: %f\n", FILENM, __LINE__, p2.cursor_x);
#endif
            break; }

        case Addresses::PLAYER_ATTRIB::P2_CURSOR_Y: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* cy = &val_int;
            float cursy = *((float*)cy);
            p2.cursor_y = cursy;
#if MEMORY_OUT
            printf("%s:%d\tP2_CURSOR_Y: %f\n", FILENM, __LINE__, p2.cursor_y);
#endif
            break; }

        /*stage selection cursor different than character selection*/
        case Addresses::PLAYER_ATTRIB::STAGE_SELECT_X1:{
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* sx = &val_int;
            float scursx = *((float*)sx);
            p1.cursor_x = p2.cursor_x = scursx;
#if MEMORY_OUT
            printf("%s:%d\tSTAGE_SELECT_X1: %f\n", FILENM, __LINE__, p1.cursor_x);
#endif
            break; }

        case Addresses::PLAYER_ATTRIB::STAGE_SELECT_X2:{
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* sx = &val_int;
            float scursx = *((float*)sx);
            p1.cursor_x = p2.cursor_x = scursx;
#if MEMORY_OUT
            printf("%s:%d\tSTAGE_SELECT_X2: %f\n", FILENM, __LINE__, p1.cursor_x);
#endif
            break; }

        case Addresses::PLAYER_ATTRIB::STAGE_SELECT_Y1: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* sy = &val_int;
            float scursy = *((float*)sy);
            p1.cursor_y = p2.cursor_y = scursy;
#if MEMORY_OUT
            printf("%s:%d\tSTAGE_SELECT_Y1: %f\n", FILENM, __LINE__, p1.cursor_y);
#endif
            break; }

        case Addresses::PLAYER_ATTRIB::STAGE_SELECT_Y2: {
            val_int = std::stoul(val.c_str(), nullptr, 16);
            unsigned int* sy = &val_int;
            float scursy = *((float*)sy);
            p1.cursor_y = p2.cursor_y = scursy;
#if MEMORY_OUT
            printf("%s:%d\tSTAGE_SELECT_Y2: %f\n", FILENM, __LINE__, p1.cursor_y);
#endif
            break; }

        default:
        {
            printf("%s:%d\tWARNING::P2 BASE FOUND: 0x%x as offset but not caught\n",
                FILENM, __LINE__, val_int);
        }
        }


        /*only print information if we are in game*/

#if MEMORY_OUT
        if (in_game)
        {
            print();
            display_player_actions(p1);
            display_player_actions(p2);
        }
#endif
    } // End Globals

    return 1;
}

//helper function to debug player action frames
void MemoryScanner::display_player_actions(Player p){

     printf("%s:%d\tPlayer Action Frame State\n"
                    "Action : %d \n"
                    "Action Frame: %.4f \n"
                    "Action Count: %d \n"
                    "Invicible : %d \n"
                    "Hitlag Left: %.4f \n"
                    "Hitstun Left: %.4f \n"
                    "Charging : %d \n"
                , FILENM, __LINE__, p.action, p.action_frame, p.action_count, 
                p.invincibility, p.hitlag_remaining, p.hitstun_remaining, p.charging);

}
