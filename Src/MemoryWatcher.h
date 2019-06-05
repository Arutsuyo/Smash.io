#ifndef _MemoryWatcher_H
#define _MEMORYSCANNER_H
#include <string>
#include "Player.h"
#include "GameState.h"

class MemoryWatcher {

	Player p1;
	Player p2;

	int m_file;

	std::string userPath;

	bool init_socket();

	bool in_game;


public:
	bool success = false;

	// We load into Character select, we want to detect when that happens
	int current_Menu = MENU::POSTGAME_SCORES;

	// We're not tracking enough to play anywhere else but Final Destination 
	// so we wanna know when that loads
	int current_stage = STAGE::YOSHI_STORY;

	// 0: Player1 1: Player2
	Player GetPlayer(bool pl);
	bool ReadMemory(bool prin);
	bool print();

	MemoryWatcher(std::string s);
	~MemoryWatcher();


};

#endif