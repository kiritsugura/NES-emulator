#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <string>
#include <inttypes.h>
//#include "SDL.h"
#include "debug.h"
#include <iostream>
#include <fstream>

#include "APU.h"

class PPU;

#define Absolute(memory,opcode) (readFromMemory(*(opcode+1),*(opcode+2)))
#define Zerop(memory,opcode) (readFromMemory(*(opcode+1),0))
#define AbsoluteI(memory,opcode,reg) (readFromMemory((*(opcode+1)+reg), *(opcode+2)+((*(opcode+1)+reg)>0xff)))
#define ZeropI(memory,opcode,X) (readFromMemory((*(opcode+1)+X)&0xff,0))
#define Branch(address,opcode) (((opcode)<0x80) ? ((address)+(opcode)): ((address)-((~(opcode)+1)&0xff)))
//pre-indexing
#define IndexedI(memory,opcode,X) (readFromMemory(memory[(*(opcode+1)+X)&0xff],memory[(*(opcode+1)+X+1)&0xff]))
#define IndexedAdr(memory,opcode,X) ((readFromMemory((*(opcode+1)+X+1)&0xff,0)<<8)| readFromMemory((*(opcode+1)+X)&0xff,0))
//post-indexing
//#define IndirectI(memory,opcode,Y) (readFromMemory(  (memory[*(opcode+1)]+Y) + (memory[(*(opcode+1)+1)]   )))
#define IndirectAdr(memory,opcode,Y) (memory[*(opcode+1)] +Y+ (memory[(*(opcode+1)+1)&0xff]<<8) )
//check the above clock cycles again.
#define carry(res) (this->f.Carry=(((res)>0xff)) ? 1:0)
#define carryR(res) (this->f.Carry=((res)&0x1))
#define carryCMP(register,other) (this->f.Carry=(register>=other))
//Negative and Zero both work correctly
#define zero(res) (this->f.Zero=((res)==0))
#define negative(res) (this->f.Negative= ((res)&0x80)>0)
#define over(res,org,additional) (this->f.Overflow=(((additional)&0x80)==((org)&0x80) && ((res)&0x80)!=((org)&0x80))) 

extern "C"
{
    #include <SDL2/SDL.h>
}

typedef struct ProgramStatus{
	unsigned char Carry:1;
	unsigned char Zero:1;
	unsigned char InterruptDisable:1;
	unsigned char DecimalMode:1;	//binary coded decimal mode,nes does not support this mode, so it lacks functionality(2A03).
	unsigned char Break:1;			//break and fire an irq.
	unsigned char Extra:1;
	unsigned char Overflow:1;
	unsigned char Negative:1;
}flags;

class Processor{
	protected:
		unsigned char A;
		unsigned char X;	//Index register.
		unsigned char Y;	//Index register.
		///offset from 0x100
		///offset is 0x100+StackPointer, no wrap checked.
		///set to 0xff at the start.
		unsigned char StackPointer;
		unsigned char *memory;
		unsigned short ProgramCounter;
		//1 is startup, 0 is ready.
		flags f;
		PPU *pictureProcessing;
		uint8_t controllerButtons,controller1Input;
		unsigned int mapNum;
		uint8_t *buffer;
		int Debug;
			
	public:
		//for DMA and such...
		unsigned short extraCycles;
		int sentToPPU;
		Processor(std::string filename);
		~Processor();		
		unsigned char execute6502Command();
		
		int test(std::ofstream *t);
		
		void setInput(SDL_Event *event);
		PPU* getPPU();
		void printCPU();
		unsigned short readExtra();
		void printMemory(unsigned short offset,unsigned short bounds);
		void printMemoryRange(unsigned short offset,unsigned short range);
		void writeToMemory(unsigned char addressL,unsigned char addressH,unsigned char value);
		unsigned char readFromMemory(unsigned char addressL,unsigned char addressH);
		unsigned char readFromMemory(unsigned short address);
		int Interrupt(char type);
		//Returns a pointer to a specified address in memory.
		unsigned char* requestMemory(unsigned short address);
};


#endif //PROCESSOR_H