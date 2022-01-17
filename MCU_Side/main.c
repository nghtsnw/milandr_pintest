#include <MDR32F9Qx_uart.h>
#include <MDR32F9Qx_port.h>
#include "Clock.h"
#include "Uart.h"
#include "global.h"

signed int delay = 100000000;

uint16_t receivedData;
uint8_t rxBufCount = 0;
uint8_t rxBuf[5]; //[FF, код операции, порт, пин, хэш]
const uint8_t rxBufSize = 5;
uint8_t startRxFlag;
uint8_t startRxReboot = 10;
uint8_t rxOk = 0;
void toRxBuf(uint16_t);
void readPinState(void);
uint8_t calcCrc(const uint8_t *arr, const uint8_t size);
void doTask(void);
void toggleSet(MDR_PORT_TypeDef*, uint32_t);
uint16_t toggleTimeDiv = 0;
MDR_PORT_TypeDef *tPort;
uint32_t tPin;
void toggleTask(void);
uint16_t watchDogCount;
void collectTxData(void);
uint8_t txData[16];
uint8_t txDataDebug[16];
const uint8_t txDataSize = 16;
void sendTxData(void);
uint8_t uartBysy = 0;
void UART_send_byte(uint8_t);
uint8_t tmpcrc;
uint8_t tmpcrc2;
uint8_t togglePermission = 0;
int transmitCount;
void setAsOut(MDR_PORT_TypeDef*, uint16_t);

// Переменные для хранения статуса выводов портов
uint16_t porta;
uint16_t portb;
uint16_t portc;
uint16_t portd;
uint16_t porte;
uint16_t portf;

uint16_t porttmp;

