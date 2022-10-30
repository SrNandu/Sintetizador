#include "LPC17xx.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpdma.h"
#include <cr_section_macros.h>

#include "Const.h"

#ifndef CONFIG_H_
#define CONFIG_H_

void cfgPines();
void cfgDAC();
void cfgDMA(uint16_t sgnInicial[],GPDMA_LLI_Type *listaDma);
void cfgNVIC();
void cfgADC();
void cfgTIM0();
void makeSignals(uint16_t signals[][TRANSFERSIZE]);


#endif /* CONFIG_H_ */
