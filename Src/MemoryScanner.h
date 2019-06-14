#ifndef _MEMORYSCANNER_H
#define _MEMORYSCANNER_H
#include <string>
#include "Player.h"
#include "Controller.h"

class MemoryScanner {

	Player p1;
	Player p2;

	int socketfd = -1;

	std::string userPath;

	bool init_socket();

	bool in_game;

	int current_stage = -1;

	//helper function for debugging to display player actions
	void display_player_actions(Player p);

public:
	bool success = false;

	// 0: Player1 1: Player2
	Player GetPlayer(bool pl);
	bool print();

	int CurrentStage() { return this->current_stage; }

	// Read a DGRAM from Dolphin. This will return:
	// 1 on a successful parse
	// 0 Nothing in the socket
	// -1 error
	int UpdatedFrame();

	MemoryScanner(std::string s);
	~MemoryScanner();


};

#endif