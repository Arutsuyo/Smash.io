#include "MemoryWatcher.h"
#include "GameState.h"
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
#include <algorithm>
#include <vector>
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
 * bool MemoryWatcher::ReadMemory() -> bool MemoryWatcher::ReadMemory(bool prin)
 **/


std::string menus[] =
{
    "CHARACTER_SELECT",
    "STAGE_SELECT",
    "IN_GAME",
    "POSTGAME_SCORES",
};

std::string stages[] =
{
    "BATTLEFIELD",
    "FINAL_DESTINATION",
    "DREAMLAND",
    "FOUNTAIN_OF_DREAMS",
    "POKEMON_STADIUM",
    "YOSHI_STORY",
};

MemoryWatcher::MemoryWatcher(std::string inUserDir, bool dumpRead)
{
    // Open the dump file
    if (dumpRead)
        memdumpfd = fopen("memdump.txt", "w");

    current_stage = -1;

    userPath = inUserDir;
    success = init_socket();

    printf("%s:%d\tMemory Scanner Created\n", FILENM, __LINE__);
}

MemoryWatcher::~MemoryWatcher() {
    if (memdumpfd)
        fclose(memdumpfd);

    printf("%s:%d\tDestroying MemoryWatcher\n", FILENM, __LINE__);
    /*shutdown the socket*/
    if (shutdown(m_file, 2) < 0)
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
    if (p1.facing == 0 || p2.facing == 0)
    {
        printf("%s:%d\t--Invalid Data: %d:%d\n", FILENM, __LINE__,
            p1.facing, p2.facing);
        return false;
    }
    if (p1.pos_x == 0 || p1.pos_y == 0) // Generally you will start opposing
    {
        printf("%s:%d\t--Invalid Data: %f:%f\n", FILENM, __LINE__,
            p1.pos_x, p1.pos_y);
        return false;
    }

    printf("%s:%d\tMemory Scan\n"
        "\tP1:%hi P1:%f P1:%f P1:%f\n", FILENM, __LINE__,
        p1.percent, p1.facing, p1.pos_x, p1.pos_y);
    printf("\tP2:%hi P1:%f P1:%f P1:%f\n",
        p2.percent, p2.facing, p2.pos_x, p2.pos_y);
    return true;
}

/*initializes unix socket in default path.... later maybe add custom path support/env var*/
bool MemoryWatcher::init_socket() {

    printf("%s:%d\tCreating Socket\n", FILENM, __LINE__);
    std::string sock_path = userPath + "MemoryWatcher/MemoryWatcher";
    printf("%s:%d\tSocket Path: %s\n", FILENM, __LINE__, sock_path.c_str());


    /*set up socket*/
    if ((m_file = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0)) == -1)
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:socket", strerror(errno));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    unlink(sock_path.c_str());

    strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_file, (struct sockaddr*) & addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:bind", strerror(errno));
        return false;
    }
    return true;
}

