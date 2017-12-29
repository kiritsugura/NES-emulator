#include "PPU.h"
#include "Processor.h"
#include <string.h>
#include <stdlib.h>

#undef DEBUG_H
#define WIDTH 800
#define HEIGHT 700

#define SWAP(integer) (((integer&0x0000ff00)<<8)|((integer&0xff000000)>>24)|((integer&0x00ff0000)>>8))

#define SpriteSheet(PPUCTRL) ((((PPUCTRL)&0x8)>0) ? 0x1000:0x0000)
#define BackSheet(PPUCTRL) ((((PPUCTRL)&0x10)==0) ? 0x0000:0x1000)
PPU::PPU(Processor *pro){
	vRam=new unsigned char[0x4000];
	spriteRam=new unsigned char[0x100];
	secondOAM=new unsigned char[0x20];
	memset(vRam,0,0x4000);
	cpu=pro;
	PPUCTRL=0;
	PPUMASK=0;
	PPUSTATUS=0;
	OAMADDR=0;
	OAMDATA=0;
	PPUADDR=0;
	PPUDATA=0;
	OAMDMA=0;
	currentScanline=261;
	addressReset=0;
	writeToggle=0;
	fineX=0;
	tempAddress=0;
	currentAddress=0;
	dma=0;
	fromCPU=0;
	if(SDL_Init(SDL_INIT_VIDEO)<0)
        exit(10);
	this->window=NULL;
	this->window=SDL_CreateWindow("NES Emulator",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,WIDTH,HEIGHT,SDL_WINDOW_SHOWN);
	if(this->window==NULL)
        exit(6);
	unsigned long pal[]={0x75757500, 0x8F1B2700, 0xAB000000, 0x9F004700, 0x77008F00, 0x1300AB00, 0x0000A700, 0x000B7F00, 0x002F4300, 0x00470000, 0x00510000, 0x173F0000, 0x5F3F1B00, 0x00000000, 0x00000000,0x00000000,
			 0xBCBCBC00, 0xEF730000, 0xEFB32300, 0xF3008300, 0xBF00BF00, 0x5B00E700, 0x002BDB00, 0x0F4FCB00, 0x00738B00, 0x00970000, 0x00AB0000, 0x3B930000, 0x8B830000, 0x00000000, 0x00000000,0x00000000,
			 0xFFFFFF00, 0xFFBF3F00, 0xFF975F00, 0xFD8BA700, 0xFF7BF700, 0xB777FF00, 0x6377FF00, 0x3B9BFF00, 0x3FBFF300, 0x13D38300, 0x4BDF4F00, 0x98F85800, 0xDBEB0000, 0x00000000, 0x00000000,0x00000000,
			 0xFFFFFF00, 0xFFE7AB00, 0xFFD7C700, 0xFFCDD700, 0xFFC7FF00, 0xDBC7FF00, 0xB3BFFF00, 0xABDBFF00, 0xA3E7FF00, 0xA3FFE300, 0xBFF3AB00, 0xCFFFB300, 0xF3FF9F00, 0x00000000, 0x00000000,0x00000000};
	int i=0;
	while(i<0x40){
		Colors[i]=SWAP(pal[i]);
		i++;
	}
}


PPU::~PPU(){
	delete vRam;
	delete spriteRam;
}


//1 is vertical, 0 is horizontal mirroring.
void PPU::setMirroring(unsigned char c){
	this->mirroring=c;
}

