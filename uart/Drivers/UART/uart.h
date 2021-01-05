/*******************************************************************************

    UART驱动代码
    基于STM32 HAL库 和 freeRTOS
    提供阻塞式串口收发接口

*******************************************************************************/

#pragma once

/* Defines -------------------------------------------------------------------*/
#define UART_BUFFER_SIZE    256 //串口缓冲区大小

/* Enum ----------------------------------------------------------------------*/
typedef enum
{
    UART_1  = 0,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_NUM,
}uart_enum;

/* Exported functions --------------------------------------------------------*/
int uart_send( uart_enum eUARTx, const uint8_t msg[], uint16_t length, uint16_t timeout_ms );
int uart_recv( uart_enum eUARTx, uint8_t *pBuffer, uint16_t buffer_size, uint16_t timeout_ms, uint16_t interval_ms );

