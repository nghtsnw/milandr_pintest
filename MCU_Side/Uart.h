#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include "global.h"

void UART_Initialize (uint32_t uartBaudRate);
void UART_InitIRQ(uint32_t priority);
void UartSetBaud(uint32_t baudRate, uint32_t freqCPU);
void UART_DeinitFunc(void);

#endif // _UART_H