void PPU::setPatternTable(unsigned char *buffer){
	memcpy(this->vRam,buffer,0x2000);
	#ifdef DEBUG_H
		printf("Pattern table CHRROM->PPU VRAM.\n");
	#endif	
	/* Pattern Table test.
	SDL_Surface *screen=NULL;
	screen=SDL_GetWindowSurface(window);
	unsigned long color;
	unsigned short x=0,y=0,rowdex;
	short i=0;
	int xoffset=0,yoffset=0;
	unsigned row[8];
	int redo=0;		
	Uint8 * pixel = (Uint8*)screen->pixels;
	while(i<0x1000){
		rowdex=0;
		while(rowdex<8){
			row[rowdex]=((vRam[i+redo]&(0x1<<(7-rowdex)))>>(7-rowdex)) | (((vRam[i+0x8+redo]&(0x1<<(7-rowdex)))>>(7-rowdex))<<1);
			//printf("(%i %i)%i-%04X ",x,y,row[rowdex],i);
			switch(row[rowdex]&0x3){
				case 0:
					color=0xCC0CC000;
					break;
				case 1:
					color=0x00ffff00;
					break;
				case 2:
					color=0x0000ffff;
					break;
				case 3:
					color=0x00ff00ff;
					break;
			}

			*(Uint32*)(pixel + (y*2 * screen->pitch) + (x*2 * sizeof(Uint32)))=color;
			*(Uint32*)(pixel + ((y*2+1) * screen->pitch) + (x*2 * sizeof(Uint32)))=color;
			*(Uint32*)(pixel + (y*2 * screen->pitch) + ((x*2+1) * sizeof(Uint32)))=color;
			*(Uint32*)(pixel + ((y*2+1) * screen->pitch) + ((x*2+1) * sizeof(Uint32)))=color;
			//*((Uint32*)pixel)= color;
			++x;
			++rowdex;
		}
		//printf("\n");
		x=xoffset;
		++y;
		if((y&0xf)==8){
			xoffset+=8;
			y=yoffset;
		}
		if(xoffset==128){
			yoffset+=8;
			xoffset=0;
		}
		if((i&0x7)==0x7) i+=0x8;
		++i;
		if(i>=0x1000 && redo==0){
			i=0;
			x=0;
			xoffset=0;
			y=20;
			yoffset=200;
			redo=0x1000;
		}
	}*/
}

void PPU::render(){
	static SDL_Rect r={0,0,800,600};
	static SDL_Surface *screen=SDL_GetWindowSurface(window);
	SDL_UpdateWindowSurface(window);
	SDL_FillRect(screen,&r,0x00FF0000);
}


void PPU::writeToSRAM(unsigned short address, unsigned char value){}
unsigned char PPU::readFromSRAM(unsigned short address){}
void PPU::SRAMDMA(unsigned char *memory){
	memcpy(spriteRam,memory,0x100);
	OAMREST=0;
	dma=1;
	#ifdef DEBUG_H
		printf("OAM Set\n");
		int i=0;
		for(i=0;i<64;++i){
			printf("Y: %02X\n",spriteRam[i<<2]);
		}
	#endif
	//printf("OAM Set\n");
	//printf("%i %i %i %i\n",spriteRam[0],spriteRam[1],spriteRam[2],spriteRam[3]);
}
void PPU::writeToVRAM(unsigned short address, unsigned char value){
	//#ifdef DEBUG_H
//printf("Write to VRAM 0x%04X 0x%02X %i\n",address,value,currentScanline);
	//#endif
	//address&=0x3fff;
	if(address<0x2000){
		vRam[address]=value;
	}else if(address<0x3F00){
		switch(mirroring){
			//Horizontal mirroring.
			case 0:{
				vRam[currentAddress&0x23FF]=value;
				vRam[(currentAddress&0x23FF)+0x400]=value;
				//#ifdef DEBUG_H
		//printf("Nametable at 0x%04X,0x%04X\t0x%02X\n",currentAddress&0x27FF,(currentAddress&0x27FF)+0x800,value);
				//#endif				
				break;
			}
			//Vertical mirroring.
			case 1:{
				vRam[currentAddress&0x27FF]=value;
				vRam[(currentAddress&0x27FF)+0x800]=value;
				#ifdef DEBUG_H
					printf("Nametable at 0x%04X,0x%04X\t0x%02X\n",currentAddress&0x27FF,(currentAddress&0x27FF)+0x800,value);
				#endif
				break;					
			}
			case 2:{
				vRam[currentAddress&0x23FF]=value;
				vRam[(currentAddress&0x23FF)+0x400]=value;
				vRam[currentAddress&0x27FF]=value;
				vRam[(currentAddress&0x27FF)+0x800]=value;
				#ifdef DEBUG_H
					printf("Nametable at 0x%04X,0x%04X\t0x%02X\n",currentAddress&0x27FF,(currentAddress&0x27FF)+0x800,currentAddress&0x23FF,(currentAddress&0x23FF)+0x400,value);
				#endif
				break;					
			}
			default:{
				printf("Unsupported mirroring, exiting.\n");
				exit(5);
			}
		}
	}else if(address<0x3fff){
		if((address&0x3)==0){
			#ifdef DEBUG_H
				printf("Written to PAL BGCD at 0x%04X\t0x%02X\n",0x3F00+(address&0xf),value);
			#endif
			char temp=0;
			vRam[address&0x3f0f]=value;
			vRam[(address&0x3f0f)+0x10]=value;
		}
		else{					
		#ifdef DEBUG_H
			printf("Written to PAL at 0x%04X\t0x%02X\n",(address&0x3f1f),value);
		#endif
			vRam[address]=value;
		}
	}else{
		//printf("DERD 0x%04X\n",address);
		//exit(4);
	}
	if((PPUCTRL&0x4)>0){
		currentAddress+=32;
	}else{
		++currentAddress;
	}		
}