#ifdef VE91
const uint8_t milandrIndex = 0x01;
const uint8_t pinMap[6][16] = // port, pin enable or disable
    {{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

#elif defined VE92
const uint8_t milandrIndex = 0x02;
const uint8_t pinMap[6][16] = // port, pin enable or disable
    {{1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0}};
		
#elif defined VE93
const uint8_t milandrIndex = 0x03;
const uint8_t pinMap[6][16] = // port, pin enable or disable
    {{1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},{1,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0}};

#endif

//	Тактовая частота ядра
#define CPU_FREQ HSE_Value * PLL_MUL	// 8MHz * 1

#define RS485_TX_ON		PORT_SetBits(RS485_PORT_TX, RS485_PinTX)
#define RS485_TX_OFF	PORT_ResetBits(RS485_PORT_TX, RS485_PinTX)

int main(void)
{    
  delay = 1000000; // Предпусковая задержка
  while(--delay) __nop();		
//	txDataSize = sizeof(txData) / sizeof(txData[0]);
//	rxBufSize = sizeof(rxBuf) / sizeof(rxBuf[0]);
	Clock_Init_HSE_PLL(PLL_MUL - 1); //	Тактирование ядра от внешнего кварца (HSE)		
	
	#define TimerTick CPU_FREQ/10 // Количество тактов процессора до прерывания
	SysTick_Config(TimerTick); // Инициализация таймера SysTick
	
	UART_Initialize(UART_RATE); //	Инициализация всех портов как входы с подтяжкой к питанию, и UART
	UART_InitIRQ(2);
	
	while (1)
  {
    if (watchDogCount >= 35){
			UART_Cmd (UART_X, DISABLE);
			UART_DeinitFunc();
			watchDogCount = 0;
			UART_Initialize(UART_RATE); // Если три секунды не было сброса переменной - сбрасывается конфигурация портов
			UART_InitIRQ(1);
			rxBufCount = 0;
			startRxFlag = 1;
			UART_Cmd (UART_X, ENABLE);
		}
		
		if (rxOk){
		collectTxData();
		sendTxData();
		rxOk = 0;
		}
 }
}

void toRxBuf(uint16_t word)
{
	if (word == 0xFF && uartBysy == 0){
		startRxFlag = 1;
		rxBufCount = 0;
	}
	if (startRxFlag)
	{					
				rxBuf[rxBufCount] = word;
				if (rxBufCount < rxBufSize) rxBufCount++;
		if (rxBufCount >= 5) // Выглядит хреново, но с else почему-то не входило в тело, оставлю пока что так 
				{
					rxBufCount = 0;
					startRxFlag = 0;
					tmpcrc = (calcCrc(rxBuf, rxBufSize));
					tmpcrc2 = (rxBuf[4]); //(rxBuf[rxBufSize-1]);
					if (tmpcrc == tmpcrc2)	
					{
						doTask();
						rxOk = 1;						
					}
					else // Если crc не прошло то нужно обнулить счётчик и некоторое время не читать данные
					{ // Как вариант то поднять startRxFlag в systick таймере где нибудь через треть секунды						
						rxOk = 0;
						startRxReboot = 0;
					}
				}
	}
}

void doTask()
{
	NVIC_DisableIRQ(UART1_IRQn);
	NVIC_DisableIRQ(SysTick_IRQn);
	// Код действия в первом байте:
	// 01 - установить как вход с подтяжкой к питанию
	// 02 - установить как выход
	// 03 - выход в состояние 1
	// 04 - выход в состояние 0
	// 05 - моргать выходом с частотой 1Hz
	// A1 - сброс watchdog, если долго нету (отсутсвие связи например) то контроллер переводит все порты в режим входов 
	static PORT_InitTypeDef GPIOInitStruct; // Структура для инициализации линий ввода-вывода
	
	
	static MDR_PORT_TypeDef  *MDR_PORT_X;
	     if (rxBuf[2] == 0x00) MDR_PORT_X = MDR_PORTA;
	else if (rxBuf[2] == 0x01) MDR_PORT_X = MDR_PORTB;
	else if (rxBuf[2] == 0x02) MDR_PORT_X = MDR_PORTC;
	else if (rxBuf[2] == 0x03) MDR_PORT_X = MDR_PORTD;
	else if (rxBuf[2] == 0x04) MDR_PORT_X = MDR_PORTE;
	else if (rxBuf[2] == 0x05) MDR_PORT_X = MDR_PORTF;
	
	static uint16_t PORT_Pin_X;
	if (rxBuf[2] == 0x00) PORT_Pin_X = PORT_Pin_0;
	else if (rxBuf[3] == 0x01) PORT_Pin_X = PORT_Pin_1;
	else if (rxBuf[3] == 0x02) PORT_Pin_X = PORT_Pin_2;
	else if (rxBuf[3] == 0x03) PORT_Pin_X = PORT_Pin_3;
	else if (rxBuf[3] == 0x04) PORT_Pin_X = PORT_Pin_4;
	else if (rxBuf[3] == 0x05) PORT_Pin_X = PORT_Pin_5;
	else if (rxBuf[3] == 0x06) PORT_Pin_X = PORT_Pin_6;
	else if (rxBuf[3] == 0x07) PORT_Pin_X = PORT_Pin_7;
	
	switch (rxBuf[1])
	{
		case 0x01: // 01 - установить как вход с подтяжкой к питанию
			togglePermission = 0;
			PORT_StructInit (&GPIOInitStruct);
			// Инициализация входа с подтяжкой к питанию
			GPIOInitStruct.PORT_SPEED = PORT_SPEED_SLOW;
			GPIOInitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
			GPIOInitStruct.PORT_FUNC  = PORT_FUNC_PORT;	
			GPIOInitStruct.PORT_OE    = PORT_OE_IN;
			GPIOInitStruct.PORT_PULL_UP = PORT_PULL_UP_ON;
			GPIOInitStruct.PORT_Pin   = PORT_Pin_X;
			PORT_Init (MDR_PORT_X, &GPIOInitStruct);
			break;
		
		case 0x02: // 02 - установить как выход					
//			PORT_StructInit (&GPIOInitStruct);			
//			// Инициализация выхода
//			GPIOInitStruct.PORT_SPEED = PORT_SPEED_SLOW;
//			GPIOInitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
//			GPIOInitStruct.PORT_FUNC  = PORT_FUNC_PORT;	
//			GPIOInitStruct.PORT_OE    = PORT_OE_OUT;
//			GPIOInitStruct.PORT_PULL_UP = PORT_PULL_UP_OFF;
//			GPIOInitStruct.PORT_Pin   = PORT_Pin_X;			
//			PORT_Init (MDR_PORT_X, &GPIOInitStruct);
//			togglePermission = 0;
//			PORT_ResetBits(MDR_PORT_X, PORT_Pin_X);
			break;
		
		case 0x03: // 03 - выход в состояние 1
			setAsOut(MDR_PORT_X, PORT_Pin_X);
			togglePermission = 0;
			PORT_SetBits(MDR_PORT_X, PORT_Pin_X);
			break;
		
		case 0x04: // 04 - выход в состояние 0
			setAsOut(MDR_PORT_X, PORT_Pin_X);
			togglePermission = 0;
			PORT_ResetBits(MDR_PORT_X, PORT_Pin_X);
			break;
		
		case 0x05: // 05 - моргать выходом с частотой 1Hz
			setAsOut(MDR_PORT_X, PORT_Pin_X);
			toggleSet(MDR_PORT_X, PORT_Pin_X);
			togglePermission = 1;
			break;
		
		case 0xA1: // A1 - сброс watchdog, если долго нету (отсутсвие связи) то контроллер переводит все порты в режим входов (в main())
			watchDogCount = 0;
			break;
			
		default: break;
	}
	NVIC_EnableIRQ(UART1_IRQn);
	NVIC_EnableIRQ(SysTick_IRQn);
}

void setAsOut(MDR_PORT_TypeDef  *MDR_PORT_X, uint16_t PORT_Pin_X)
{
			static PORT_InitTypeDef GPIOInitStruct;
			PORT_StructInit (&GPIOInitStruct);			
			// Инициализация выхода
			GPIOInitStruct.PORT_SPEED = PORT_SPEED_SLOW;
			GPIOInitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
			GPIOInitStruct.PORT_FUNC  = PORT_FUNC_PORT;	
			GPIOInitStruct.PORT_OE    = PORT_OE_OUT;
			GPIOInitStruct.PORT_PULL_UP = PORT_PULL_UP_OFF;
			GPIOInitStruct.PORT_Pin   = PORT_Pin_X;			
			PORT_Init (MDR_PORT_X, &GPIOInitStruct);
			togglePermission = 0;
			PORT_ResetBits(MDR_PORT_X, PORT_Pin_X);
}

void collectTxData()
{
	txData[0] = 0xFF;
	txData[1] = 0xFF;
	txData[2] = milandrIndex;
	txData[3] = porta;
	txData[4] = porta >> 8;
	txData[5] = portb;
	txData[6] = portb >> 8;
	txData[7] = portc;
	txData[8] = portc >> 8;
	txData[9] = portd;
	txData[10] = portd >> 8;
	txData[11] = porte;
	txData[12] = porte >> 8;
	txData[13] = portf;
	txData[14] = portf >> 8;
	NVIC_DisableIRQ(UART1_IRQn);
	NVIC_DisableIRQ(SysTick_IRQn);
	txData[15] = calcCrc(txData, txDataSize);
	NVIC_EnableIRQ(UART1_IRQn);
	NVIC_EnableIRQ(SysTick_IRQn);
}

void readPinState() // Заполнение массивов со статусами ножек контроллера, можно было бы сделать проще через RXTX
{
	porttmp = MDR_PORTA->RXTX;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_0) == 1) porta |= 0x0001; else porta &= ~0x0001;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_1) == 1) porta |= 0x0002; else porta &= ~0x0002;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_2) == 1) porta |= 0x0004; else porta &= ~0x0004;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_3) == 1) porta |= 0x0008; else porta &= ~0x0008;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_4) == 1) porta |= 0x0010; else porta &= ~0x0010;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_5) == 1) porta |= 0x0020; else porta &= ~0x0020;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_6) == 1) porta |= 0x0040; else porta &= ~0x0040;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_7) == 1) porta |= 0x0080; else porta &= ~0x0080;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_8) == 1) porta |= 0x0100; else porta &= ~0x0100;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_9) == 1) porta |= 0x0200; else porta &= ~0x0200;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_10) == 1) porta |= 0x0400; else porta &= ~0x0400;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_11) == 1) porta |= 0x0800; else porta &= ~0x0800;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_12) == 1) porta |= 0x1000; else porta &= ~0x1000;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_13) == 1) porta |= 0x2000; else porta &= ~0x2000;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_14) == 1) porta |= 0x4000; else porta &= ~0x4000;
	if (PORT_ReadInputDataBit (MDR_PORTA, PORT_Pin_15) == 1) porta |= 0x8000; else porta &= ~0x8000;
	
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_0) == 1) portb |= 0x0001; else portb &= ~0x0001;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_1) == 1) portb |= 0x0002; else portb &= ~0x0002;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_2) == 1) portb |= 0x0004; else portb &= ~0x0004;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_3) == 1) portb |= 0x0008; else portb &= ~0x0008;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_4) == 1) portb |= 0x0010; else portb &= ~0x0010;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_5) == 1) portb |= 0x0020; else portb &= ~0x0020;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_6) == 1) portb |= 0x0040; else portb &= ~0x0040;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_7) == 1) portb |= 0x0080; else portb &= ~0x0080;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_8) == 1) portb |= 0x0100; else portb &= ~0x0100;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_9) == 1) portb |= 0x0200; else portb &= ~0x0200;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_10) == 1) portb |= 0x0400; else portb &= ~0x0400;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_11) == 1) portb |= 0x0800; else portb &= ~0x0800;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_12) == 1) portb |= 0x1000; else portb &= ~0x1000;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_13) == 1) portb |= 0x2000; else portb &= ~0x2000;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_14) == 1) portb |= 0x4000; else portb &= ~0x4000;
	if (PORT_ReadInputDataBit (MDR_PORTB, PORT_Pin_15) == 1) portb |= 0x8000; else portb &= ~0x8000;
	
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_0) == 1) portc |= 0x0001; else portc &= ~0x0001;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_1) == 1) portc |= 0x0002; else portc &= ~0x0002;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_2) == 1) portc |= 0x0004; else portc &= ~0x0004;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_3) == 1) portc |= 0x0008; else portc &= ~0x0008;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_4) == 1) portc |= 0x0010; else portc &= ~0x0010;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_5) == 1) portc |= 0x0020; else portc &= ~0x0020;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_6) == 1) portc |= 0x0040; else portc &= ~0x0040;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_7) == 1) portc |= 0x0080; else portc &= ~0x0080;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_8) == 1) portc |= 0x0100; else portc &= ~0x0100;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_9) == 1) portc |= 0x0200; else portc &= ~0x0200;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_10) == 1) portc |= 0x0400; else portc &= ~0x0400;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_11) == 1) portc |= 0x0800; else portc &= ~0x0800;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_12) == 1) portc |= 0x1000; else portc &= ~0x1000;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_13) == 1) portc |= 0x2000; else portc &= ~0x2000;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_14) == 1) portc |= 0x4000; else portc &= ~0x4000;
	if (PORT_ReadInputDataBit (MDR_PORTC, PORT_Pin_15) == 1) portc |= 0x8000; else portc &= ~0x8000;
	
	
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_0) == 1) portd |= 0x0001; else portd &= ~0x0001;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_1) == 1) portd |= 0x0002; else portd &= ~0x0002;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_2) == 1) portd |= 0x0004; else portd &= ~0x0004;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_3) == 1) portd |= 0x0008; else portd &= ~0x0008;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_4) == 1) portd |= 0x0010; else portd &= ~0x0010;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_5) == 1) portd |= 0x0020; else portd &= ~0x0020;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_6) == 1) portd |= 0x0040; else portd &= ~0x0040;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_7) == 1) portd |= 0x0080; else portd &= ~0x0080;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_8) == 1) portd |= 0x0100; else portd &= ~0x0100;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_9) == 1) portd |= 0x0200; else portd &= ~0x0200;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_10) == 1) portd |= 0x0400; else portd &= ~0x0400;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_11) == 1) portd |= 0x0800; else portd &= ~0x0800;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_12) == 1) portd |= 0x1000; else portd &= ~0x1000;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_13) == 1) portd |= 0x2000; else portd &= ~0x2000;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_14) == 1) portd |= 0x4000; else portd &= ~0x4000;
	if (PORT_ReadInputDataBit (MDR_PORTD, PORT_Pin_15) == 1) portd |= 0x8000; else portd &= ~0x8000;
	
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_0) == 1) porte |= 0x0001; else porte &= ~0x0001;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_1) == 1) porte |= 0x0002; else porte &= ~0x0002;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_2) == 1) porte |= 0x0004; else porte &= ~0x0004;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_3) == 1) porte |= 0x0008; else porte &= ~0x0008;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_4) == 1) porte |= 0x0010; else porte &= ~0x0010;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_5) == 1) porte |= 0x0020; else porte &= ~0x0020;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_6) == 1) porte |= 0x0040; else porte &= ~0x0040;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_7) == 1) porte |= 0x0080; else porte &= ~0x0080;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_8) == 1) porte |= 0x0100; else porte &= ~0x0100;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_9) == 1) porte |= 0x0200; else porte &= ~0x0200;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_10) == 1) porte |= 0x0400; else porte &= ~0x0400;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_11) == 1) porte |= 0x0800; else porte &= ~0x0800;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_12) == 1) porte |= 0x1000; else porte &= ~0x1000;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_13) == 1) porte |= 0x2000; else porte &= ~0x2000;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_14) == 1) porte |= 0x4000; else porte &= ~0x4000;
	if (PORT_ReadInputDataBit (MDR_PORTE, PORT_Pin_15) == 1) porte |= 0x8000; else porte &= ~0x8000;
	
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_0) == 1) portf |= 0x0001; else portf &= ~0x0001;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_1) == 1) portf |= 0x0002; else portf &= ~0x0002;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_2) == 1) portf |= 0x0004; else portf &= ~0x0004;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_3) == 1) portf |= 0x0008; else portf &= ~0x0008;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_4) == 1) portf |= 0x0010; else portf &= ~0x0010;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_5) == 1) portf |= 0x0020; else portf &= ~0x0020;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_6) == 1) portf |= 0x0040; else portf &= ~0x0040;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_7) == 1) portf |= 0x0080; else portf &= ~0x0080;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_8) == 1) portf |= 0x0100; else portf &= ~0x0100;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_9) == 1) portf |= 0x0200; else portf &= ~0x0200;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_10) == 1) portf |= 0x0400; else portf &= ~0x0400;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_11) == 1) portf |= 0x0800; else portf &= ~0x0800;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_12) == 1) portf |= 0x1000; else portf &= ~0x1000;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_13) == 1) portf |= 0x2000; else portf &= ~0x2000;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_14) == 1) portf |= 0x4000; else portf &= ~0x4000;
	if (PORT_ReadInputDataBit (MDR_PORTF, PORT_Pin_15) == 1) portf |= 0x8000; else portf &= ~0x8000;
}


