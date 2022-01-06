#include <MDR32F9Qx_port.h>
#include <MDR32F9Qx_rst_clk.h>
#include <MDR32F9Qx_uart.h>

#include "Uart.h"


// Инициализация модуля UART и портов
void UART_Initialize (uint32_t uartBaudRate)
{
  UART_DeinitFunc();
  
  // Структура для инициализации линий ввода-вывода
  PORT_InitTypeDef GPIOInitStruct;

  // Структура для инициализации модуля UART
  UART_InitTypeDef UARTInitStruct;

  // Разрешение тактирования всех портов и модуля UART1 (PORT B)
  RST_CLK_PCLKcmd (UART_CLOCK | UART_CLOCK_TX | UART_CLOCK_RX | RST_CLK_PCLK_PORTA | RST_CLK_PCLK_PORTB 
		| RST_CLK_PCLK_PORTC | RST_CLK_PCLK_PORTD | RST_CLK_PCLK_PORTE | RST_CLK_PCLK_PORTF, ENABLE);

  // Общая конфигурация линий ввода-вывода
  PORT_StructInit (&GPIOInitStruct);
  GPIOInitStruct.PORT_SPEED = PORT_SPEED_MAXFAST;
  GPIOInitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
	
	// Инициализация всех ножек портов как входов
  GPIOInitStruct.PORT_SPEED = PORT_SPEED_SLOW;
  GPIOInitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
  GPIOInitStruct.PORT_FUNC  = PORT_FUNC_PORT;	
  GPIOInitStruct.PORT_OE    = PORT_OE_IN;
	GPIOInitStruct.PORT_PULL_UP = PORT_PULL_UP_ON;
  GPIOInitStruct.PORT_Pin   = PORT_Pin_All;
  PORT_Init (MDR_PORTA, &GPIOInitStruct);
	PORT_Init (MDR_PORTB, &GPIOInitStruct);
	PORT_Init (MDR_PORTC, &GPIOInitStruct);
	PORT_Init (MDR_PORTD, &GPIOInitStruct);
	PORT_Init (MDR_PORTE, &GPIOInitStruct);
	PORT_Init (MDR_PORTF, &GPIOInitStruct);

  // Конфигурация и инициализация линии для приема данных 
	GPIOInitStruct.PORT_SPEED = PORT_SPEED_MAXFAST;
	GPIOInitStruct.PORT_PULL_UP = PORT_PULL_UP_OFF;
	
	GPIOInitStruct.PORT_FUNC  = UART_PORT_FuncRX;
  GPIOInitStruct.PORT_OE    = PORT_OE_IN;
  GPIOInitStruct.PORT_Pin   = UART_PORT_PinRX;
  PORT_Init (UART_PORT_RX, &GPIOInitStruct);	

  // Конфигурация и инициализация линии для передачи данных 
  GPIOInitStruct.PORT_FUNC  = UART_PORT_FuncTX;	
  GPIOInitStruct.PORT_OE    = PORT_OE_OUT;
  GPIOInitStruct.PORT_Pin   = UART_PORT_PinTX;
  PORT_Init (UART_PORT_TX, &GPIOInitStruct);  

	// Конфигурация ноги активации передатчика RS485 (если есть)
	#ifdef RS485_EN
	GPIOInitStruct.PORT_FUNC  = PORT_FUNC_PORT;	
  GPIOInitStruct.PORT_OE    = PORT_OE_OUT;
  GPIOInitStruct.PORT_Pin   = RS485_PinTX;
  PORT_Init (RS485_PORT_TX, &GPIOInitStruct);  
	#endif
  
  // Конфигурация модуля UART
  UARTInitStruct.UART_BaudRate            = uartBaudRate;                  // Скорость передачи данных
  UARTInitStruct.UART_WordLength          = UART_WordLength8b;             // Количество битов данных в сообщении
  UARTInitStruct.UART_StopBits            = UART_StopBits1;                // Количество STOP-битов
  UARTInitStruct.UART_Parity              = UART_Parity_No;                // Контроль четности
  UARTInitStruct.UART_FIFOMode            = UART_FIFO_OFF;                 // Включение/отключение буфера
  UARTInitStruct.UART_HardwareFlowControl = UART_HardwareFlowControl_RXE   // Аппаратный контроль за передачей и приемом данных
                                          | UART_HardwareFlowControl_TXE;

  // Инициализация модуля UART
  UART_Init (UART_X, &UARTInitStruct);

  // Выбор предделителя тактовой частоты модуля UART
  UART_BRGInit (UART_X, UART_HCLKdiv1);


  // Выбор источников прерываний (прием и передача данных)
  UART_ITConfig (UART_X, UART_IT_RX | UART_IT_TX, ENABLE);

  // Разрешение работы модуля UART
  UART_Cmd (UART_X, ENABLE);
}

void UART_DeinitFunc(void)
{
    UART_DeInit (UART_X);
    NVIC_DisableIRQ (UART_IRQ);
}

void UART_InitIRQ(uint32_t priority) // priority = 1
{
  // Назначение приоритета аппаратного прерывания от UART
  NVIC_SetPriority (UART_IRQ, priority);

  // Разрешение аппаратных прерываний от UART
  NVIC_EnableIRQ (UART_IRQ);
}	


void UartSetBaud(uint32_t baudRate, uint32_t freqCPU)
{
	uint32_t divider = freqCPU / (baudRate >> 2);
	uint32_t CR_tmp = UART_X->CR;
	uint32_t LCR_tmp = UART_X->LCR_H;
	
//  while ( !(UART_X->FR & UART_FLAG_TXFE) ); // wait FIFO empty
	while ( (UART_X->FR & UART_FLAG_BUSY) ); // wait 

  UART_X->CR = 0;
  UART_X->IBRD = divider >> 6;
  UART_X->FBRD = divider & 0x003F;
  UART_X->LCR_H = LCR_tmp;
  UART_X->CR = CR_tmp;
}


// Обработка аппаратных прерываний модуля UART
/*void UARTx_IRQHandler (void)
{
  // Если произошло прерывание по завершении приема данных...
  if (UART_GetITStatusMasked (UART_X, UART_IT_RX) == SET)
  {
    // Сброс прерывания
    UART_ClearITPendingBit (UART_X, UART_IT_RX);

    // Делать что-то здесь

  }

  // Если произошло прерывание по завершении передачи данных...
  if (UART_GetITStatusMasked(UART_X, UART_IT_TX) == SET)
  {
    // Сброс прерывания
    UART_ClearITPendingBit (UART_X, UART_IT_TX);

    // Наделать снова

  }
}
*/
