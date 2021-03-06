/* Copyright 2011-2016 Tyler Gilbert; 
 * This file is part of Stratify OS.
 *
 * Stratify OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Stratify OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Stratify OS.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * 
 */

#include "core_local.h"
#include "lpc_local.h"


extern void mcu_set_sleep_mode(int * level);
#ifdef __lpc17xx__
extern LPC_GPDMACH_Type * mcu_dma_get_channel(uint32_t chan);
#endif

int mcu_core_user_sleep(core_sleep_t level){

#ifdef __lpc17xx__
	int tmp;

	LPC_GPDMACH_Type * channel_regs;

	//Wait for any DMA transactions on RAM to complete (AHB transactions are OK in idle but not deep sleep)
	for(tmp = 0; tmp < DMA_MAX_CHANNEL; tmp++){
		if ( LPC_GPDMA->EnbldChns & (1<<tmp) ){
			channel_regs = mcu_dma_get_channel(tmp);
			if ( level >= CORE_DEEPSLEEP ){
				if ( (channel_regs->CDestAddr <= 0x10001FFF) ||
						(channel_regs->CSrcAddr <= 0x10001FFF) ){

					//wait for DMA to complete
					while(  LPC_GPDMA->EnbldChns & (1<<tmp) ){
						//this will trip the WDT if it takes too long
					}
				}
			} else {
				//wait for DMA to complete
				while(  LPC_GPDMA->EnbldChns & (1<<tmp) ){
					//this will trip the WDT if it takes too long
				}
			}
		}
	}
#endif

	cortexm_svcall((cortexm_svcall_t)mcu_set_sleep_mode, &level);
	if ( level < 0 ){
		return level;
	}

	//Wait for an interrupt
	__WFI();
	return 0;
}

void mcu_core_prepare_deepsleep(int level){
	if( level >= CORE_DEEPSLEEP ){
#if defined LPCXX7X_8X
		LPC_SC->CLKSRCSEL = 0; //Use IRC
		LPC_SC->CCLKSEL = 1; //Use sysclk with no divider
#endif
		//disable PLL and use IRC
		MCU_CLR_BIT(LPC_SC->PLL0CON, PLLE0);
		LPC_SC->PLL0FEED = 0xAA;
		LPC_SC->PLL0FEED = 0x55;
	}
}

void mcu_core_recover_deepsleep(int level){

	if( MCU_TEST_BIT(LPC_SC->PCON, 8) ){
		//deep sleep
		MCU_SET_BIT(LPC_SC->PCON, 8);
	}

	if( MCU_TEST_BIT(LPC_SC->PCON, 9) ){
		//deep sleep
		MCU_SET_BIT(LPC_SC->PCON, 9);
	}

	mcu_core_initclock(1);
	mcu_core_setusbclock(mcu_board_config.core_osc_freq); //set the USB clock
}