uint8_t calcCrc(const uint8_t *arr, const uint8_t arrSize)
{
	static uint8_t crc;
	crc = 0;
	for (int i = 0; i < arrSize-1; i++) crc += arr[i];
	return crc;
}


void sendTxData(void){
    transmitCount = 0;
    if (uartBysy == 0){
    uartBysy = 1;
    } 
		UART_ITConfig(UART_X,UART_IT_RX, DISABLE);
		#ifdef RS485_EN
		RS485_TX_ON; 
		#endif	
		
    while(transmitCount <= 15){ // Скармливаем весь массив в цикле    
    UART_send_byte(txData[transmitCount]);
			transmitCount++;
		}
		
		#ifdef RS485_EN
		RS485_TX_OFF; 
		#endif
		UART_ClearITPendingBit( UART_X, UART_IT_RX);
		UART_ITConfig(UART_X, UART_IT_RX, ENABLE);
    uartBysy = 0;
}

void UART_send_byte(uint8_t byte)
{
	NVIC_DisableIRQ(SysTick_IRQn);
	UART_SendData(UART_X, byte);
	// Костыли для миландров
	while (UART_X->FR & UART_FLAG_BUSY);
	UART_ReceiveData (UART_X); 
	UART_ClearITPendingBit(UART_X, UART_IT_RX);
	NVIC_EnableIRQ(SysTick_IRQn);
	
}

