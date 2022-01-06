#ifndef _CLOCK_H
#define _CLOCK_H


void Clock_Init_HSE_PLL(uint32_t PLL_Mul);

#ifdef USE_MDR1986VE1
void Clock_Init_HSE_dir(void);
#endif

#endif  //_CLOCK_H
