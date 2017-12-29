#include "Processor.h"
//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include "PPU.h"

#undef DEBUG_H


//balance cycles waiting for PPU about 29658 CPU cycles.

//printing CHRROM for the pattern tables.

//look into status of Break flag during BRK and RTI.
//Look into the 'effective' cycles on Read vs Write.
//Issue with 6502 with JMP instruction at page boundaries, 0xff+1 goes to 0x00 instead of 0x100 etc.


//verify branch behaviors, SBC possibly as well, interrupts.
//work on IRQ, NMI, RESET
/*
//Implied Addressing- 1 byte, 2 Clock Cycles: clear set bits, increment decrement and transfer between internal registers, Stack related Implied can be 3 Cycles.
//Immediate Addressing- 2 byte instruction(opcode, constant value), **find cycles amount**
//Absolute Addressing- 3 byte instruction(opcode,lowAddress,highAddress), Check cycles. 4 cycles, jump absolute is 3.
//Accumulator- occurs on the accumulator.
//Zero Page Addressing- 2 byte instruction(opcode, address in page zero of memory), address High is 00, low is byte 2, used to get RAM addresses 0-ff.
//Relative Addressing- 2 byte instruction(opcode, offset), The address specified is relative to the current one as a signed char ,2 3 or 4 cycles Only used in branches.
//Absolute Index Addressing- 3 byte instruction(opcode, lowAddress, highAddress), Similar to Absolute, but index register is added onto address.
//Zero Page Indexing- 2 byte instruction, one cycle longer than Zero Page.
//Indirect Addressing- 2 byte instruction(opcode, zeroPageAddress), specifies a where the low value of an address is in Zero Page, high is that++, value is then loaded from it.
//Indexed Indirect- 2 byte(opcode, zeroPageAddress), uses X, same as above but adds X to the zeroPageAddress, 6 cycles.
//Indirect Indexed- 2 byte(opcode,zeroPageAddress), uses Y, similar to above except goes through values in lowAddress+Y.
//Indirect Absolute- used for jump.  

///NOTE during an interrupt, the contents of the processor are automatically saved onto the stack.
///check for the wrapping of values
*/

/*Tested opcodes(cycles not tested):
	      0x01, 0x05, 0x06, 0x07, 0x09, 0x0A, 0x0D, 0x0E,
		  0x10, 0x11, 0x15, 0x16, 0x18, 0x19, 0x1D, 0x1E,
		  0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2A, 0x2C, 0x2D, 0x2E, 
		  0x30, 0x31, 0x35, 0x36, 0x38, 0x39, 0x3D, 0x3E, 
		  0x40, 0x41, 0x45, 0x45, 0x46, 0x48, 0x49, 0x4A, 0x4C, 0x4D, 0x4E, 
		  0x50, 0x51, 0x55, 0x56, 0x58, 0x59, 0x5D, 0x5E,
		  0x60, 0x61, 0x65, 0x66, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E,
		  0x70, 0x71, 0x75, 0x76, 0x78, 0x79, 0x7D, 0x7E,
		  0x81, 0x84, 0x85, 0x86, 0x88, 0x8A, 0x8C, 0x8D, 0x8E, 
		  0x90, 0x91, 0x94, 0x95, 0x96, 0x98, 0x99, 0x9A, 0x9D,
		  0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6, 0xA8, 0xA9, 0xAA, 0xAC, 0xAD, 0xAE,
		  0xB0, 0xB1, 0xB4, 0xB5, 0xB6, 0xB8, 0xB9, 0xBA, 0xBC, 0xBD, 0xBE, 
		  0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC8, 0xC8, 0xC9, 0xCA, 0xCC, 0xCD, 0xCE, 
		  0xD0, 0xD1, 0xD5, 0xD6, 0xD8, 0xD9, 0xDD, 0xDE, 
		  0xE0, 0xE1, 0xE4, 0xE4, 0xE5, 0xE6, 0xE8, 0xE9, 0xEC, 0xED, 0xEE, 
		  0xF0, 0xF1, 0xF5, 0xF6, 0xF8, 0xF9, 0xFD, 0xFE
*/

//smb mapper is 0.



PPU* Processor::getPPU(){
	return pictureProcessing;
}

Processor::Processor(std::string filename){
	this->memory=new unsigned char[0x10000];
	memset(memory,0,0x10000);
	extraCycles=0;
	controllerButtons=0;
	this->sentToPPU=0;
	this->A=0x0;
	this->X=0x00;
	this->Y=0x00;
	this->StackPointer=0x00;
	this->ProgramCounter=0x34;
	//this->ProgramCounter=0x24;
	this->f=*((flags*)&this->ProgramCounter);
	this->pictureProcessing=new PPU(this);
	Debug=0;
	#ifdef DEBUG_H
		printf("Filename is: %s\n\n",filename.c_str());
	#endif
	FILE *file=fopen(filename.c_str(),"rb");
    if(file==NULL){
        printf("Can not open file.");
		exit(0);
	}
    fseek(file,0L,SEEK_END);
    uint64_t fileSize=ftell(file);
    fseek(file,0L,SEEK_SET);
    buffer=new uint8_t[fileSize];

    if(buffer==NULL)
        exit(2);
    fread(buffer,fileSize,1,file);
    fclose(file);
	//printf("Header 0x%02X %02X %02X %02X %02X %02X %02X %02X %08X\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7], *(int*)((void*)buffer));
	if(*(int*)((void*)buffer)==0x1A53454E){
		printf("NES file, proceeding.\n");
		//loading the header info.
		unsigned char PGRROM=buffer[4];
		unsigned char CHRROM=buffer[5];
		unsigned char flag6=buffer[6];
		unsigned char flag7=buffer[7];
		unsigned char flag8=buffer[8];
		unsigned char flag9=buffer[9];
		this->mapNum=((flag7&0xf0)<<4)|((flag6&0xf0)>>4);
		//#ifdef DEBUG_H
			//if((flag7&0x4)!=0x4){
				//int index=4;
				//while(index<15){
					//printf("Byte %i:0x%04X\n",index,buffer[index]);
					//++index;
				//}
				if(CHRROM!=0){
					printf("PGRROM size: 0x%04X x 16KB\nCHRROM size:0x%04X x 8KB\nPGRRAM size: 0x%04X x 8KB\n",PGRROM, CHRROM, flag8);
				}else{
					printf("PGRROM size: 0x%04X x 16KB\nCHRRAM\nPGRRAM size: 0x%04X x 8KB\n",PGRROM, flag8);
				}
				printf("Flag6: 0x%04X\nFlag7: 0x%04X\nFlag8: 0x%04X\nFlag9: 0x%04X\n",flag6,flag7,flag8,flag9);
				printf("PGRROM is %i and ",PGRROM*16384);
				printf("CHRROM is %u, remaining amount is %u\n",(CHRROM*8192),(fileSize-(PGRROM*16384+CHRROM*8192+16)));
				printf("\nFile size is %u\n",fileSize);
			//}
			//else{
			//	printf("NES 2.0 format:");
			//	mapNum=((flag8&0xf0)<<8)|((flag7&0xf0)<<4)|(flag6&0xf0);
			//	printf("Mapper variant: 0x%01X\n",flag8&0xf);
			//}
		//#endif
		if(mapNum==0 && PGRROM==2){
			#ifdef DEBUG_H
				printf("Mapper 0\n");
			#endif
			memcpy((memory+0x8000),(buffer+16),PGRROM*16384);
			//rom is loaded at this point.
		}else if(mapNum==0 && PGRROM==1){
			#ifdef DEBUG_H
				printf("Mapper 0.1\n");
			#endif
			memcpy((memory+0x8000),(buffer+16),16384);
			memcpy((memory+0xC000),(buffer+16),16384);
		}
		else if(mapNum==2){
			//#ifdef DEBUG_H
				printf("Mapper 2\n");
				//printf("%i\n",(16+(16384*(PGRROM-1))));
				memcpy((memory+0x8000),(buffer+16),16384);
				memcpy((memory+0xC000),(buffer+16+(16384*(PGRROM-1))),16384);
			//#endif	
			//printf("Done\n");
			//exit(0);			
			
		}else{
			if((flag7&0x4)==0x4){
				printf("NES 2.0 format:");
				
				
			}
			printf("Mapper %i not supported\n",mapNum);
			exit(0);
		}
		//Check pattern table.
		#ifdef DEBUG_H
			printf("CHR ROM:\n");
			unsigned short i=0;
			for(;i<0x2000;++i){
				binary((0x8000+i),buffer[16+PGRROM*16384+i],(0x8000+i+0x8),buffer[16+PGRROM*16384+0x8+i]);
				if((i&0x7)==7){	
					printf("\n\n");
					i+=8;
				}
			}
		#endif
		if(CHRROM!=0){
			pictureProcessing->setPatternTable((buffer+16+PGRROM*16384));
		}
		if((flag6&0x2)){
			pictureProcessing->setMirroring(flag6&0x2);
		}else{
			pictureProcessing->setMirroring(flag6&0x1);
		}
		//printf("After pattern table: 0x%04X\n",(fileSize-(PGRROM*16384+0x2000+0x10)));
//CM
		//set ppu flags and load proper items into memory here.
	}else{
		exit(3);
	}
	this->Interrupt(1);

}

