#ifndef _PLAYER_H
#define _PLAYER_H
#include <stdint.h>
typedef uint32_t uint32;

// Player structure
struct Player{
    // Statics:
    float s_facing = 0;
    short percent = 0;
    int falls = 0;
    short stocks = 0;
    float TDR = 0; // Total Damage Received
    float TDG = 0; // Total Damage Given

    // Character entity data - Action Information
    uint32 action_state = 0;
    float action_frame_count = 0;
    uint32 body_state = 0; // 0 = normal, 1 = invulnerable, 2 = invisible
    uint32 smash_state = 0; //2 = charging smash, 3 = attacking with charged smash, 0 = all other times
    short smash_frame_counter = 0; //default = 0x42700000 = 60 to launch

    // Character entity data - Physics information
    float facing = 0;
    // Self induced
    float self_h_velocity = 0;
    float self_v_velocity = 0;
    // Attack induced
    float att_h_velocity = 0;
    float att_v_velocity = 0;
    // Positioning
    float pos_x = 0; // Left is negative, Right is Positive, 0 is center
    float pos_y = 0; // Only Active while in the air (thus init to a sane spot)
    float delta_x = 0;
    float delta_y = 0;

    bool onGround = 0; //0x00000000 on ground, 0x00000001 in air

    short times_jumped = 0;

    float cursor_x = 0;
    float cursor_y = 0;
};

#endif