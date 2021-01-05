/*******************************************************************************

    

*******************************************************************************/
#pragma once

#include "uart.h"
#define QUEUE_SIZE      UART_BUFFER_SIZE    //队列大小

/* 循环缓冲队列
    head == tail时，队列为空
    (head + 1) % (QUEUE_SIZE + 1) == tail时，队列为满
*/
typedef struct
{
    uint8_t buffer[ QUEUE_SIZE + 1 ]; //缓冲数组
    uint16_t head;  //队头
    uint16_t tail;  //队尾
}queue_type;

int enqueue( queue_type *pQueue, uint8_t value );
int dequeue( queue_type *pQueue, uint8_t *pValue );
int get_queue_length( queue_type *pQueue );
int get_queue_available_length( queue_type *pQueue );
