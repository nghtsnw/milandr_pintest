#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <stdint.h>

#define VE93 // Указываем версию микросхемы. Допустимые варианты: VE91, VE92, VE93. Так же не забыть поменять устройство в свойствах проекта.
#define RS485_EN // Говорим программе что работаем через RS485, т.е. будет задейстована нога контроллера для включения передатчика на микросхеме RS485. Если не нужно то закомментировать.

#ifdef RS485_EN
#define RS485_PORT_TX MDR_PORTE // Соответственно определяем порт
#define RS485_PinTX PORT_Pin_3 // и пин на этом порту, к которому подключен вход активации передатчика.
#endif

#define USE_UART1 // Указываем какой модуль UART контроллера используем - первый или второй (USE_UART1 или USE_UART2).
#define UART_RATE 19200 // Скорость обмена. В клиенте на ПК стоит такая же.

// Далее секция установок пинов rx tx выбранного uart. Для определения порта, пина и функции порта следует обратиться к документации на микроконтроллер и принципиальной схеме устройства в котором всё стоит.
#ifdef USE_UART1
#define UART_X						MDR_UART1
#define UART_IRQ					UART1_IRQn
#define UART_CLOCK 				RST_CLK_PCLK_UART1

#define UART_CLOCK_TX 		RST_CLK_PCLK_PORTB
#define UART_CLOCK_RX 		RST_CLK_PCLK_PORTB
		
#define UART_PORT_TX			MDR_PORTB
#define UART_PORT_PinTX		PORT_Pin_5
#define UART_PORT_FuncTX  PORT_FUNC_ALTER
		
#define UART_PORT_RX			MDR_PORTB	
#define UART_PORT_PinRX		PORT_Pin_6
#define UART_PORT_FuncRX  PORT_FUNC_ALTER
	
#elif defined USE_UART2
#define UART_X						MDR_UART2
#define UART_IRQ					UART2_IRQn
#define UART_CLOCK 				RST_CLK_PCLK_UART2	

#define UART_CLOCK_TX 		RST_CLK_PCLK_PORTF
#define UART_CLOCK_RX 		RST_CLK_PCLK_PORTF
	
#define UART_PORT_TX			MDR_PORTF
#define UART_PORT_PinTX		PORT_Pin_1
#define UART_PORT_FuncTX  PORT_FUNC_OVERRID
	
#define UART_PORT_RX			MDR_PORTF	
#define UART_PORT_PinRX		PORT_Pin_0
#define UART_PORT_FuncRX  PORT_FUNC_OVERRID

#endif

#endif // _GLOBAL_H
