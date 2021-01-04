#include <gba.h>
#include <math.h>
#include <stdio.h>

#include "left_bin.h"
#include "right_bin.h"

#define SAMPLE_RATE    32768
#define CPU_CLOCK      16777216
#define REFRESH_CYCLES 280896.0f

#define TRACK_LENGTH   ((left_bin_size + right_bin_size) / 2)

s32 samplesLeft;

void playSound()
{
	// full track
	samplesLeft = TRACK_LENGTH;
	
	// enable sound
	SNDSTAT = SNDSTAT_ENABLE;
	
	// set up direct sound(tm) control
	DSOUNDCTRL = DSOUNDCTRL_A100 | DSOUNDCTRL_B100 |
				 DSOUNDCTRL_AL | DSOUNDCTRL_ATIMER(0) | DSOUNDCTRL_ARESET |
				 DSOUNDCTRL_BR | DSOUNDCTRL_BTIMER(0) | DSOUNDCTRL_BRESET ;
	
	// set up left channel dma
	REG_DMA1SAD = (u32)left_bin;
	REG_DMA1DAD = (u32)&REG_FIFO_A;
	REG_DMA1CNT = DMA_ENABLE | DMA32 | DMA_SPECIAL | DMA_REPEAT | DMA_SRC_INC | DMA_DST_INC;
	
	// set up right channel dma
	REG_DMA2SAD = (u32)right_bin;
	REG_DMA2DAD = (u32)&REG_FIFO_B;
	REG_DMA2CNT = DMA_ENABLE | DMA32 | DMA_SPECIAL | DMA_REPEAT | DMA_SRC_INC | DMA_DST_INC;
	
	// set timer 0
	REG_TM0CNT_L = -(CPU_CLOCK / SAMPLE_RATE);
	REG_TM0CNT_H = TIMER_START;
}

void stopSound()
{
	// no samples left to play
	samplesLeft = 0;
	
	// disable dma 1 + 2 and timer 0
	REG_TM0CNT_H = !TIMER_START;
	REG_DMA1CNT  = !DMA_ENABLE;
	REG_DMA2CNT  = !DMA_ENABLE;
	
	// disable sound to save battery
	SNDSTAT = !SNDSTAT_ENABLE;
}

void irqHandler()
{
	// scan keys
	scanKeys();

	// ensure timer 0 enabled
	if (REG_TM0CNT_H & 0x80)
	{
		// subtract (approximately) the number of samples played between every vblank
		samplesLeft -= (u32)round(SAMPLE_RATE / (CPU_CLOCK / REFRESH_CYCLES));
		
		// stop sound if exhausted
		if (samplesLeft <= 0)
			stopSound();
	}
}

int main()
{
	u32 keys;
	
	// init irq and handle vblank
	irqInit();
	irqSet(IRQ_VBLANK, irqHandler);
	irqEnable(IRQ_VBLANK);
	
	// init console
	consoleDemoInit();
	
	// print prompt
	iprintf("\x1b[3;1HSimonTime's near CD quality");
	iprintf("\x1b[4;8HGBA audio demo");
	iprintf("\x1b[10;4HPress A to play music");
	iprintf("\x1b[11;4HPress B to stop music");
	
	for (;;)
	{
		// wait for vblank
		VBlankIntrWait();
		
		// get pressed keys
		keys = keysDown();
		
		// play sound if A button pressed and not currently playing
		if ((keys & KEY_A) && !samplesLeft)
			playSound();
		
		// stop sound if B button pressed
		if (keys & KEY_B)
			stopSound();
	}
}