//1 if vblank, 0 if not.
int PPU::VBlank(){
	return (PPUSTATUS&0x80)>0;
}

unsigned char PPU::readFromVRAM(unsigned short address){
	unsigned char value=0;
	static unsigned char currentChar,lastChar;
	//#ifdef DEBUG_H
//printf("Read at 0x%04X\t0x%02X\n",currentAddress,value);
	//#endif
	//address&=0x3fff;
	if(address<0x2000){
		currentChar=vRam[address];
		value=lastChar;
		lastChar=currentChar;
	}else if(address<0x3F00){
			#ifdef DEBUG_H
				printf("Nametable read at 0x%04X\t0x%02X\n",currentAddress,value);
			#endif
			currentChar=vRam[address];
			#ifdef DEBUG_H
				printf("Nametable mirror read at 0x%04X\t0x%02X\n",currentAddress,value);
			#endif
		value=lastChar;
		lastChar=currentChar;
	}else if(currentScanline>240){
		address&=0x3f1f;
		//background colors only.
		if((address&0x3)==0){
			#ifdef DEBUG_H
				printf("Read PAL BGCD at 0x%04X\t0x%02X\n",address&0x3f1f,value);
			#endif
			value=vRam[address&0x3f00+(address&0xc)];
			value=vRam[address&0x3f10+(address&0xc)];
		}else{
							
			#ifdef DEBUG_H
				printf("Read PAL at 0x%04X\t0x%02X\n",(address&0x3f1f),value);
			#endif
			value=vRam[address&0x3f1f];
		}
		lastChar=value;
	}else{

		return value;
	}
	if(PPUCTRL&0x4){
		currentAddress+=32;
	}else{
		++currentAddress;
	}
	return value;
}
void PPU::setRegister(unsigned char reg, unsigned char value){
	//disallow certain registers on startup.
	ioLine=value;
	switch(reg&0x0f){
		case 0:{	
			//write only.
			#ifdef DEBUG_H
				printf("Write to PPUCTRL(Controller):\n\tNMI enable(V):\t\t\t%i\n\tPPU master/slave(P):\t\t%i\n\tSprite Height(H):\t\t%i\n\tBackground Tile Select(B):\t%i\n\tSprite Tile Select(S):\t\t%i\n\tIncrement Mode(I):\t\t%i\n\tNametable Select(NN):\t\t%i\n\t\t\t\t\t%i\n\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0,(value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0); 
			#endif
			tempAddress=(tempAddress&(0x73FF))| ((value&0x3)<<10);
			PPUCTRL=value;
			if((PPUSTATUS&0x80)>0 && (value&0x80)>0){
				PPUSTATUS&=0x7f;
				cpu->Interrupt(0);
			}
			break;
		}
		case 1:{	
			//write only.
			#ifdef DEBUG_H
				printf("Write to PPUMASK(Mask):\n\tBlue Emphasis(B):\t\t\t%i\n\tGreen Emphasis(G):\t\t\t%i\n\tRed Emphasis(R):\t\t\t%i\n\tShow Sprites(s):\t\t\t%i\n\tShow Background(b):\t\t\t%i\n\tSprite Mask 8 Leftmost Pixels(M):\t%i\n\tBackground Mask 8 Leftmost Pixels(m):\t%i\n\tGreyscale(G):\t\t\t\t%i\n\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0,(value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0);
			#endif
			//printf("SC: %i\n",currentScanline);
			PPUMASK=value;
			break;
		}
		case 3:{	
			//write only.
			#ifdef DEBUG_H
				printf("Write to OAMADDR:\n\tAddress:\t%i\n\t\t\t%i\n\t\t\t%i\n\t\t\t%i\n\t\t\t%i\n\t\t\t%i\n\t\t\t%i\n\t\t\t%i 0x%02X\n\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0, (value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0,value);
			#endif
			OAMADDR=value;
			OAMREST=OAMADDR;
			//printf("ADDR: %02X\n",OAMADDR);
			break;
		}
		case 4:{	
			//read write.
			#ifdef DEBUG_H
				printf("Write to OAMDATA:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0, (value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0);
			#endif
			if(currentScanline>240){
				spriteRam[(OAMADDR++)&0xff]=value;
				OAMREST=OAMADDR;
			}
			break;
		}
		case 5:{	
			//write x2
			#ifdef DEBUG_H
				printf("Write to PPUSCROLL:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0, (value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0);
			#endif
	//CV sets scroll after status bar.
			//if(currentScanline<240 && (PPUMASK&0x18)>0)
//printf("SCROLL: 0x%02X,%i\n",value,currentScanline);
			if(writeToggle==0){
				PPUSCROLLQ1=value;
				fineX=PPUSCROLLQ1&0x7;
				tempAddress=(tempAddress&0x7FE0)|((PPUSCROLLQ1&0xF8)>>3);
				writeToggle=1;
			}else if(writeToggle==1){
				writeToggle=0;
				PPUSCROLLQ2=value;
				tempAddress=(tempAddress&0x0C1F)|((PPUSCROLLQ2&0x7)<<12)|((PPUSCROLLQ2&0xf8)<<2);
			}
			break;
		}
		case 6:{	
			//write x2
			#ifdef DEBUG_H
				printf("Write to PPUADDR:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0, (value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0);
			#endif
			PPUADDR=value;
			if(writeToggle==0){
				tempAddress=(tempAddress&0xFF)|((PPUADDR&0x3F)<<8);
				tempAddress&=(0x3FFF);
				#ifdef DEBUG_H
					printf("START PPUADDRESS: 0x%04X\n",currentAddress);
				#endif
				writeToggle=1;
			}else if(writeToggle==1){
				tempAddress=(tempAddress&0x7F00)|(PPUADDR);
				currentAddress=tempAddress;
				//#ifdef DEBUG_H
				//if(currentScanline<240 && (PPUMASK&0x18)>0)
	//printf("SET PPUADDRESS: 0x%04X %i\n",currentAddress,currentScanline);
				//#endif
				writeToggle=0;
			}
			break;
		}
		case 7:{	
			//write deal with the mirroring here as well.
			#ifdef DEBUG_H
				printf("Write to PPUDATA:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
				(value&0x80)>0,(value&0x40)>0,(value&0x20)>0, (value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0);
				printf("PPU DATA WRITE: 0x%02X\tP:0x%04X\n",value,currentAddress);
			#endif	
			if((PPUMASK&0x10)>0 && currentScanline<240){
				printf("BAD\n");
			}
			writeToVRAM(currentAddress,value);
			PPUDATA=value;
			break;
		}
		//case 8:{
			//#ifdef DEBUG_H
			//	printf("Write to OAMDMA:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
			//	(value&0x80)>0,(value&0x40)>0,(value&0x20)>0, (value&0x10)>0,(value&0x8)>0,(value&0x4)>0,(value&0x2)>0,(value&0x1)>0);
			//#endif
			//OAMDMA=value;
			//break;
		//}
	}
	ioLine=value;
}
// scanlines 240-261 are vblank. 
//262 scanlines per frame, scanline lasts for 341 PPU clock cycles (3 PPU cycles=1 CPU cycle)  //89342 PPU cycles to render this.
//vblank lasts 20 scanlines. 
//each clock cycle produces one pixel.
//CPU is 559 nano per cycle, so PPU is about 186 nano per cycle. meaning 5,369,319 cycles per second.
 
//pattern table identifies the palette entry into the palette table from the start offset.
//value stored in each entry of the nametable(Larger one) is the tile that is going to be used. 
//Value stored in the attribute table is the upper two bits of the palatte index into the system palatte.

void PPU::printVRAM(unsigned short offset,unsigned short bounds){
	int i=0;
	printf("VRAM:\n");
	for(i=offset;i<offset+bounds;++i){
		printf("  0x%04X:\t0x%02X\n",i,this->vRam[i]);
	}
}
unsigned char PPU::readRegister(unsigned char reg){
	fromCPU=1;
	switch(reg&0x0f){
		case 2:{
			//read only.
			#ifdef DEBUG_H
				printf("Read from PPUSTATUS(Status):\n\tVertical Blank(V):\t%i\n\tSprite 0 Hit(S):\t%i\n\tSprite Overflow(O):\t%i\n\tOther:\t\t\t%i\n\t\t\t\t%i\n\t\t\t\t%i\n\t\t\t\t%i\n\t\t\t\t%i\n",
				(PPUSTATUS&0x80)>0,(PPUSTATUS&0x40)>0,(PPUSTATUS&0x20)>0,(PPUSTATUS&0x10)>0,(PPUSTATUS&0x8)>0,(PPUSTATUS&0x4)>0,(PPUSTATUS&0x2)>0,(PPUSTATUS&0x1)>0); 
			#endif
			writeToggle=0;
			ioLine=PPUSTATUS;
			PPUSTATUS&=(0x7F);
			break;
		}
		case 4:{	
			//read write.
			//#ifdef DEBUG_H
				printf("Read from OAMDATA:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
				OAMDATA&0x80>0,OAMDATA&0x40>0,OAMDATA&0x20>0,OAMDATA&0x10>0,OAMDATA&0x8>0,OAMDATA&0x4>0,OAMDATA&0x2>0,OAMDATA&0x1>0);
			//#endif
			ioLine=spriteRam[(OAMADDR++)&0xff];
			OAMREST=OAMADDR;
			break;
		}
		case 7:{	
			//write and read
			#ifdef DEBUG_H
				printf("Read from PPUDATA:\n\tData:\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n\t\t%i\n",
				PPUDATA&0x80>0,PPUDATA&0x40>0,PPUDATA&0x20>0, PPUDATA&0x10>0,PPUDATA&0x8>0,PPUDATA&0x4>0,PPUDATA&0x2>0,PPUDATA&0x1>0);
			#endif
			ioLine=readFromVRAM(currentAddress&0x3fff);
			//if((PPUMASK&0x10)>0 && currentScanline<240){
	//printf("PPUDATA %i at 0x%04X S:%i\n",ioLine,currentAddress,currentScanline);
			//}
			//ioLine=0;
			break;
		}
		//default: printf("pain");
	}
	return ioLine;
}

//divide into two functions for doing the background then the sprite evaluation.

unsigned char PPU::executePPU(unsigned short cycles){
	if(cycles>1500){
		//printf("Cycles: %i\n",cycles);
		//cycles-=1;
		//fromCPU=0;
	}
	//if(dma){
	//	dma=0;
	//	cycles+=512;
	//}
	while(cycles){
		this->executeBack();
		--cycles;
	}
}

unsigned char PPU::attributeFind(unsigned char addOff,unsigned char attribute){
	//left.
	if((addOff&0x3)<2){
		//top
		if((addOff&0x7f)<0x40){
			attribute&=0x3;
			attribute<<=2;
		}
		//bottom
		else if((addOff&0x7f)>0x39){
			attribute&=0x30;
			attribute>>=2;
		}
	}
	//right
	else if((addOff&0x3)>1){
		//top
		if((addOff&0x7f)<0x40){
			attribute&=0xC;
		}
		//bottom
		else if((addOff&0x7f)>0x39){
			attribute&=0xC0;
			attribute>>=4;
		}
	}
	return attribute;
}

//rewrite to wrap better and use the current address when set.

void PPU::executeBack(){
	static int currentCycles=0;
	static SDL_Surface *screen=SDL_GetWindowSurface(window);
	static Uint8 * pixel = (Uint8*)screen->pixels;
	static unsigned long backColor,spriteColor,outputColor;
	static unsigned short tileBMPLow=0,tileBMPHigh=0,attribute=0,nametable=0,addOff=0,scale=2,pdex=0,s0,offsetX=0,spriteBMPLow,spriteBMPHigh,s0High=0;
	static unsigned char currentSprites=0,evaled=0,currentX;
	static unsigned char currentSpriteData[0x20],currentSpriteNums=0,s0Hit=0,sAttrib,spriteClamp=0,spriteDex=0,sdex,sprite0Hit=0,mode16=0,alternated=0,continuing=0,OVF=0;
	
	if(currentCycles<256 && currentCycles!=0 && currentScanline<240){
		//Sprite evaluation...
		if(currentCycles<65){	//clear out spriteRAM
		}
		//write the sprites...
		else if(evaled!=1 && currentSprites!=8){
			if(currentCycles==65){
				OAMADDR=OAMREST;
				//printf("F:%02X\n",OAMREST);
			}
			if(currentSprites!=8){
				if( ( ((spriteRam[OAMADDR]+1<=currentScanline+1) && ((spriteRam[OAMADDR]+9)>currentScanline+1)) || ((spriteRam[OAMADDR]+1<=currentScanline+1)  && (PPUCTRL&0x20)>0 && ((spriteRam[OAMADDR]+17)>currentScanline+1)) ) && (secondOAM[currentSprites<<2]<240)){	
					secondOAM[currentSprites<<2]=spriteRam[OAMADDR];
					secondOAM[(currentSprites<<2)+1]=spriteRam[OAMADDR+1];
					secondOAM[(currentSprites<<2)+2]=spriteRam[OAMADDR+2];
					secondOAM[(currentSprites<<2)+3]=spriteRam[OAMADDR+3];
					++currentSprites;
					if(currentCycles==65){
						//printf("%i\n",currentCycles);
						s0Hit=1;
					}
					//Potential Sprite0 hit.
				}

			}
			OAMADDR+=4;
			if(OAMADDR==0 || OAMADDR>0xff){
				evaled=1;
				OAMADDR=OAMREST;
			}
		}
		//we have found 8 sprites, check for overflow.
		else if(currentSprites==8 && !evaled){
			if((spriteRam[OAMADDR]+1<=currentScanline+1 && spriteRam[OAMADDR]+9>currentScanline+1) || ((spriteRam[OAMADDR]+1<=currentScanline+1)  && (PPUCTRL&0x20)>0 && ((spriteRam[OAMADDR]+17)>currentScanline+1))){
				PPUSTATUS|=0x20;
				evaled=1;
				OAMADDR=OAMREST;
				//printf("Overflow\n");
			}else{
				++OAMADDR;
			}
			if((OAMADDR&0x3)==0){
				OAMADDR-=4;
			}
			if((PPUSTATUS&0x20)==0){
				OAMADDR+=4;	
			}
			if(OAMADDR>0xff || OAMADDR==0){
				evaled=1;
				OAMADDR=OAMREST;
			}
		}
	}
	if(currentCycles==0){
	}
	//The scanline is being processed and rendered.
	else if(currentCycles<256 && currentScanline>7 && currentScanline<232 && (PPUMASK&0x18)>0 ){
		//printf("B:0x%04X %i\n",currentAddress,currentScanline);
		
		//background evaluation.
		//time to swap out the values...
		if( (((currentX==0 && fineX!=0) || ((currentX+fineX)&0x7)==0 )) && (PPUMASK&0x8)>0){
			//we need to scroll right...
			//The top one is c
			nametable=vRam[0x2000|(currentAddress&0x0fff)];
			//printf("0x%04X\n",BackSheet(PPUCTRL)|(nametable<<4)+((currentScanline)&0x7));
			tileBMPLow=vRam[BackSheet(PPUCTRL)|(nametable<<4)+((currentScanline)&0x7)];
			tileBMPHigh=vRam[BackSheet(PPUCTRL)|(nametable<<4)+((currentScanline)&0x7)+0x8];
			attribute=vRam[0x23C0|(currentAddress&0x0C00)|((currentAddress>>4)&0x38)|((currentAddress>>2)&0x07)];
			attribute=attributeFind(0x2000|(currentAddress&0xfff),attribute);
			if(currentX==0 && fineX!=0){
				offsetX=7-fineX;
			}else{
				offsetX=7;
			}
			//printf("B:0x%04X\n",currentAddress);
			//exit(0);
			if((currentAddress&0x001F)==0x001F){
				currentAddress&=(~0x001F);
				currentAddress^=0x0400;		
				//printf("SWH %i\n",currentX);				
			}else{
				++currentAddress;
			}
			//currentAddress=(currentAddress&0x0FFF)|0x2000;
			//printf("A:0x%04X\n",currentAddress);
		}
		pdex=(attribute|(((tileBMPHigh&(1<<(offsetX)))>>(offsetX))<<1)|((tileBMPLow>>(offsetX))&0x1));
		//printf("%02X\n",pdex);
		//s0Hit means that sprite 0 could hit something, not that it has.
		outputColor=0;
		//printf("0x%02X\n",pdex);
		if((pdex&0x3)==0){
			backColor=Colors[vRam[0x3F00]];
		}else{
			backColor=Colors[vRam[0x3F00+pdex]];	
		}		
		outputColor=backColor;
		char sprites=currentSpriteNums-1;
		
		for(;sprites>=0;--sprites){
			if((currentSpriteData[(sprites<<2)+3]<=currentX && currentSpriteData[(sprites<<2)+3]+8>currentX) ){
				//printf("Sprite\n");
				if((currentX<8 && (PPUMASK&0x4)==0)){
					continue;
				}
				//printf("%02X %02X\n",currentSpriteData[(sprites<<2)+1],currentSpriteData[(sprites<<2)+2]);
				spriteDex=currentX-currentSpriteData[(sprites<<2)+3];
				if((PPUCTRL&0x20)>0){
					int addr=0,check=0;
					int sheet=((currentSpriteData[(sprites<<2)+1])&1)>0 ? 0x1000:0x0;
					//printf("S %04X\n",sheet|currentSpriteData[(sprites<<2)+1]);printf("S %04X\n",sheet|currentSpriteData[(sprites<<2)+1]);
					if((currentSpriteData[(sprites<<2)+2]&0x80)>0){
						//addr=sheet+(((currentSpriteData[(sprites<<2)+1]&0xFE)<<4)+ (7-(currentScanline-currentSpriteData[(sprites<<2)]-1)&0x7));
						//printf("S %04X\n",addr);							
						//addr=sheet+(((currentSpriteData[(sprites<<2)+1]&0xFE)<<4)+ ((currentScanline-currentSpriteData[(sprites<<2)]-1)&0x7));
						addr=sheet+(((currentSpriteData[(sprites<<2)+1]&0xFE)<<4)+ (7-(currentScanline-currentSpriteData[(sprites<<2)]-1)&0x7));
						check=currentScanline-currentSpriteData[(sprites<<2)]-1>=8;
					}else{
						addr=sheet+(((currentSpriteData[(sprites<<2)+1]&0xFE)<<4)+ ((currentScanline-currentSpriteData[(sprites<<2)]-1)&0x7));
						check=(currentScanline-currentSpriteData[(sprites<<2)]-1<8);	
					}
					spriteBMPLow=(check) ? vRam[addr]:vRam[addr+0x10];
					spriteBMPHigh=(check) ? vRam[addr+0x8]:vRam[addr+0x18];
				}else{
					int addr=0;
					if((currentSpriteData[(sprites<<2)+2]&0x80)>0){
						addr=SpriteSheet(PPUCTRL)|((currentSpriteData[(sprites<<2)+1])<<4)|(7-(currentScanline-currentSpriteData[(sprites<<2)]-1));			
					}else{
						addr=SpriteSheet(PPUCTRL)|((currentSpriteData[(sprites<<2)+1])<<4)|((currentScanline-currentSpriteData[(sprites<<2)]-1));
					}					
					spriteBMPLow=vRam[addr];
					spriteBMPHigh=vRam[addr+0x8];		
				}	
				sAttrib=(currentSpriteData[(sprites<<2)+2]&0x3)<<2;
				//flip horizontal
				if((currentSpriteData[(sprites<<2)+2]&0x40)>0){
					sdex=(sAttrib| (((spriteBMPHigh>>(spriteDex))&0x1)<<1) |((spriteBMPLow>>(spriteDex))&0x1));
				}
				//no horizontal flip.
				else{
					sdex=(sAttrib| (((spriteBMPHigh>>(7-spriteDex))&0x1)<<1) |((spriteBMPLow>>(7-spriteDex))&0x1));
				}				
				spriteColor=Colors[vRam[0x3F10+sdex]];
				if(sprites==0 && (sdex&0x3)>0 && (pdex&0x3)>0 && (PPUMASK&0x18)>0 && s0>0){
					PPUSTATUS|=0x40;
					//printf("OH %i %i\n",currentX,currentScanline);
				}
				if((PPUMASK&0x10)==0){
					break;
				}
				if( ((((currentSpriteData[(sprites<<2)+2])&0x20)==0 && (sdex&0x3)!=0)) || ((sdex&0x3)!=0 && (pdex&0x3)==0) ){
					outputColor=spriteColor;
				}
			}
		}	
		if(currentX<=8 && (PPUMASK&0x2)==0){
			outputColor=Colors[0xF];
		}
		char tempx=0,tempy=0;
		for(tempx=0;tempx<2;++tempx){
			for(tempy=0;tempy<2;++tempy){
				*((Uint32*)(pixel+(((currentScanline-8)*scale+tempy) * screen->pitch) + ((currentX*scale+tempx) * sizeof(Uint32))))=outputColor;
			}
		}
		--offsetX;
		++currentX;
		
	}else if((PPUMASK&0x18)==0 && currentScanline>7 && currentScanline<232){
		char tempx=0,tempy=0;
		for(tempx=0;tempx<2;++tempx){
			for(tempy=0;tempy<2;++tempy){
				*((Uint32*)(pixel+(((currentScanline-8)*scale+tempy) * screen->pitch) + ((currentX*scale+tempx) * sizeof(Uint32))))=Colors[0xF];
			}
		}
	} 
	else if(currentScanline==261){
		if(currentCycles==1){
			PPUSTATUS&=0x1F;
			PPUSTATUS|=(ioLine&0x1F);
			//printf("ADD 0x%04X 0x%04X\n",currentAddress,tempAddress);
		}
		if(currentCycles>280 && currentCycles<305 && (PPUMASK&0x18)>0){				
			currentAddress=(currentAddress&(0x041F))|(tempAddress&(~0x041F));
			//currentAddress=tempAddress;
			//printf("CC: 0x%04X\n",currentAddress);
		}
	}
	if(currentScanline==241 && currentCycles==1){
		PPUSTATUS|=0x80;
		if(PPUCTRL&0x80){			
			PPUCTRL&=0x7f;
			cpu->Interrupt(0);
		}
	}
	++currentCycles;	
	if(currentCycles==256 && (PPUMASK&0x18)>0){
		currentX=0;
		offsetX=7-fineX;
		if((currentAddress&0x7000)!=0x7000){
			currentAddress+=0x1000;
		}else{
			currentAddress&=(~0x7000);
			unsigned char tempY=(currentAddress&0x03E0)>>5;
			if(tempY==29){
				tempY=0;
				currentAddress^=0x0800;
			}else if(tempY==31){
				tempY=0;
			}else{
				++tempY;
			}
			currentAddress=(currentAddress&(~0x03E0))|(tempY<<5);
		}
		currentAddress=(currentAddress&(~0x041F))|(tempAddress&(0x041F));
	}
	if((PPUMASK&0x18)>0 && currentCycles>327 && currentCycles<340){
			currentAddress=(currentAddress&(~0x041F))|(tempAddress&(0x041F));
			//printf("REST 0x%04X\n",currentAddress);
	}
	
	
	if(currentCycles==341){
		currentCycles=0;
		++currentScanline;
		evaled=0;
		memcpy(currentSpriteData,secondOAM,0x4*currentSprites);
		currentSpriteNums=currentSprites;
		currentSprites=0;
		s0=s0Hit;
		s0Hit=0;
		//fineX=0xff;
		//The shifting issue was caused by the writing not having enought time to write in vblank and the process overwriting the address that it was actively using.
		if(currentScanline==262){
			currentScanline=0;
	//printf("END 0x%04X\n",currentAddress);
		}
	}
}

//each is a 16×16 pixels or 2 tiles by 2 tiles in the attribute table.
signed char PPU::execute(unsigned long cycles){
	//check cycles here.
}


