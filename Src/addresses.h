#ifndef _ADDRESSES_H
#define _ADDRESSES_H

/* Original Source: GameState.h
 * This file is sourced from another Dolphin Super Smash Bros bot, altf4/Smashbot
 * https://github.com/altf4/SmashBot/blob/v1.0/Util/GameState.h
 *
 * We are using the source file as a template to parse memory locations which can
 * sometimes be quite frustrating to parse. Using this source helped cut down a
 * lot of time and allowed us to train. We have rewritten some of the enums to
 * track what we care about.
 **/

// a good portion of these addresses came from https://smashboards.com 
namespace Addresses {


    enum PLAYERS {
        /*ATTRIBUTE BASE POINTER*/
        PLAYER_ONE = 0x453130,
        PLAYER_TWO = 0x453FC0,
    };

    enum MENUS{
        /*offset to get stage */
        MENU_STATE = 0x479d30,

        // Menus / Stages
        CHARACTER_SELECT = 0xD00,
        STAGE_SELECT = 0xD01,
        IN_GAME = 0xD02,
        POSTGAME = 0xD04,
        ERROR_VS_2CHAR = 0x100,
        ERROR_CHAR_2VS = 0x200,
        ERROR_STAGE = 0x01,//this is also the video that plays but if we are there we should probably leave anyway
    };

    enum PLAYER_ATTRIB {

        /*health*/
        P1_HEALTH = 0x4530E0,
        P2_HEALTH = 0x453F70,

        /*stock*/
        P1_STOCK = 0x45310E,
        P2_STOCK = 0x453F9E,

        /*direction*/
        P1_DIR = 0x4530C0,
        P2_DIR = 0x453F50,

        /*X direction*/
        P1_COORD_X = 0x453090,
        P2_COORD_X = 0x453F20,

        /*Y Direction*/
        P1_COORD_Y = 0x453094,
        P2_COORD_Y = 0x453F24,

        /*Cursor */
        P1_CURSOR_X = 0x01118DEC,
        P1_CURSOR_Y = 0x01118DF0,
        P2_CURSOR_X = 0x0111826C,
        P2_CURSOR_Y = 0x01118270,

        /*stage selection cursors */
        STAGE_SELECT_X1 = 0x0c8ee38,
        STAGE_SELECT_X2 = 0x0c8ee50,
        STAGE_SELECT_Y1 = 0x0C8EE3C,
        STAGE_SELECT_Y2 = 0x0C8EE60

    };

    enum FRAMES{

        ACTION = 0x70,
        ACTION_COUNTER = 0x20CC,
        ACTION_FRAME = 0X8f4, //working

        INVINCIBLE = 0X19EC, //maybe....
        HITLAG = 0X19BC, 
        HITSTUN = 0X23A0,
        SMASH_CHARGE = 0X2174, 
    };

    enum CHARACTERS {
        // Listed in Roster Order
        DR_MARIO = 0,
        MARIO = 1,
        LUIGI = 2,
        BOWSER = 3,
        PEACH = 4,
        YOSHI = 5,
        DK = 6,
        C_FALCON = 7,
        GANONDORF = 8,
        FALCO = 9,
        FOX = 10,
        NESS = 11,
        ICE_CLIMBERS = 12,
        KIRBY = 13,
        SAMUS = 14,
        ZELDA = 15,
        LINK = 16,
        YOUNG_LINK = 17,
        PICHU = 18,
        PIKACHU = 19,
        JIGGLYPUFF = 20,
        MEWTWO = 21,
        MR_GAME_AND_WATCH = 22,
        MARTH = 23,
        ROY = 24
    };

    enum ACTIONS {