void Processor::setInput(SDL_Event *event){
	unsigned short scancode;
	//Input returned in the form A,B,Select,Start,Up,Down,Left,Right.
	switch(event->type){
		case SDL_KEYDOWN:{
			scancode=event->key.keysym.scancode;
			//hardcoded to test.
			switch(scancode){
				case SDL_SCANCODE_A:
					//printf("A\n");
					controllerButtons|=0x2;
					break;
				case SDL_SCANCODE_W:
					controllerButtons|=0x8;
					break;
				case SDL_SCANCODE_S:
					controllerButtons|=0x4;
					break;
				case SDL_SCANCODE_D:
					controllerButtons|=0x1;
					break;
				case SDL_SCANCODE_SPACE:
					controllerButtons|=0x80;
					break;				
				case SDL_SCANCODE_E:
					controllerButtons|=0x40;
					break;
				case SDL_SCANCODE_G:
					controllerButtons|=0x20;
					break;
				case SDL_SCANCODE_H:
					controllerButtons|=0x10;
					break;
				case SDL_SCANCODE_L:
					Debug=1;
					break;
				default:
					break;
			}
		break;
		}
		case SDL_KEYUP:{
			scancode=event->key.keysym.scancode;
			switch(scancode){
				case SDL_SCANCODE_A:
					controllerButtons&=~0x2;
					break;
				case SDL_SCANCODE_W:
					controllerButtons&=~0x8;
					break;
				case SDL_SCANCODE_S:
					controllerButtons&=~0x4;
					break;
				case SDL_SCANCODE_D:
					controllerButtons&=~0x1;
					break;
				case SDL_SCANCODE_SPACE:
					controllerButtons&=~0x80;
					break;				
				case SDL_SCANCODE_E:
					controllerButtons&=~0x40;
					break;
				case SDL_SCANCODE_G:
					controllerButtons&=~0x20;
					break;
				case SDL_SCANCODE_H:
					controllerButtons&=~0x10;
					break;
				case SDL_SCANCODE_L:
					Debug=0;
					break;
				default:
					break;
			}
		}
		break;
		default:
		break;
	}
}

Processor::~Processor(){
	delete buffer;
	delete memory;
	delete pictureProcessing;
}

unsigned char* Processor::requestMemory(unsigned short address){
	return (address>0x00 && address<0x10000) ? (memory+address):memory;
}

void Processor::printCPU(){
	printf("CPU State:\nA:\t0x%02X\nX:\t0x%02X\nY:\t0x%02X\nSP:\t0x%02X\nPG:\t0x%04X\n",this->A,this->X,this->Y,this->StackPointer,this->ProgramCounter);
	printf("Flags:\n\tCarry:\t%01X\n\tZero:\t%01X\n\tID:\t%01X\n\tDM:\t%01X\n\tBreak:\t%01X\n\tExtra:\t%01X\n\tV:\t%01X\n\tNeg:\t%01X\n",
	        this->f.Carry,this->f.Zero,this->f.InterruptDisable,this->f.DecimalMode,this->f.Break,this->f.Extra,this->f.Overflow,this->f.Negative);
}

void Processor::printMemoryRange(unsigned short offset,unsigned short range){
	int i=0;
	printf("Memory:\n");
	for(i=offset;i<range;++i){
		printf("  0x%04X:\t0x%02X\n",i,this->memory[i]);
	}
}

void Processor::printMemory(unsigned short offset,unsigned short bounds){
	int i=0;
	printf("Memory:\n");
	for(i=offset;i<offset+bounds;++i){
		printf("  0x%04X:\t0x%02X\n",i,this->memory[i]);
	}
}

unsigned short Processor::readExtra(){
	unsigned short amount=extraCycles;
	extraCycles=0;
	return amount;
}

void Processor::writeToMemory(unsigned char addressL,unsigned char addressH,unsigned char value){
	unsigned short address=(addressH<<8) | (addressL);
	if(address < 0x2000){
		//write to normal memory.
		this->memory[address&0x7ff]=value;
		#ifdef DEBUG_H
			printf("Value of 0x%02X written to address 0x%04X.\n",value, address&0x7ff);
		#endif
	}else if(address<0x4000){
		//write to PPU.
		this->sentToPPU=1;
		pictureProcessing->setRegister((address&0x7),value);
		//this->sentToPPU=1;
		#ifdef DEBUG_H
			printf("Value of 0x%02X written to address 0x%04X.\n",value, address);
		#endif
	}else if(address < 0x8000){
		#ifdef DEBUG_H
			printf("Value of 0x%02X written to address 0x%04X.\n",value, address);
		#endif
		if(address>0x3FFF && address<0x4008){
			//printf("Pulse at 0x%04X\n",address);
			this->memory[address]=0;
		}else if(address<0x400B){
			//printf("Triangle at 0x%04X\n",address);
			this->memory[address]=0;
		}else if(address<0x400F){
			//printf("Noise at 0x%04X\n",address);
			this->memory[address]=0;
		}else if(address<0x4013){
			//printf("DMC at 0x%04X\n",address);
			this->memory[address]=0;
		}else if(address==0x4014){	//OAMDMA
			pictureProcessing->SRAMDMA((&memory[0]+(value<<8)));
			pictureProcessing->setRegister(8,value);
			//this takes 512 cycles.
			extraCycles=512;
			//this->sentToPPU=1;
		}else if(address==0x4015){
			//printf("Option1 at 0x%04X\n",address);
			this->memory[address]=0;
		}else if(address==0x4016){
			controller1Input=value;
		}else if(address==0x4017){
			//printf("Option2 at 0x%04X\n",address);
			this->memory[address]=0;
		}else{
			this->memory[address]=value;
		}
		//IO more information needed.
	}else if(address >= 0x8000){
		switch(mapNum){
			case 2:{
				memcpy((memory+0x8000),(buffer+16+(16384*value)),0x4000);
				//printf("Bank %i\n",value);
				//exit(10);
				break;
			}default:{
				printf("This area cannot be written to 0x%04X.\n",address);
				//ROM cannot write here.
				exit(4);
			}
		}
	}
}
unsigned char Processor::readFromMemory(unsigned short address){
	return readFromMemory(address&0xff,(address&0xff00)>>8);
}

unsigned char Processor::readFromMemory(unsigned char addressL,unsigned char addressH){
	unsigned short address=(addressH<<8) | (addressL);
	if(address < 0x2000){
		#ifdef DEBUG_H
			printf("Value of 0x%02X read from address 0x%04X.\n",this->memory[address&0x7ff], address&0x7ff);
		#endif
		
		return this->memory[address];
		//read from normal memory.
	}else if(address<0x4000){
		this->sentToPPU=1;
		unsigned short value=pictureProcessing->readRegister(address&0x7);
		//read from PPU.
		#ifdef DEBUG_H
			printf("Value of 0x%02X read from address 0x%04X.\n",value, address);
		#endif
		
		return value;
	}else if(address < 0x8000){
		if(address==0x4014){
			printf("Cannot read from DMA\n");
			exit(9);
		}
		
		//Input returned in the form A,B,Select,Start,Up,Down,Left,Right.
		//player 1
		
		else if(address==0x4016){
			if(controller1Input&0x1){
				return 0xE0|((controllerButtons&0x80)>>7);
			}
			static char readNum=0;
			if(readNum==8){
				//controller1Input|=0x1;
				readNum=0;
			}
			return 0xE0|((controllerButtons&(1<<(7-readNum)))>>(7-readNum++));
		}
		//player 2
		else if(address==0x4017){
			if(controller1Input&0x1){
				return 0xE0|((controllerButtons&0x80)>>7);
			}
			static char readNum=0;
			if(readNum==8){
				readNum=0;
				//controller1Input|=0x1;
			}
			return 0xE0|((controllerButtons&(1<<(7-readNum)))>>(7-readNum++));
		}
		return memory[address];
		//IO more information needed.
		return this->memory[address];
	}else if(address > 0x8000){
		//printf("Read 0x%04X\n",address);
		//exit(4);
		return this->memory[address];
	}
	return 255;
}

