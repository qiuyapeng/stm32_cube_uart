/*******************************************************************************

    

*******************************************************************************/
#pragma once

#include "uart.h"
#define QUEUE_SIZE      UART_BUFFER_SIZE    //���д�С

/* ѭ���������
    head == tailʱ������Ϊ��
    (head + 1) % (QUEUE_SIZE + 1) == tailʱ������Ϊ��
*/
typedef struct
{
    uint8_t buffer[ QUEUE_SIZE + 1 ]; //��������
    uint16_t head;  //��ͷ
    uint16_t tail;  //��β
}queue_type;

int enqueue( queue_type *pQueue, uint8_t value );
int dequeue( queue_type *pQueue, uint8_t *pValue );
int get_queue_length( queue_type *pQueue );
int get_queue_available_length( queue_type *pQueue );
