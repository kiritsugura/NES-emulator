#include "Processor.h"
#include "PPU.h"
#include "APU.h"
	//9246
#define CYC 19250
#define range 20
//#include <time.h>

#include <inttypes.h>
#include <stdint.h>
//#include <stdio.h>
#include <sys/time.h>
#include <string>
//#include <iostream>
//#include <fstream>

//about 559 nano per cycle

#define ClockRate 1789773

//about 60fps.
#define RefreshRate 16


static int rate=ClockRate/60;
uint64_t timeusec(){
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1E6) + time.tv_usec;
}


//Verify:
//The timing of the PPU with respect to the CPU.


//To-Do:
//Set up sound timing
//SFML
//sound

//Research other mappers.
//input and bindings
//nice gui and save states.

int main(int argc,char **argv){
	if(argc!=2){
		printf("Incorrect usage: NES ROM_NAME");
		exit(5);
	}
	std::string s=argv[1];
	Processor Pro=Processor(("ROMS/"+s).c_str());
	
	
	
	int index=0;
	PPU *picture=Pro.getPPU();
	//set up proper startup state in PPU and Processor.
	unsigned long cycles=0,resets=0,pcyc;
	unsigned char exits,hold=0;
	int i=0;
	
	int started=0;
	//16000
	uint64_t current,frametime=rate;
	SDL_Event event;
	for(;;){

		//*
		current=timeusec();
		//|| picture->VBlank()==0
		//cycles<=rate ||
		//while(cycles<=rate || (picture->VBlank()!=0 && started!=1) ){
		int c=Pro.execute6502Command()+Pro.extraCycles;
		picture->executePPU(c*3);			
		started=picture->VBlank();
		//if(picture->VBlank()==0 && started==1){
			
		//}
		cycles+=c;
		while(SDL_PollEvent(&event)){
			if(event.type==SDL_QUIT){
				exits=1;
			}else{
				Pro.setInput(&event);
			}
		}
		if(cycles>=rate && picture->VBlank()==1){
			cycles-=rate;
			frametime=timeusec();
			picture->render();			
		}
		//}
		//cycles-=rate;
		//frametime=timeusec();
		//do a busy loop while we wait... fix later. can do about 400-600 frames per second if not writing to the pixels.
		//while((frametime-current)<16666){
		//	frametime=timeusec();
		//}
		//printf("Frame %i\n",picture->currentScanline);
		//picture->render();
		
		if(exits&1)
			break;
		//*/
	}
}