        ON_HALO = 0x0d,
        STANDING = 0x0e,
        WALK_SLOW = 0x0f,
        WALK_MIDDLE = 0x10,
        WALK_FAST = 0x11,
        TURNING = 0x12,
        TURNING_RUN = 0x13,
        DASHING = 0x14,
        RUNNING = 0x15,
        KNEE_BEND = 0x18,
        JUMPING_FORWARD = 0x19,
        JUMPING_BACKWARD = 0x1A,
        JUMPING_ARIAL_FORWARD = 0x1b,
        JUMPING_ARIAL_BACKWARD = 0x1c,
        FALLING = 0x1D,   
        DEAD_FALL = 0x23, 
        CROUCH_START = 0x27,
        CROUCHING = 0x28,
        CROUCH_END = 0x29, 
        LANDING = 0x2a, 
        LANDING_SPECIAL = 0x2b, 
        NEUTRAL_ATTACK_1 = 0x2c,
        NEUTRAL_ATTACK_2 = 0x2d,
        NEUTRAL_ATTACK_3 = 0x2e,
        DASH_ATTACK = 0x32,
        FTILT_HIGH = 0x33,
        FTILT_HIGH_MID = 0x34,
        FTILT_MID = 0x35,
        FTILT_LOW_MID = 0x36,
        FTILT_LOW = 0x37,
        UPTILT = 0x38,
        DOWNTILT = 0x39,
        FSMASH_MID = 0x3c,
        UPSMASH = 0x3f,
        DOWNSMASH = 0x40,
        NAIR = 0x41,
        FAIR = 0x42,
        BAIR = 0x43,
        UAIR = 0x44,
        DAIR = 0x45,
        NAIR_LANDING  = 0x46,
        FAIR_LANDING  = 0x47,
        BAIR_LANDING  = 0x48,
        UAIR_LANDING  = 0x49,
        DAIR_LANDING  = 0x4a,
        DAMAGE_HIGH_1 = 0x4b,
        DAMAGE_HIGH_2 = 0x4c,
        DAMAGE_HIGH_3 = 0x4d,
        DAMAGE_NEUTRAL_1 = 0x4e,
        DAMAGE_NEUTRAL_2 = 0x4f,
        DAMAGE_NEUTRAL_3 = 0x50,
        DAMAGE_LOW_1 = 0x51,
        DAMAGE_LOW_2 = 0x52,
        DAMAGE_LOW_3 = 0x53,
        DAMAGE_AIR_1 = 0x54,
        DAMAGE_AIR_2 = 0x55,
        DAMAGE_AIR_3 = 0x56,
        DAMAGE_FLY_HIGH = 0x57,
        DAMAGE_FLY_NEUTRAL = 0x58,
        DAMAGE_FLY_LOW = 0x59,
        DAMAGE_FLY_TOP = 0x5a,
        DAMAGE_FLY_ROLL = 0x5b,
        SHIELD_START = 0xb2,
        SHIELD = 0xb3,
        SHIELD_RELEASE = 0xb4,
        SHIELD_STUN = 0xb5,
        SHIELD_REFLECT = 0xb6,
        GRAB = 0xd4,
        GRAB_RUNNING = 0xd6,
        ROLL_FORWARD = 0xe9,
        ROLL_BACKWARD = 0xea,
        SPOTDODGE = 0xEB,
        AIRDODGE = 0xEC,
        EDGE_TEETERING_START = 0xF5, //Starting of edge teetering
        EDGE_TEETERING = 0xF6,
        SLIDING_OFF_EDGE = 0xfb, 
        EDGE_CATCHING = 0xFC, 
        EDGE_HANGING = 0xFD,
        EDGE_GETUP_SLOW = 0xFE,  // >= 100% damage
        EDGE_GETUP_QUICK = 0xFF, // < 100% damage
        EDGE_ATTACK_SLOW = 0x100, // < 100% damage
        EDGE_ATTACK_QUICK = 0x101, // >= 100% damage
        EDGE_ROLL_SLOW = 0x102, // >= 100% damage
        EDGE_ROLL_QUICK = 0x103, // < 100% damage
        ENTRY = 0x142,    //Start of match. Can't move
        ENTRY_START = 0x143,    //Start of match. Can't move
        ENTRY_END = 0x144,    
        NEUTRAL_B_CHARGING = 0x156,
        NEUTRAL_B_ATTACKING = 0x157,
        NEUTRAL_B_CHARGING_AIR = 0x15A,
        NEUTRAL_B_ATTACKING_AIR = 0x15B,
        DOWN_B_GROUND_START = 0x168,
        DOWN_B_GROUND = 0x169,
        DOWN_B_STUN = 0x16d, 
        DOWN_B_AIR = 0x16e,
        UP_B_GROUND = 0x16f,
        UP_B = 0x170,    
    
    };
    //347 falcon punch

};



#endif
