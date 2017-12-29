#ifndef PPU_H
#define PPU_H
extern "C"
{
    #include <SDL2/SDL.h>
}

#include <stdio.h>
#include <map>

//PPU memory ranges from 0x0-0x3FFF

//current priority:
//render a frame using the ppu, verify writes from CPU are correct.
//SDL and openGL.
//after testing and verifying, implement memory mappers.
//after this implement threads and timing verification.

//finally implement sound.
class Processor;

//set up vblank line to prcessor interrupt.

class PPU{

	protected:
		//PPU registers.
		unsigned char PPUCTRL;	//0x2000
		unsigned char PPUMASK;	//0x2001
		unsigned char PPUSTATUS;//0x2002
		unsigned short PPUSCROLLQ1,PPUSCROLLQ2;
		unsigned char OAMADDR,OAMREST;	//0x2003
		unsigned char OAMDATA;	//0x2004
		//unsigned char PPUSCROLLX;//0x2005
		//unsigned char PPUSCROLLY;
		unsigned char PPUADDR;	//0x2006
		unsigned char PPUDATA;	//0x2007
		unsigned short OAMDMA;	//0x4014
		unsigned char addressReset;	//To deal with the PPU address set returning the value one later.
		unsigned short currentAddress;
		unsigned char mirroring;
		unsigned short addressOffset,tempAddress;
		unsigned char writeToggle,fineX;
		
		
		Processor *cpu;
		unsigned char *vRam;
		unsigned char *spriteRam;
		unsigned char *secondOAM;
		unsigned char ioLine;
		//unsigned short currentScanline;
		
//palate is global, set up palate table next in order to add the proper colors, then start the rendering pipeline, verify the proper writes in the setup of the rom. size is 0x40.
		std::map<unsigned char,unsigned long> Colors;
		
		SDL_Window *window;
	public:
		unsigned int dma,fromCPU;
		unsigned short currentScanline;
		PPU(Processor *pro);
		~PPU();	
		//262 scalines * 341 per scanline. 1CPU Cycle=3 PPU cycles.
		unsigned char readRegister(unsigned char reg);
		void setRegister(unsigned char reg, unsigned char value);
		signed char execute(unsigned long cycles);
		
		
		unsigned char executePPU(unsigned short cycles);
		void executeBack();
		
		
		//set the mirroring for the game.
		void setMirroring(unsigned char c);
		
		void render();
		unsigned char attributeFind(unsigned char addOff,unsigned char attribute);
		void printVRAM(unsigned short offset,unsigned short bounds);
		void setPatternTable(unsigned char *buffer);
		void writeToVRAM(unsigned short address, unsigned char value);
		unsigned char readFromVRAM(unsigned short address);
		void writeToSRAM(unsigned short address, unsigned char value);
		unsigned char readFromSRAM(unsigned short address);
		void SRAMDMA(unsigned char *memory);
		int VBlank();
};

#endif //PPU_H