//check for BRK and NMI overlaps.
//Assuming 0 is NMI, 1 is RST, 2 is IRQ.
int Processor::Interrupt(char type){
	switch(type){
		case 0:{
			#ifdef DEBUG_H
				printf("NMI\n");
			#endif
			//FFFA FFFB VBlank, deal with the ppu.
			this->f.Extra=1;
			writeToMemory(this->StackPointer--,0x1,(this->ProgramCounter&0xff00)>>8);
			writeToMemory(this->StackPointer--,0x1,this->ProgramCounter&0xff);
			writeToMemory(this->StackPointer--,0x1,*((char*)(&this->f)));
			this->ProgramCounter=(readFromMemory(0xfb,0xff)<<8)|readFromMemory(0xfa,0xff);
			break;
		}
		case 1:{
			#ifdef DEBUG_H
				printf("Reset interrupt\n");
			#endif
			//FFFC FFFD, reset appears to be called on startup.
			this->StackPointer-=3;
			this->ProgramCounter=(readFromMemory(0xfd,0xff)<<8)|readFromMemory(0xfc,0xff);
			break;
		}
		case 2:{
			if(this->f.InterruptDisable==1 && this->f.Break==0)
				return 0;
			#ifdef DEBUG_H
				printf("IRQ\n");
			#endif
			//FFFE FFFF, Peripheral or BRK.
			writeToMemory(this->StackPointer--,0x1,(this->ProgramCounter&0xff00)>>8);
			writeToMemory(this->StackPointer--,0x1,this->ProgramCounter&0xff);
			writeToMemory(this->StackPointer--,0x1,*((char*)(&this->f)));
			this->ProgramCounter=(readFromMemory(0xff,0xff)<<8)|readFromMemory(0xfe,0xff);
			this->f.Break=0;
			this->f.Extra=0;
			break;
		}
	}
	this->f.InterruptDisable=1;
	return 7;
}

static int flag=0;

