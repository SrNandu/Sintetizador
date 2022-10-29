/*
 * config.c
 *
 *  Created on: 28 oct. 2022
 *      Author: ferminverdolini
 */
#include "Config.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpdma.h"
#include <cr_section_macros.h>

// Funcion que configura los puertos
void cfgPines(){

	// Se configuran los pines y las interrupciones de las 13 teclas del piano
	PINSEL_CFG_Type pincfg;
	pincfg.Portnum = 0; // Puerto 0
	pincfg.Funcnum = 0; // Funcion GPIO
	pincfg.Pinmode = PINSEL_PINMODE_PULLUP; // Resistencias de pull up
	for(int i = 0; i<12; i++){
		pincfg.Pinnum = i;
		PINSEL_ConfigPin(&pincfg); // Se configuran los pines del 0 al 11
	}
	pincfg.Pinnum = 15; // Se configura la 13ra tecla, puerto 15
	PINSEL_ConfigPin(&pincfg);
	GPIO_IntCmd(0,(0xFF || (1<<15)),0); // Se habilitan interrupciones por flanco de subida y de bajada
	GPIO_IntCmd(0,(0xFF || (1<<15)),1);

	// Se configuran las teclas de control de ondas y de cambio de octavas
	pincfg.Portnum = 2; // Puerto 2
	pincfg.Pinnum = 1; // Funcion EINT
	pincfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	pincfg.Pinmode = PINSEL_PINMODE_PULLUP; // Resistencias de pull up
	for(int i = 10;i<14; i++){
		pincfg.Pinnum = i;
		PINSEL_ConfigPin(&pincfg);
	}
	// Se configuran las interrupciones externas TODO(ver de cambiar por manejo de bytes)
	EXTI_InitTypeDef extcfg;
	extcfg.EXTI_Mode = EXTI_MODE_LEVEL_SENSITIVE;
	extcfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
	extcfg.EXTI_Line = EXTI_EINT0;
	EXTI_Config(&extcfg);
	extcfg.EXTI_Line = EXTI_EINT1;
	EXTI_Config(&extcfg);
	extcfg.EXTI_Line = EXTI_EINT2;
	EXTI_Config(&extcfg);
	extcfg.EXTI_Line = EXTI_EINT3;
	EXTI_Config(&extcfg);

}

// Funcion que configura el DMA
void cfgDAC(){
	DAC_Init(LPC_DAC);

	// Se configura el DAC
	DAC_CONVERTER_CFG_Type daccfg;
	daccfg.CNT_ENA = 1; // Se habilita el timeout counter
	daccfg.DBLBUF_ENA = 0; // Se deshabilita el doble buffereo
	daccfg.DMA_ENA = 1; // Se habilita las request a DMA
	DAC_ConfigDAConverterControl(LPC_DAC,&daccfg);

}

// Funcion que configura el DMA y prend el DMA
void cfgDMA(uint16_t sgnInicial[],GPDMA_LLI_Type *listaDma){
	GPDMA_Init();
	// Se inicializa el DMA apuntando a la lista de la señal rectangula
	listaDma->NextLLI = (uint32_t) &listaDma;
	listaDma->SrcAddr = (uint32_t) sgnInicial;
	listaDma->DstAddr = (uint32_t) &(LPC_DAC ->DACR);
	listaDma->Control = TRANSFERSIZE | (1 << 18) | ( 1 << 21);

	GPDMA_Channel_CFG_Type dmacfg;
	dmacfg.ChannelNum = 0;
	dmacfg.TransferSize = TRANSFERSIZE;
	dmacfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	dmacfg.SrcMemAddr = (uint32_t) listaDma->SrcAddr;
	dmacfg.DstConn = GPDMA_CONN_DAC;
	dmacfg.DMALLI = (uint32_t) &listaDma;

	GPDMA_Setup(&dmacfg);
	GPDMA_ChannelCmd(0,DISABLE);
	// Se desactiva la interrupcion para que el dac no comience a pedir datos

	// Se configura el puerto 0.26 como AOUT
		PINSEL_CFG_Type dac;
		dac.Portnum = 0;
		dac.Pinnum = 26;
		dac.Pinmode = 2;
		dac.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&dac); // Se prende el DAC
}
void makeSignals(uint16_t signals[][TRANSFERSIZE]){
	uint16_t i;
	uint16_t pendiente = DACSIZE/TRANSFERSIZE;

	for(i=0;i<TRANSFERSIZE;i++){
		if(i<TRANSFERSIZE/2){
			signals[SGNRECT][i]=0;
			signals[SGNTRIANG][i]=i*pendiente*2;
		}else{
			signals[SGNRECT][i]=DACSIZE-1;
			signals[SGNTRIANG][i]=(TRANSFERSIZE-i-1)*pendiente*2;
		}
		signals[SGNSIERRA][i]=i*pendiente;
	}
}
