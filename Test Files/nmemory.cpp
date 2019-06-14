#include "MemoryScanner.h"
#include "Navigation.h"
#include "Controller.h"
#include "Player.h"
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <pwd.h>


int main(){

	struct passwd *pw = getpwuid(getuid());
	std::string p1 = pw->pw_dir;
	std::string p = pw->pw_dir;
	std::string path;
	std::string pipepath;

	path = p1 + "/.dolphin-emu/";
	pipepath = p + "/.dolphin-emu/Pipes/pipe";
	puts("pipe path is : ");
	puts(pipepath.c_str());
	
	MemoryScanner mem = MemoryScanner(path);
	
	Controller cont = Controller(false);
	/*cont.CreateFifo(pipepath, 1);
	cont.OpenController();*/
	Navigation nav = Navigation(cont, &mem);
	int i =0;
	for(;;){

		nav.FindPos();

		mem.UpdatedFrame(false);

		Player p1 = mem.GetPlayer(true);
		printf("CURRENT MENU %d:\n", p1.current_menu);
		
	}


}

