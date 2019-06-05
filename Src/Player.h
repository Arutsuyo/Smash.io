#ifndef _PLAYER_H
#define _PLAYER_H

struct Player{

	unsigned int health;
	int dir; //left -1 right 1
	float pos_x;	
	float pos_y;
	float cursor_x;
	float cursor_y;
};

#endif