bool MemoryWatcher::ReadMemory(bool prin)
{
    int buflen = 128, ret = -1;
    char* buf = new char[buflen];
    memset(buf, '\0', buflen);

    struct sockaddr remaddr;
    socklen_t addr_len;
    memset(&addr_len, '\0', sizeof(addr_len));

    if ((ret = recvfrom(m_file, buf, sizeof(buf), 0, &remaddr, &addr_len)) == -1)
    {
        // Check if the socket is just empty
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            delete buf;
            return true;
        }

        // Nope, we error'd
        delete buf;
        fprintf(stderr, "%s:%d: %s: %s\n", FILENM, __LINE__,
            "--ERROR:recvfrom", strerror(errno));
        return false;
    }

    if (memdumpfd)
        fwrite(buf, sizeof(char), strlen(buf), memdumpfd);

    std::stringstream ss(buf);
    std::string region, value;

    // Loop here, the buffer reads 2 lines at a time. If the buffer has 
    // something, then it has 2 things. Message Construction:
    // https://github.com/dolphin-emu/dolphin/blob/123bbbca2c3382165cf58288e4d65d0974a9c0db/Source/Core/Core/MemoryWatcher.cpp#L77
    // A demo of it can be found in "Test Files/mem.cpp"
    while (ss.rdbuf()->in_avail())
    {
        std::getline(ss, region, '\n');
        std::getline(ss, value, '\n');
        uint32 value_int;

        // Get the base address
        std::size_t found = region.find(" ");
        //Is this a followed pointer?
        std::size_t found = region.find(" ");

        // See if this is a global address
        if (found != std::string::npos)
        {
            std::string ptr = region.substr(found + 1);
            std::string base = region.substr(0, found);
            uint32 ptr_int = std::stoul(ptr.c_str(), nullptr, 16);
            uint32 base_int = std::stoul(base.c_str(), nullptr, 16);

            // Figure out what base we're parsing
            switch (base_int)
            {
            case P1CHAR:
            case P2CHAR:
            {
                // Get the players we're changing
                Player* ply = (base_int == P1CHAR) ?
                    &p1 :&p2;
                // Figure out what ptr we followed
                switch (ptr_int)
                {
                    //Action
                case ACTION:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->action_state = value_int;
                    break;
                    //Action counter
                case RFACING:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->action_state = val;
                    break;
                    // Self Hor Velocity
                case SHVELO:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->self_h_velocity = val;
                    break;
                case SVVELO:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->self_v_velocity = val;
                    break;
                case AHVELO:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->att_h_velocity = val;
                    break;
                case AVVELO:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->att_v_velocity = val;
                    break;
                case XPOS:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->pos_x = val;
                    break;
                case YPOS:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->pos_y = val;
                    break;
                case XDELTA:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->delta_x = val;
                    break;
                case YDELTA:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->delta_y = val;
                    break;
                case GROUND:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->onGround = value_int == 0 ? true : false;
                    break;
                case ACTIONCOUNTER:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->action_frame_count = val;
                    break;
                case JUMPS:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->times_jumped = value_int;
                    break;
                case BSTATE:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->body_state = value_int;
                    break;
                case SMASHSTATE:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->smash_state = value_int;
                    break;
                case SMASHCOUNT:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->smash_frame_counter = value_int;
                    break;
                default:
                    std::cout << "WARNING: Unexpected PCHAR: "
                        << std::hex << ptr_int << std::endl;
                } // End ptr switch
            }// End PCHAR case

            case P1STATIC:
            case P2STATIC:
            {
                // Get the players we're changing
                Player* ply = (base_int == P1STATIC) ?
                    &p1 : &p2;
                // Figure out what ptr we followed
                switch (ptr_int)
                {
                case FACING:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->s_facing = val;
                    break;
                case PERCENT:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->percent = value_int;
                    break;
                case FALLS:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->falls = value_int;
                    break;
                case STOCKS:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    ply->stocks = value_int;
                    break;
                case TDR:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->TDR = val;
                    break;
                case TDG:
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    ply->TDG = val;
                    break;
                default:
                    std::cout << "WARNING: Unexpected PSTATIC: "
                        << std::hex << ptr_int << std::endl;
                } // End ptr
            } // End PSTATIC

            case STAGEPT:
            {
                switch (ptr_int)
                {
                case 0x02:
                    printf("%s:%d\tInternal Stage: Princess Peach's Castle\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x03:
                    printf("%s:%d\tInternal Stage: Rainbow Cruise\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x04:
                    printf("%s:%d\tInternal Stage: Kongo Jungle\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x05:
                    printf("%s:%d\tInternal Stage: Jungle Japes\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x06:
                    printf("%s:%d\tInternal Stage: Great Bay\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x07:
                    printf("%s:%d\tInternal Stage: Hyrule Temple\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x08:
                    printf("%s:%d\tInternal Stage: Brinstar\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x09:
                    printf("%s:%d\tInternal Stage: Brinstar Depths\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x0A:
                    printf("%s:%d\tInternal Stage: Yoshi's Story\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x0B:
                    printf("%s:%d\tInternal Stage: Yoshi's Island\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x0C:
                    printf("%s:%d\tInternal Stage: Fountain of Dreams\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x0D:
                    printf("%s:%d\tInternal Stage: Green Greens\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x0E:
                    printf("%s:%d\tInternal Stage: Corneria\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x0F:
                    printf("%s:%d\tInternal Stage: Venom\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x10:
                    printf("%s:%d\tInternal Stage: Pokemon Stadium\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x11:
                    printf("%s:%d\tInternal Stage: Poke Floats\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x12:
                    printf("%s:%d\tInternal Stage: Mute City\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x13:
                    printf("%s:%d\tInternal Stage: Big Blue\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x14:
                    printf("%s:%d\tInternal Stage: Onett\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x15:
                    printf("%s:%d\tInternal Stage: Fourside\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x16:
                    printf("%s:%d\tInternal Stage: Icicle Mountain\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x18:
                    printf("%s:%d\tInternal Stage: Mushroom Kingdom\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x19:
                    printf("%s:%d\tInternal Stage: Mushroom Kingdom II\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x1B:
                    printf("%s:%d\tInternal Stage: Flat Zone\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x1C:
                    printf("%s:%d\tInternal Stage: Dream Land\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x1D:
                    printf("%s:%d\tInternal Stage: Yoshi's Island (64)\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                case 0x1E:
                    printf("%s:%d\tInternal Stage: Kongo Jungle(64)\n", FILENM, __LINE__);
                    internal_stage = ptr_int;
                    break;
                default:
                    std::cout << "WARNING: Unexpected STAGEPT: "
                        << std::hex << ptr_int << std::endl;
                    internal_stage = ptr_int;
                }// End internal stage
            }
                default:
                    std::cout << "WARNING: Unexpected BASE_PTR: "
                        << std::hex << base_int << std::endl;
            }// End BASE+PTR switch

            // Check out Global addresses
            uint32 region_int = std::stoul(region.c_str(), nullptr, 16);
            switch (region_int)
            {
                // Global frame timer
                case FRAMETIMER:
                {
                    std::cout << "WARNING: NOT IMPLEMENTED FRAMETIMER: "
                        << std::hex << base_int << std::endl;

                    /*value_int = std::stoul(value.c_str(), nullptr, 16);
                    m_state->m_memory->frame = value_int;*/
                    break;
                }
                //Menu state
                case SCENECTRL:
                {
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    value_int &= 0x000000FF;
                    current_Menu = value_int;
                    break;
                }
                //Stage
                case STAGELOAD:
                {
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    value_int = value_int >> 16;
                    current_stage = value_int;
                    break;
                }
                //p1 cursor x
                case P1CURSORX:
                {
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    p1.cursor_x = val;
                    break;
                }
                //p1 cursor y
                case P1CURSORY:
                {
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    p1.cursor_y = val;
                    break;
                }
                //p2 cursor x
                case P2CURSORX:
                {
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    p2.cursor_x = val;
                    break;
                }
                //p2 cursor y
                case P2CURSORY:
                {
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    uint32* val_ptr = &value_int;
                    float val = *((float*)val_ptr);
                    p2.cursor_y = val;
                    break;
                }
                case P1CHARSELECTED:
                case P2CHARSELECTED:
                {
                    std::string* charnm = (region_int == P1CHARSELECTED) ?
                        &p1name : &p2name;
                    value_int = std::stoul(value.c_str(), nullptr, 16);
                    value_int &= 0x000000FF;
                    switch (value_int)
                    {
                    case 0x00:
                        *charnm = "Dr.Mario";
                        break;
                    case 0x01:
                        *charnm = "Mario";
                        break;
                    case 0x02:
                        *charnm = "Luigi";
                        break;
                    case 0x03:
                        *charnm = "Bowser";
                        break;
                    case 0x04:
                        *charnm = "Peach";
                        break;
                    case 0x05:
                        *charnm = "Yoshi";
                        break;
                    case 0x06:
                        *charnm = "DK";
                        break;
                    case 0x07:
                        *charnm = "Cpt.Falcon";
                        break;
                    case 0x08:
                        *charnm = "Ganon";
                        break;
                    case 0x09:
                        *charnm = "Falco";
                        break;
                    case 0x0A:
                        *charnm = "Fox";
                        break;
                    case 0x0B:
                        *charnm = "Ness";
                        break;
                    case 0x0C:
                        *charnm = "Ice_Climbers";
                        break;
                    case 0x0D:
                        *charnm = "Kirby";
                        break;
                    case 0x0E:
                        *charnm = "Samus";
                        break;
                    case 0x0F:
                        *charnm = "Zelda";
                        break;
                    case 0x10:
                        *charnm = "Link";
                        break;
                    case 0x11:
                        *charnm = "Y_Link";
                        break;
                    case 0x12:
                        *charnm = "Pichu";
                        break;
                    case 0x13:
                        *charnm = "Pikachu";
                        break;
                    case 0x14:
                        *charnm = "Jiggly";
                        break;
                    case 0x15:
                        *charnm = "Mewtwo";
                        break;
                    case 0x16:
                        *charnm = "Game_Watch";
                        break;
                    case 0x17:
                        *charnm = "Marth";
                        break;
                    case 0x18:
                        *charnm = "Roy";
                        break;

                    }
                    std::cout << std::hex << "P1: " << value_int << std::endl;
                    break;
                }
            }// End Switch
        }// End Else

    }// End while loop

    return true;
}