//check stack placemnet instructions PHA and others
unsigned char Processor::execute6502Command(){
	unsigned char *opcode=&memory[this->ProgramCounter++];
	if(Debug){
		printf("PG:0x%04X\tA:0x%02X X:0x%02X Y:0x%02X P:0x%02X SP:0x%02X O:0x%02X %02X %02X\n",ProgramCounter-1,this->A,this->X,this->Y,(*(char*)(&this->f))&0xff,this->StackPointer,*opcode,*(opcode+1),*(opcode+2));		
	}
	#ifdef DEBUG_H 
		printf("PG:0x%04X\tA:0x%02X X:0x%02X Y:0x%02X P:0x%02X SP:0x%02X O:0x%02X %02X %02X\n",ProgramCounter-1,this->A,this->X,this->Y,(*(char*)(&this->f))&0xff,this->StackPointer,*opcode,*(opcode+1),*(opcode+2));
	#endif
	extraCycles=0;
	if(*opcode>=0x00 && *opcode<0x10){
		switch(*opcode){
			case 0x00:	/*BRK:Implied:			CYCLES:7 */{
				++this->ProgramCounter;
				this->f.Break=1;
				return Interrupt(2);
			}
			case 0x01:	/*ORA ($DD),X:Indexed Indirect:Z,N:		CYCLES:6*/{
				unsigned char result=this->A|IndexedI(memory,opcode,this->X);
				zero(result);
				negative(result);
				++this->ProgramCounter;
				this->A=result;
				return 6;
			}
				case 0x02:	/*NOP:(No instruction)*/	break;
				case 0x03:  /*NOP:(No instruction)*/	break;
				case 0x04:	/*NOP:(No instruction)*/ 	break;
			case 0x05:  /*ORA $DD:Zero Page:Z,N::		CYCLES:3*/{
				unsigned char result=this->A|Zerop(memory,opcode);
				zero(result);
				negative(result);
				++this->ProgramCounter;
				this->A=result;
				return 3;
			}
			case 0x06:	/*ASL $DD:Zero Page:C,Z,N:		CYCLES:5*/{
				unsigned short result=Zerop(memory,opcode)<<1;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				writeToMemory(*(opcode+1),0,result&0xff);
				++this->ProgramCounter;
				return 5;
			}
				case 0x07:	/*NOP:(No instruction)*/ break;
			case 0x08: 	/*PHP:Implied:		CYCLES:3*/{	
				this->f.Break=1;
				this->f.Extra=1;
				writeToMemory(this->StackPointer--,0x1,*((char*)(&this->f)));
				this->f.Break=0;
				this->f.Extra=0;
				return 3;
			}
			case 0x09: 	/*ORA #$DD:Immediate:Z,C:():		CYCLES:2*/{
				unsigned char result=this->A|*(opcode+1);
				zero(result);
				negative(result);
				++this->ProgramCounter;
				this->A=result;
				return 2;
			}
			case 0x0A: 	/*ASL A:Accumulator:C,Z,N:		CYCLES:2*/{
				unsigned short result=this->A<<1;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				this->A=result&0xff;
				return 2;
			}	
				case 0x0B:  /*NOP:(No instruction)*/	break;
			case 0x0C: 	/*NOP:(No instruction)*/	{
				this->ProgramCounter+=2;
				return 4;
			}
			case 0x0D: 	/*ORA $DDDD:Absolute:Z,N::		CYCLES:4*/{
				unsigned char result=this->A|Absolute(memory,opcode);
				zero(result);
				negative(result);
				this->ProgramCounter+=2;
				this->A=result;
				return 4;
			}
			case 0x0E: 	/*ASL $DDDD:Absolute:C,Z,N:		CYCLES:6*/{
				unsigned short result=Absolute(memory,opcode)<<1;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),result&0xff);
				return 6;
			}
				case 0x0F:  /*NOP:(No instruction)*/	break;
		}
	}
	else if(*opcode>=0x10 && *opcode<0x20){
		switch(*opcode){
			case 0x10: /*BPL $DD:Relative:		CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Negative==0){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0x11: /*ORA ($DD),Y:Indirect Indexed:Z,N::		CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				unsigned char result=this->A|readFromMemory(addr&0xff,(addr&0xff00)>>8);
				zero(result);
				negative(result);
				++this->ProgramCounter;
				this->A=result;
				return 5+(((addr-this->Y)&0xff)>(addr&0xff));
			}
				case 0x12: /*NOP(No instruction)*/	break;
				case 0x13: /*NOP(No instruction)*/	break;
				case 0x14: /*NOP(No instruction)*/	break;
			case 0x15: /*ORA $DD,X:Zero Page, X:Z,N::		CYCLES:4*/{
				unsigned char result=this->A|ZeropI(memory,opcode,this->X);
				zero(result);
				negative(result);
				++this->ProgramCounter;
				this->A=result;
				return 4;
			}
			case 0x16: /*ASL $DD,X:Zero Page, X:C,Z,N:		CYCLES:6*/{
				unsigned short result=ZeropI(memory,opcode,this->X)<<1;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory((*(opcode+1)+this->X)&0xff,0,result&0xff);
				return 6;
			}
				case 0x17: /*NOP(No instruction)*/	break;
			case 0x18: /*CLC:Implied:C:			CYCLES:2*/{
				this->f.Carry=0;
				return 2;
			}
			case 0x19: /*ORA $DDDD,Y:Absolute, Y:Z,N::		CYCLES:4(+1)*/{
				unsigned char result=this->A|AbsoluteI(memory,opcode,this->Y);
				zero(result);
				negative(result);
				this->ProgramCounter+=2;
				this->A=result;
				return 4+(*(opcode+1)+this->Y>0xff);
			}
				case 0x1A: /*NOP(No instruction)*/ return 2;
	
				case 0x1B: /*NOP(No instruction)*/	break;
				case 0x1C: /*NOP(No instruction)*/	this->ProgramCounter+=2; return 5;
			case 0x1D: /*ORA $DDDD,X:Absolute, X:Z,N::		CYCLES:4(+1)*/{
				unsigned char result=this->A|AbsoluteI(memory,opcode,this->X);
				zero(result);
				negative(result);
				this->ProgramCounter+=2;
				this->A=result;
				return 4+((*(opcode+1)+this->X)>0xff);
			}
			case 0x1E: /*ASL $DDDD,X:Absolute, X:C,Z,N:		CYCLES:7*/{
				unsigned short result=AbsoluteI(memory,opcode,this->X)<<1;
				unsigned short address=(*(opcode+1)|(*(opcode+2)<<8))+this->X;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(address&0xff,(address&0xff00)>>8,result&0xff);
				return 7;
			}
				case 0x1F: /*NOP(No instruction)*/	break;
		}		
	}
	else if(*opcode>=0x20 && *opcode<0x30){
		switch(*opcode){
			case 0x20: /*JSR $DDDD:Absolute:		CYCLES:6*/{
				writeToMemory(this->StackPointer--,0x1,((this->ProgramCounter+1) & 0xff00)>>8);
				writeToMemory(this->StackPointer--,0x1,(this->ProgramCounter+1) & 0xff);
				this->ProgramCounter=(*(opcode+2)<<8) | *(opcode+1);
				return 6;
			}
			case 0x21: /*AND $DD,X:Indexed Indirect:Z,N:		CYCLES:6*/{
				this->A&=IndexedI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;		
				return 6;
			}
				case 0x22: /*NOP(No instruction)*/	break;
				case 0x23: /*NOP(No instruction)*/	break;
			case 0x24: /*BIT $DD:Zero Page:Z,V,N:		CYCLES: 3*/{
				char value=Zerop(memory,opcode);
				zero(value&this->A);
				this->f.Negative=(value&0x80)>>7;
				this->f.Overflow=(value&0x40)>>6;
				++this->ProgramCounter;
				return 3;
			}
			case 0x25: /*AND $DD:Zero Page:Z,N:		CYCLES:3*/{
				this->A&=Zerop(memory,opcode);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 3;
			}
			case 0x26: /*ROL $DD:Zero Page:C,Z,N:		CYCLES:5*/{
				unsigned short result=(Zerop(memory,opcode)<<1)|this->f.Carry;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,result&0xff);
				return 5;
			}
				case 0x27: /*NOP(No instruction)*/	break;
			case 0x28: /*PLP:Implied:C,Z,I,D,B,V,N:		CYCLES:4*/{
				char flg=readFromMemory(++this->StackPointer,0x1);
				this->f=*((flags*)(&flg));
				this->f.Break=0;
				this->f.Extra=1;
				return 4;
			}
			case 0x29: /*AND #$NN:Immediate:Z,N:		CYCLES:2*/{
				this->A&=*(opcode+1);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 2;
			}
			case 0x2A: /*ROL A:Accumulator:C,Z,N:		CYCLES:2*/{
				unsigned short result=(this->A<<1)|this->f.Carry;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				this->A=result&0xff;
				return 2;
			}
				case 0x2B: /*NOP(No instruction)*/	break;
			case 0x2C: /*BIT $DDDD:Absolute:Z,V,N:		CYCLES: 4*/{
				char value=Absolute(memory,opcode);
				zero(value&this->A);
				this->f.Negative=(value&0x80)>>7;
				this->f.Overflow=(value&0x40)>>6;
				this->ProgramCounter+=2;
				return 4;
			}
			case 0x2D: /*AND $DDDD:Absolute:Z,N:		CYCLES:4*/{
				this->A&=Absolute(memory,opcode);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0x2E: /*ROL $DDDD:Absolute:C,Z,N:		CYCLES:6*/{
				unsigned short result=(Absolute(memory,opcode)<<1)|this->f.Carry;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),result&0xff);
				return 6;
			}
				case 0x2F: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0x30 && *opcode<0x40){
		switch(*opcode){
			case 0x30: /*BMI $DD:Relative:		CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Negative){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0x31: /*AND ($DD),Y:Indirect Indexed:Z,N:		CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				unsigned char result=this->A&readFromMemory(addr&0xff,(addr&0xff00)>>8);
				zero(result);
				negative(result);
				++this->ProgramCounter;
				this->A=result;
				return 5+(((addr-this->Y)&0xff)>(addr&0xff));
			}
				case 0x32: /*NOP(No instruction)*/	break;
				case 0x33: /*NOP(No instruction)*/	break;
				case 0x34: /*NOP(No instruction)*/	break;
			case 0x35: /*AND $DD,X:Zero Page,X:Z,N:		CYCLES:4*/{
				this->A&=ZeropI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 4;
			}
			case 0x36: /*ROL $DD,X:Zero Page,X:C,Z,N:		CYCLES:6*/{
				unsigned short result=(ZeropI(memory,opcode,this->X)<<1)|this->f.Carry;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory((*(opcode+1)+this->X)&0xff,0,result&0xff);
				return 6;
			}
			case 0x37: /*NOP(No instruction)*/	break;
			case 0x38: /*SEC:Implied:C:		CYCLES:2*/{
				this->f.Carry=1;
				return 2;
			}
			case 0x39: /*AND $DDDD,Y:Absolute,Y:Z,N:		CYCLES:4(+1)*/{
				this->A&=AbsoluteI(memory,opcode,this->Y);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;			
				return 4+(*(opcode+1)+this->Y>0xff);
			}
				case 0x3A: /*NOP(No instruction)*/	return 2;
				case 0x3B: /*NOP(No instruction)*/	break;
				case 0x3C: /*NOP(No instruction)*/	this->ProgramCounter+=2; return 5;
			case 0x3D: /*AND $DDDD,X:Absolute,X:Z,N:		CYCLES:4(+1)*/{
				this->A&=AbsoluteI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;			
				return 4+(*(opcode+1)+this->X>0xff);
			}	
			case 0x3E: /*ROL $DDDD,X:Absolute,X:C,Z,N:		CYCLES:7*/{
				unsigned short result=(AbsoluteI(memory,opcode,this->X)<<1)|this->f.Carry;
				unsigned short address=((*(opcode+2)<<8)|*(opcode+1))+this->X;
				carry(result);
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(address&0xff,(address&0xff00)>>8,result&0xff);
				return 7;
			}
				case 0x3F: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0x40 && *opcode<0x50){
		switch(*opcode){
			case 0x40: /*RTI:Implied:		CYCLES:6*/{
				char flg=readFromMemory(++this->StackPointer,0x1);
				this->f=*((flags*)(&flg));
				this->f.Break=0;
				this->f.Extra=1;
				this->ProgramCounter=readFromMemory(++this->StackPointer,0x1);
				this->ProgramCounter|=(readFromMemory(++this->StackPointer,0x1)<<8);
				//printf("RI: PG:0x%04X\tA:0x%02X X:0x%02X Y:0x%02X P:0x%02X SP:0x%02X O:0x%02X %02X %02X\n",ProgramCounter-1,this->A,this->X,this->Y,(*(char*)(&this->f))&0xff,this->StackPointer,*opcode,*(opcode+1),*(opcode+2));

				//printf("R:%04X\n",this->ProgramCounter);
				return 6;
			}
			case 0x41: /*EOR $DD,X:Indexed Indirect:Z,N:		CYCLES:6*/{
				this->A^=IndexedI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 6;
			}
				case 0x42: /*NOP(No instruction)*/	break;
				case 0x43: /*NOP(No instruction)*/	break;
				case 0x44: /*NOP(No instruction)*/	break;
			case 0x45: /*EOR $DD:Zero Page:Z,N:			CYCLES:3*/{
				this->A^=Zerop(memory,opcode);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 3;
			}
			case 0x46: /*LSR $DD:Zero Page:C,Z,N:		CYCLES:5*/{
				unsigned char rhs=Zerop(memory,opcode);
				unsigned char result=rhs>>1;
				zero(result);
				negative(result);
				carryR(rhs);
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,result);
				return 5;
			}
				case 0x47: /*NOP(No instruction)*/	break;
			case 0x48: /*PHA:Implied:		CYCLES:3*/{
				writeToMemory(this->StackPointer--,0x1,this->A);
				return 3;
			}
			case 0x49: /*EOR #$DD:Immediate:Z,N:		CYCLES:2*/{
				this->A^=*(opcode+1);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 2;
			}
			case 0x4A: /*LSR A:Accumulator:C,Z,N:		CYCLES:2*/{
				unsigned char result=this->A>>1;
				zero(result);
				negative(result);
				carryR(this->A);
				this->A=result;
				return 2;
			}
				case 0x4B: /*NOP(No instruction)*/	break;
			case 0x4C: /*JMP $DDDD: Absolute:		CYCLES:3*/{
				this->ProgramCounter=((*(opcode+2)<<8)|*(opcode+1));
				return 3;
			}
			case 0x4D: /*EOR $DDDD: Absolute: Z,N:		CYCLES:4*/{
				this->A^=Absolute(memory,opcode);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0x4E: /*LSR $DDDD: Absolute: C,Z,N:		CYCLES:6*/{
				unsigned char rhs=Absolute(memory,opcode);
				unsigned char result=rhs>>1;
				zero(result);
				negative(result);
				carryR(rhs);
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),result);
				return 6;
			}
				case 0x4F: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0x50 && *opcode<0x60){
		switch(*opcode){
			case 0x50: /*BVC $DD: Relative: 		CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Overflow==0){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0x51: /*EOR ($DD),Y: Indirect Indexed: Z,N:		CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				this->A^=readFromMemory(addr&0xff,(addr&0xff00)>>8);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 5+(*(opcode+1)+this->Y>0xff);
			}
				case 0x52: /*NOP(No instruction)*/break;
				case 0x53: /*NOP(No instruction)*/	break;
				case 0x54: /*NOP(No instruction)*/break;
			case 0x55: /*EOR $DD,X: Zero Page X: Z,N:		CYCLES:4*/{
				this->A^=ZeropI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 4;
			}
			case 0x56: /*LSR $DD,X: Zero Page X, C,Z,N:		CYCLES:6*/{
				unsigned char rhs=ZeropI(memory,opcode,this->X);
				unsigned char result=rhs>>1;
				zero(result);
				negative(result);
				carryR(rhs);
				++this->ProgramCounter;
				writeToMemory((*(opcode+1)+this->X)&0xff,0,result);
				return 6;
			}
				case 0x57: /*NOP(No instruction)*/	break;
			case 0x58: /*CLI: Implied: I		CYCLES: 2*/{
				this->f.InterruptDisable=0;
				return 2;
			}
			case 0x59: /*EOR: $DDDD,Y: Absolute Y: Z,N:		CYCLES:4(+1)*/{
				this->A^=AbsoluteI(memory,opcode,this->Y);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4+(*(opcode+1)+this->Y>0xff);
			}
				case 0x5A: /*NOP(No instruction)*/ return 2;
				case 0x5B: /*NOP(No instruction)*/	break;
				case 0x5C: /*NOP(No instruction)*/ this->ProgramCounter+=2; return 5;
			case 0x5D: /*EOR $DDDD,X: Absolute X: Z,N:		CYCLES:4(+1)*/{
				this->A^=AbsoluteI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4+(*(opcode+1)+this->X>0xff);
			}
			case 0x5E: /*LSR $DDDD,X: Absolute X: C,Z,N:		CYCLES:7*/{
				unsigned char rhs=AbsoluteI(memory,opcode,this->X);
				unsigned char result=rhs>>1;
				unsigned short address=((*(opcode+2)<<8)|*(opcode+1))+this->X;
				zero(result);
				negative(result);
				carryR(rhs);
				this->ProgramCounter+=2;
				writeToMemory((address&0xff),(address&0xff00)>>8,result);
				return 7;
			}
				case 0x5F: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0x60 && *opcode<0x70){
		switch(*opcode){
			case 0x60: /*RTS: Implied:		CYCLES:6*/{
				this->ProgramCounter=readFromMemory(++this->StackPointer,0x1);
				this->ProgramCounter|=(readFromMemory(++this->StackPointer,0x1)<<8);
				++this->ProgramCounter;
				return 6;
			}
			case 0x61: /*ADC ($DD,X): Indexed Indirect: C,Z,V,N,   CYCLES:6 */{
				unsigned short rhs=IndexedI(memory,opcode,this->X)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				++this->ProgramCounter;
				return 6;
			}
				case 0x62: /*NOP(No instruction)*/break;
				case 0x63: /*NOP(No instruction)*/	break;
				case 0x64: /*NOP(No instruction)*/break;
			case 0x65: /*ADC $DD: Zero Page: C,Z,V,N        CYCLES:3  */{
				unsigned short rhs=Zerop(memory,opcode)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				++this->ProgramCounter;
				return 3;
			}
			case 0x66: /*ROR $DD: Zero Page: C,Z,N:		CYCLES:5*/{
				unsigned short rhs=Zerop(memory,opcode);
				unsigned short result=(rhs>>1)|(this->f.Carry<<7);
				carry(((rhs&0x1)<<8));
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,result&0xff);
				return 5;
			}
				case 0x67: /*NOP(No instruction)*/	break;
			case 0x68: /*PLA: Implied: Z,N:		CYCLES:4*/{
				this->A=readFromMemory(++this->StackPointer,0x1);
				zero(this->A);
				negative(this->A);
				return 4;
			}
			case 0x69: /*ADC #$DD:Immediate: C,Z,V,N		CYCLES:2*/{
				unsigned short rhs=*(opcode+1)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				//printf("\tO:%i\tC:%i\n",this->f.Overflow,this->f.Carry);
				carry(result);
				this->A=result&0xff;
				++this->ProgramCounter;
				return 2;
			}
			case 0x6A: /*ROR A: Accumulator: C,Z,N:		CYCLES:2*/{
				unsigned short rhs=this->A;
				unsigned short result=(rhs>>1)|(this->f.Carry<<7);
				carry(((rhs&0x1)<<8));
				zero(result&0xff);
				negative(result&0xff);
				this->A=result&0xff;
				return 2;
			}
				case 0x6B: /*NOP(No instruction)*/	break;
			case 0x6C: /*JMP $NNNN: Indirect:		CYCLES:5*/{
				//printf("%04X\n",ProgramCounter);
				this->ProgramCounter=Absolute(memory,opcode);
				//
				if(*(opcode+1)==0xff){
					this->ProgramCounter|=(readFromMemory(0,*(opcode+2))<<8);
				}else{
					this->ProgramCounter|=(readFromMemory(*(opcode+1)+1,*(opcode+2))<<8);
				}
				return 5;
			}
			case 0x6D: /*ADC $DDDD: Absolute: C,Z,V,N		CYCLES:4*/{
				unsigned short rhs=Absolute(memory,opcode)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				this->ProgramCounter+=2;
				return 4;
			}
			case 0x6E: /*ROR $DDDD:Absolute: C,Z,N:		CYCLES:6*/{
				unsigned short rhs=Absolute(memory,opcode);
				unsigned short result=(rhs>>1)|(this->f.Carry<<7);
				carry(((rhs&0x1)<<8));
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),result&0xff);
				return 6;
			}
				case 0x6F: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0x70 && *opcode<0x80){
		switch(*opcode){
			case 0x70: /*BVS $DD: Relative: 		CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Overflow){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0x71: /*ADC ($DD),Y: Indirect Indexed: C,Z,V,N			CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				unsigned short rhs=readFromMemory(addr&0xff,(addr&0xff00)>>8)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				++this->ProgramCounter;
				return 5+(((addr-this->Y)&0xff)>(addr&0xff));
			}
				case 0x72: /*NOP(No instruction)*/break;
				case 0x73: /*NOP(No instruction)*/	break;
				case 0x74: /*NOP(No instruction)*/break;
			case 0x75: /*ADC $DD,X: Zero Page,X:C,Z,V,N			CYCLES:4*/{
				unsigned short rhs=ZeropI(memory,opcode,this->X)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				++this->ProgramCounter;
				return 4;
			}
			case 0x76: /*ROR $DD,X: Zero Page,X:C,Z,N:		CYCLES:6*/{
				unsigned short rhs=ZeropI(memory,opcode,this->X);
				unsigned short result=(rhs>>1)|(this->f.Carry<<7);
				unsigned short address=*(opcode+1)+this->X;
				carry(((rhs&0x1)<<8));
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory(address&0xff,0,result&0xff);
				return 6;
			}
				case 0x77: /*NOP(No instruction)*/	break;
			case 0x78: /*SEI: Implied: I:		CYCLES:2*/{
					this->f.InterruptDisable=1;
					return 2;
			}
			case 0x79: /*ADC $DDDD,Y: Absolute,Y: C,Z,V,N		CYCLES:4(+1)*/{
				unsigned short rhs=AbsoluteI(memory,opcode,this->Y)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				this->ProgramCounter+=2;
				return 4+ (*(opcode+1)+this->Y>0xff);
			}
				case 0x7A: /*NOP(No instruction)*/ return 2;
				case 0x7B: /*NOP(No instruction)*/	break;
				case 0x7C: /*NOP(No instruction*/ this->ProgramCounter+=2; return 5;
			case 0x7D: /*ADC $DDDD,X:Absolute,X: C,Z,V,N		CYCLES:4(+1)*/{
				unsigned short rhs=AbsoluteI(memory,opcode,this->X)+this->f.Carry;
				unsigned short result=this->A+rhs;
				zero(result&0xff);
				negative(result&0xff);
				over(result,this->A,rhs-this->f.Carry);
				carry(result);
				this->A=result&0xff;
				this->ProgramCounter+=2;
				return 4+(*(opcode+1)+this->X>0xff);
			}
			case 0x7E: /*ROR $DDDD,X: Absolute X: C,Z,N:		CYCLES:7*/{
				unsigned short rhs=AbsoluteI(memory,opcode,this->X);
				unsigned short result=(rhs>>1)|(this->f.Carry<<7);
				unsigned short address=((*(opcode+2)<<8)|*(opcode+1))+this->X;
				carry(((rhs&0x1)<<8));
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(address&0xff,(address&0xff00)>>8,result&0xff);
				return 7;
			}
				case 0x7F: /*NOP(No instruction)*/	break;
		}		
	}
	else if(*opcode>=0x80 && *opcode<0x90){
		switch(*opcode){
				case 0x80: /*NOP(No instruction*/break;
			case 0x81: /*STA($DD,X): Indexed Indirect:	CYCLES:6*/{
				unsigned short address=IndexedAdr(memory,opcode,this->X);
				++this->ProgramCounter;
				writeToMemory(address&0xff,(address&0xff00)>>8,this->A);
				return 6;
			}
				case 0x82: /*NOP(No instruction)*/break;
			case 0x83: /*NOP(No instruction) Unofficial SAX ($DD,X) Indexed Indirect*/{
				//unsigned short address=IndexedAdr(memory,opcode,this->X);
				//++this->ProgramCounter;
				//writeToMemory(address&0xff,(address&0xff00)>>8,this->A);
				//return 6;	
				return 2;				
			}
			case 0x84: /*STY $DD: Zero Page:	CYCLES:3*/{
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,this->Y);
				return 3;
			}
			case 0x85: /*STA $DD: Zero Page:	CYCLES:3*/{
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,this->A);
				return 3;
			}
			case 0x86: /*STX $DD: Zero Page:	CYCLES:3*/{
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,this->X);
				return 3;
			}
				case 0x87: /*NOP(No instruction)*/	break;
			case 0x88: /*DEY: Implied: Z,N:		CYCLES:2*/{
				--this->Y;
				zero(this->Y);
				negative(this->Y);
				return 2;
			}
				case 0x89: /*NOP(No instruction)*/break;
			case 0x8A: /*TXA: Implied: Z,N:		CYCLES:2*/{
				this->A=this->X;
				zero(this->A);
				negative(this->A);
				return 2;
			}
				case 0x8B: /*NOP(No instruction)*/	break;
			case 0x8C: /*STY $DDDD: Absolute:	CYCLES:4*/{
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),this->Y);
				return 4;
			}
			case 0x8D: /*STA $DDDD: Absolute:	CYCLES:4*/{
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),this->A);
				return 4;
			}
			case 0x8E: /*STX $DDDD:Absolute:	CYCLES:4*/{
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),this->X);
				return 4;
			}
				case 0x8F: /*NOP(No instruction)*/break;
		}
	}
	else if(*opcode>=0x90 && *opcode<0xA0){
		switch(*opcode){
			case 0x90: /*BCC $DD: Relative: 	CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Carry==0){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0x91: /*STA ($DD),Y: Indirect Indexed:	CYCLES:6*/{
				unsigned short address=IndirectAdr(memory,opcode,this->Y);
				++this->ProgramCounter;
				writeToMemory(address&0xff,(address&0xff00)>>8,this->A);
				return 6;
			}
				case 0x92: /*NOP(No instruction)*/break;
				case 0x93: /*NOP(No instruction)*/	break;
			case 0x94: /*STY $DD,X: Zero page,X:	CYCLES:4*/{
				++this->ProgramCounter;
				writeToMemory((*(opcode+1)+this->X)&0xff,0,this->Y);
				return 4;
			}
			case 0x95: /*STA $DD,X: Zero Page,X:	CYCLES:4*/{
				++this->ProgramCounter;
				writeToMemory((*(opcode+1)+this->X)&0xff,0,this->A);
				return 4;
			}
			case 0x96: /*STX $DD,Y: Zero Page,Y:	CYCLES:4*/{
				++this->ProgramCounter;
				writeToMemory((*(opcode+1)+this->Y)&0xff,0,this->X);
				return 4;
			}
				case 0x97: /*NOP(No instruction)*/	break;
			case 0x98: /*TYA: Implied: Z,N:		CYCLES:2*/{
				this->A=this->Y;
				zero(this->A);
				negative(this->A);
				return 2;
			}
			case 0x99: /*STA $DDDD,Y: Absolute Y:	CYCLES:5*/{
				unsigned short address=((*(opcode+2)<<8)|*(opcode+1))+this->Y;
				this->ProgramCounter+=2;
				writeToMemory(address&0xff,(address&0xff00)>>8,this->A);
				return 5;
			}
			case 0x9A: /*TXS :Implied:		CYCLES:2*/{
				this->StackPointer=this->X;
				return 2;
			
			}
				case 0x9B: /*NOP(No instruction)*/	break;
				case 0x9C: /*NOP(No instruction)*/ this->ProgramCounter+=2; return 5;
			case 0x9D: /*STA $DDDD,X: Absolute X:	CYCLES:5*/{
				unsigned short address=((*(opcode+2)<<8)|*(opcode+1))+this->X;
				this->ProgramCounter+=2;
				writeToMemory(address&0xff,(address&0xff00)>>8,this->A);
				return 5;
			}
				case 0x9E: /*NOP(No instruction)*/break;
				case 0x9F: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0xA0 && *opcode<0xB0){
		switch(*opcode){
			case 0xA0: /*LDY #$DD: Immediate: Z,N:		CYCLES:2*/{
				this->Y=*(opcode+1);
				zero(this->Y);
				negative(this->Y);
				++this->ProgramCounter;
				return 2;
			}
			case 0xA1: /*LDA ($DD,X): Indexed Indirect:Z,N:		CYCLES:6*/{
				this->A=IndexedI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 6;
			}
			case 0xA2: /*LDX #$DD: Immediate: Z,N:		CYCLES:2*/{
				this->X=*(opcode+1);
				zero(this->X);
				negative(this->X);
				++this->ProgramCounter;
				return 2;
			}
			case 0xA3: /*NOP(No instruction) Unofficial LAX ($DD,X)Indexed Indirect, load A and X with value.*/{				
				//this->X=IndexedI(memory,opcode,this->X);
				//this->A=this->X;
				//zero(this->X);
				//negative(this->X);
				//++this->ProgramCounter;
				//return 6;
				return 2;
			}
			case 0xA4: /*LDY $NN: Zero Page:Z,N:		CYCLES:3*/{
				this->Y=Zerop(memory,opcode);
				zero(this->Y);
				negative(this->Y);
				++this->ProgramCounter;
				return 3;
			}
			case 0xA5: /*LDA $NN: Zero Page:Z,N:		CYCLES:3*/{
				this->A=Zerop(memory,opcode);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 3;
			}
			case 0xA6: /*LDX $NN: Zero page:Z,N:		CYCLES:3*/{
				this->X=Zerop(memory,opcode);
				zero(this->X);
				negative(this->X);
				++this->ProgramCounter;
				return 3;
			}
			case 0xA7: /*NOP(No instruction) Unofficial LAX $NN Zero Page*/{				
				//this->X=Zerop(memory,opcode);
				//this->A=this->X;
				//zero(this->X);
				//negative(this->X);
				//++this->ProgramCounter;
				//return 3;
				return 2;
			}
			case 0xA8: /*TAY: Implied: Z,N:		CYCLES:2*/{
				this->Y=this->A;
				zero(this->Y);
				negative(this->Y);
				return 2;
			}
			case 0xA9: /*LDA #$DD: Immediate: Z,N:		CYCLES:2*/{
				this->A=*(opcode+1);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 2;
			}
			case 0xAA: /*TAX: Implied: Z,N:		CYCLES:2*/{
				this->X=this->A;
				zero(this->X);
				negative(this->X);
				return 2;
			}
				case 0xAB: /*NOP(No instruction)*/	break;
			case 0xAC: /*LDY $DDDD: Absolute: Z,N:		CYCLES:4*/{
				this->Y=Absolute(memory,opcode);
				zero(this->Y);
				negative(this->Y);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xAD: /*LDA $DDDD: Absolute: Z,N:		CYCLES:4*/{
				this->A=Absolute(memory,opcode);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xAE: /*LDX $DDDD: Absolute: Z,N:		CYCLES:4*/{
				this->X=Absolute(memory,opcode);
				zero(this->X);
				negative(this->X);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xAF: /*NOP(No instruction) Unofficial LAX $NNNN Absolute*/{				
				//this->X=Absolute(memory,opcode);
				//this->A=this->X;
				//zero(this->X);
				//negative(this->X);
				//this->ProgramCounter+=2;
				//return 4;
				return 2;
			}
		}
	}
	else if(*opcode>=0xB0 && *opcode<0xC0){
		switch(*opcode){
			case 0xB0: /*BCS $DD: Relative: 	CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Carry){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0xB1: /*LDA ($DD),Y: Indirect Indexed: Z,N:		CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				this->A=readFromMemory(addr&0xff,(addr&0xff00)>>8);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 5+(((addr-this->Y)&0xff)>(addr&0xff));
			}
				case 0xB2: /*NOP(No Instruction)*/break;
			case 0xB3: /*NOP(No instruction) Unofficial LAX ($DD),Y Indirect Indexed*/	{				
				//this->A=IndirectI(memory,opcode,this->Y);
				//this->X=this->A;
				//unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				//zero(this->A);
				//negative(this->A);
				//++this->ProgramCounter;
				//return 5+(((addr-this->Y)&0xff)>(addr&0xff));
				return 2;
			}
			case 0xB4: /*LDY $DD,X:Zero Page,X:Z,N:		CYCLES:4*/{
				this->Y=ZeropI(memory,opcode,this->X);
				zero(this->Y);
				negative(this->Y);
				++this->ProgramCounter;
				return 4;
			}
			case 0xB5: /*LDA $DD,X:Zero Page,X:Z,N:		CYCLES:4*/{
				this->A=ZeropI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				++this->ProgramCounter;
				return 4;
			}
			case 0xB6: /*LDX $DD,Y:Zero Page,Y:Z,N:		CYCLES:4*/{
				this->X=ZeropI(memory,opcode,this->Y);
				zero(this->X);
				negative(this->X);
				++this->ProgramCounter;
				return 4;
			}
			case 0xB7: /*NOP(No instruction) Unofficial LAX $DD,Y Zero Page*/{	
				//this->X=ZeropI(memory,opcode,this->Y);
				//this->A=this->X;
				//zero(this->X);
				//negative(this->X);
				//++this->ProgramCounter;
				//return 4;
				return 2;
			}
			case 0xB8: /*CLV:Implied:V		CYCLES:2*/{
				this->f.Overflow=0;
				return 2;
			}
			case 0xB9: /*LDA $DDDD,Y: Absolute,Y:Z,N:		CYCLES:4(+1)*/{
				this->A=AbsoluteI(memory,opcode,this->Y);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->Y)>0xff);
			}
			case 0xBA: /*TSX: Implied: Z,N:		CYCLES:2*/{
				this->X=this->StackPointer;
				zero(this->X);
				negative(this->X);
				return 2;
			}
				case 0xBB: /*NOP(No instruction)*/	break;
			case 0xBC: /*LDY $DDDD,X:Absolute X:Z,N:		CYCLES:4(+1)*/{
				this->Y=AbsoluteI(memory,opcode,this->X);
				zero(this->Y);
				negative(this->Y);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->X)>0xff);
			}
			case 0xBD: /*LDA $DDDD,X:Absolute X:Z,N:		CYCLES:4(+1)*/{
				this->A=AbsoluteI(memory,opcode,this->X);
				zero(this->A);
				negative(this->A);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->X)>0xff);
			}
			case 0xBE: /*LDX $DDDD,Y:Absolute Y:Z,N:		CYCLES:4(+1)*/{
				this->X=AbsoluteI(memory,opcode,this->Y);
				zero(this->X);
				negative(this->X);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->Y)>0xff);
			}
			case 0xBF: /*NOP(No instruction) Unofficial LAX $DDDD,Y*/{
				//this->X=AbsoluteI(memory,opcode,this->Y);
				//this->A=this->X;
				//zero(this->X);
				//negative(this->X);
				//this->ProgramCounter+=2;
				//return 4+((*(opcode+1)+this->Y)>0xff);			
				return 2;				
			}
		}
	}
	else if(*opcode>=0xC0 && *opcode<0xD0){
		switch(*opcode){
			case 0xC0: /*CPY #$DD: Immediate: C,Z,N:		CYCLES:2*/{
				unsigned short result=this->Y-*(opcode+1);
				carryCMP(this->Y,*(opcode+1));
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				return 2;
			}
			case 0xC1: /*CMP($DD,X):Indexed Indirect:C,Z,N		CYCLES:6*/{
				unsigned char rhs=IndexedI(memory,opcode,this->X);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				++this->ProgramCounter;
				return 6;
			}
				case 0xC2: /*NOP(No instruction)*/break;
				case 0xC3: /*NOP(No instruction)*/	break;
			case 0xC4: /*CPY $DD:Zero Page:C,Z,N:		CYCLES:3*/{
				unsigned char rhs=Zerop(memory,opcode);
				unsigned short result=this->Y-rhs;
				carryCMP(this->Y,rhs);
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				return 3;
			}
			case 0xC5: /*CMP $DD:Zero Page:C,Z,N:	CYCLES:3*/{
				unsigned char rhs=Zerop(memory,opcode);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				++this->ProgramCounter;
				return 3;
			}
			case 0xC6: /*DEC $DD:Zero Page:Z,N:		CYCLES:5*/{
				unsigned short result=Zerop(memory,opcode)-1;
				negative(result&0xff);
				zero(result&0xff);
				writeToMemory(*(opcode+1),0,result&0xff);
				++this->ProgramCounter;
				return 5;
			}
				case 0xC7: /*NOP(No instruction)*/	break;
			case 0xC8: /*INY: Implied: Z,N: 	CYCLES:2*/{
				++this->Y;
				zero(this->Y);
				negative(this->Y);
				return 2;
			}
			case 0xC9: /*CMP #$DD: Immediate:C,Z,N:		CYCLES:2*/{
				unsigned short result=this->A-*(opcode+1);
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,*(opcode+1));
				++this->ProgramCounter;
				return 2;
			}
			case 0xCA: /*DEX: Implied: Z,N: 	CYCLES:2*/{
				--this->X;
				zero(this->X);
				negative(this->X);
				return 2;
			}
				case 0xCB: /*NOP(No instruction)*/	break;
			case 0xCC: /*CPY $DDDD:Absolute: C,Z,N:		CYCLES:4*/{
				unsigned char rhs=Absolute(memory,opcode);
				unsigned short result=this->Y-rhs;
				carryCMP(this->Y,rhs);
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xCD: /*CMP $DDDD:Absolute: C,Z,N:		CYCLES:4*/{
				unsigned char rhs=Absolute(memory,opcode);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xCE: /*DEC $DDDD:Absolute: Z,N:		CYCLES:6*/{
				unsigned short result=Absolute(memory,opcode)-1;
				negative(result&0xff);
				zero(result&0xff);
				writeToMemory(*(opcode+1),*(opcode+2),result&0xff);
				this->ProgramCounter+=2;
				return 6;
			}
				case 0xCF: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0xD0 && *opcode<0xE0){
		switch(*opcode){
			case 0xD0: /*BNE $DD: Relative:		CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Zero==0){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0xD1: /*CMP ($DD),Y: Indirect Indexed:C,Z,N:		CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);	
				unsigned char rhs=readFromMemory(addr&0xff,(addr&0xff00)>>8);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				++this->ProgramCounter;
				return 5+(((addr-this->Y)&0xff)>(addr&0xff));
			}
				case 0xD2: /*NOP(No instruction)*/break;
				case 0xD3: /*NOP(No instruction)*/	break;
				case 0xD4: /*NOP(No instruction)*/break;
			case 0xD5: /*CMP $DD,X: Zero Page X:C,Z,N:		CYCLES:4*/{
				unsigned char rhs=ZeropI(memory,opcode,this->X);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				++this->ProgramCounter;
				return 4;
			}
			case 0xD6: /*DEC $DD,X: Zero Page X: Z,N:		CYCLES:6*/{
				unsigned short result=ZeropI(memory,opcode,this->X)-1;
				unsigned char address=*(opcode+1)+this->X;
				negative(result&0xff);
				zero(result&0xff);
				writeToMemory(address&0xff,0,result&0xff);
				++this->ProgramCounter;
				return 6;
			}
				case 0xD7: /*NOP(No instruction)*/	break;
			case 0xD8: /*CLD: Implied: D		CYCLES:2*/{
				this->f.DecimalMode=0;
				return 2;
			}
			case 0xD9: /*CMP $DDDD,Y: Absolute Y:C,Z,N:		CYCLES:4(+1)*/{
				unsigned char rhs=AbsoluteI(memory,opcode,this->Y);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->Y)>0xff);
			}
				case 0xDA: /*NOP(No instruction)*/ return 2;
				case 0xDB: /*NOP(No instruction)*/	break;
				case 0xDC: /*NOP(No instruction)*/ this->ProgramCounter+=2; return 5;
			case 0xDD: /*CMP $DDDD,X: Absolute X:C,Z,N:		CYCLES:4(+1)*/{
				unsigned char rhs=AbsoluteI(memory,opcode,this->X);
				unsigned short result=this->A-rhs;
				zero(result&0xff);
				negative(result&0xff);
				carryCMP(this->A,rhs);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->X)>0xff);
			}
			case 0xDE: /*DEC $DDDD,X: Absolute X:Z,N:		CYCLES:7*/{
				unsigned short result=AbsoluteI(memory,opcode,this->X)-1;
				unsigned short address=(*(opcode+1)|(*(opcode+2))<<8)+this->X;
				negative(result&0xff);
				zero(result&0xff);
				writeToMemory(address&0xff,(address&0xff00)>>8,result&0xff);
				this->ProgramCounter+=2;
				return 7;
			}
				case 0xDF: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0xE0 && *opcode<0xF0){
		switch(*opcode){
			case 0xE0: /*CPX #$DD: Immediate:C,Z,N:		CYCLES:2*/{
				unsigned short result=this->X-*(opcode+1);
				carryCMP(this->X,*(opcode+1));
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				return 2;
			}
			case 0xE1: /*SBC ($DD,X): Indexed Indirect:C,Z,V,N:		CYCLES:6*/{
				unsigned char rhs=~IndexedI(memory,opcode,this->X);
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				++this->ProgramCounter;
				return 6;
			}
				case 0xE2: /*NOP(No instruction)*/break;
				case 0xE3: /*NOP(No instruction)*/	break;
			case 0xE4: /*CPX $DD: Zero Page:C,Z,N:		CYCLES:3*/{
				unsigned char rhs=Zerop(memory,opcode);
				unsigned short result=this->X-rhs;
				carryCMP(this->X,rhs);
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				return 3;
			}
			case 0xE5: /*SBC $DD: Zero Page:C,Z,V,N:		CYCLES:3*/{
				unsigned char rhs=~Zerop(memory,opcode);
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				++this->ProgramCounter;
				return 3;
			}
			case 0xE6: /*INC $DD: Zero Page:Z,N:		CYCLES:5*/{
				unsigned short result=Zerop(memory,opcode)+1;
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory(*(opcode+1),0,result&0xff);
				return 5;
			}
				case 0xE7: /*NOP(No instruction)*/	break;
			case 0xE8: /*INX: Implied: Z,N: 	CYCLES:2*/{
				++this->X;
				zero(this->X&0xff);
				negative(this->X&0xff);
				return 2;
			}
			case 0xE9: /*SBC #$DD: Immediate: C,Z,V,N:		CYCLES:2*/{
				unsigned char rhs=~(*(opcode+1));
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				++this->ProgramCounter;
				return 2;
			}
			//dedicated NOP
			case 0xEA: /*NOP(No instrutcion)*/{
				return 2;
			}
				case 0xEB: /*NOP(No instruction)*/	break;
			case 0xEC: /*CPX $DDDD: Absolute:C,Z,N:		CYCLES:4*/{
				unsigned char rhs=Absolute(memory,opcode);
				unsigned short result=this->X-rhs;
				carryCMP(this->X,rhs);
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xED: /*SBC $DDDD: Absolute:C,Z,V,N:		CYCLES:4*/{
				unsigned char rhs=~Absolute(memory,opcode);
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				this->ProgramCounter+=2;
				return 4;
			}
			case 0xEE: /*INC $DDDD: Absolute:Z,N:		CYCLES:6*/{
				unsigned short result=Absolute(memory,opcode)+1;
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(*(opcode+1),*(opcode+2),result&0xff);
				return 6;
			}
				case 0xEF: /*NOP(No instruction)*/	break;
		}
	}
	else if(*opcode>=0xF0 && *opcode<0x100){
		switch(*opcode){
			case 0xF0: /*BEQ $DD: Relative: CYCLES: 2(+1,+2)*/{
				++this->ProgramCounter;
				if(this->f.Zero){
					unsigned short original=this->ProgramCounter;
					this->ProgramCounter=Branch(original,*(opcode+1));
					char result=((original&0xff00)!=(this->ProgramCounter&0xff00));
					return 3+result;
				}
				return 2;
			}
			case 0xF1: /*SBC ($DD),Y:Indirect Indexed: C,Z,V,N:		CYCLES:5(+1)*/{
				unsigned short addr=IndirectAdr(memory,opcode,this->Y);
				unsigned char rhs=~(readFromMemory(addr&0xff,(addr&0xff00)>>8));
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				++this->ProgramCounter;
				return 5+(((addr-this->Y)&0xff)>(addr&0xff));
			}
				case 0xF2: /*NOP(No instuction)*/break;
				case 0xF3: /*NOP(No instruction)*/	break;
				case 0xF4: /*NOP(No instruction)*/break;
			case 0xF5: /*SBC $DD,X: Zero Page,X: C,Z,V,N:		CYCLES:4*/{
				unsigned char rhs=~ZeropI(memory,opcode,this->X);
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				++this->ProgramCounter;
				return 4;
			}
			case 0xF6: /*INC $DD,X: Zero Page,X:Z,N:		CYCLES:6*/{
				unsigned short result=ZeropI(memory,opcode,this->X)+1;
				unsigned short address=*(opcode+1)+this->X;
				zero(result&0xff);
				negative(result&0xff);
				++this->ProgramCounter;
				writeToMemory(address&0xff,0,result&0xff);
				return 6;
			}
				case 0xF7: /*NOP(No instruction)*/	break;
			case 0xF8: /*SED: Implied: D:		CYCLES:2*/{
				this->f.DecimalMode=1;
				return 2;
			}
			case 0xF9: /*SBC $DDDD,Y: Absolute,Y: C,Z,V,N:		CYCLES:4(+1)*/{
				unsigned char rhs=~AbsoluteI(memory,opcode,this->Y);
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->Y)>0xff);
			}
				case 0xFA: /*NOP(No instruction)*/ return 2;
				case 0xFB: /*NOP(No instruction)*/	break;
				case 0xFC: /*NOP(No instruciton)*/ this->ProgramCounter+=2; return 5;
			case 0xFD: /*SBC $DDDD,X: Absolute X: C,Z,V,N:		CYCLES:4(+1)*/{
				unsigned char rhs=~AbsoluteI(memory,opcode,this->X);
				unsigned short result=this->A+rhs+(this->f.Carry);
				over(result&0xff,this->A,rhs);	
				zero(result&0xff);
				negative(result&0xff);
				carry(result);
				this->A=(result&0xff);
				this->ProgramCounter+=2;
				return 4+((*(opcode+1)+this->X)>0xff);
			}
			case 0xFE: /*INC $DDDD,X: Absolute X: Z,N:		CYCLES:7*/{
				unsigned short result=AbsoluteI(memory,opcode,this->X)+1;
				unsigned short address=((*(opcode+2)<<8)|*(opcode+1))+this->X;
				zero(result&0xff);
				negative(result&0xff);
				this->ProgramCounter+=2;
				writeToMemory(address&0xff,(address&0xff00)>>8,result&0xff);
				return 7;
			}
				case 0xFF: /*NOP(No instruction)*/break;
		}
	}
	++this->ProgramCounter;
	return 3;
}