void SysTick_Handler(void) // 100 мс
{	
	if (toggleTimeDiv >=10){ // Это для моргания раз в секунду выбранной ножкой
		toggleTask();
		toggleTimeDiv = 0;
	}		
	readPinState();		
	watchDogCount++;
	toggleTimeDiv++;
	
	if (startRxReboot < 10) startRxReboot++; // Перерыв в разрешении читать уарт, если данные пришли битые
	else startRxFlag = 1; // По окончании перерыва поднимаем разрешение читать уарт
}

void toggleSet(MDR_PORT_TypeDef *MDR_PORT_Xt, uint32_t MDR_Pin_Xt)
{
	tPort = MDR_PORT_Xt;
	tPin = MDR_Pin_Xt;
}

void toggleTask()
{
	if (togglePermission)
	{
		if (PORT_ReadInputDataBit (tPort, tPin) == 1) PORT_ResetBits(tPort, tPin);
		else PORT_SetBits(tPort, tPin);	
	}
}

void UART1_IRQHandler(void)
	{  
   if (UART_GetITStatusMasked (UART_X, UART_IT_RX) == SET){
     UART_ClearITPendingBit (UART_X, UART_IT_RX);		
		 if (startRxFlag && !uartBysy) toRxBuf(UART_ReceiveData(UART_X));
    }
  if (UART_GetITStatusMasked(UART_X, UART_IT_TX) == SET){ // Сброс прерывания по TX
    UART_ClearITPendingBit (UART_X, UART_IT_TX);
  }	
}

void UART2_IRQHandler(void)
{  
   if (UART_GetITStatusMasked (UART_X, UART_IT_RX) == SET){
     if (startRxFlag && !uartBysy) toRxBuf(UART_ReceiveData(UART_X)); 
		 UART_ClearITPendingBit (UART_X, UART_IT_RX);		  
    }
  if (UART_GetITStatusMasked(UART_X, UART_IT_TX) == SET){ // Сброс прерывания по TX
    UART_ClearITPendingBit (UART_X, UART_IT_TX);
  }	
}
