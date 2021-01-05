/*******************************************************************************

    循环队列
    先入先出。采用不完全充满方式。

*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "circular_queue.h"

/* Exported functions --------------------------------------------------------*/
/*******************************************************************************
    入队函数
 @param: 
        pQueue      队列
        value       需要入队的值
 @return:
        < 0 入队失败
        = 0 入队成功
*******************************************************************************/
int enqueue( queue_type *pQueue, uint8_t value )
{
    if ( (pQueue->tail + 1) % (QUEUE_SIZE + 1) == pQueue->head )   //队列已满
    {
        return -1;
    }
    pQueue->buffer[ pQueue->tail ] = value;
    pQueue->tail = (pQueue->tail + 1) % (QUEUE_SIZE + 1);
    return 0;
}
/*******************************************************************************
    出队函数
 @param: 
        pQueue      队列
        pValue      出队的值
 @return:
        < 0 出队失败
        = 0 出队成功
*******************************************************************************/
int dequeue( queue_type *pQueue, uint8_t *pValue )
{
    if ( pQueue->head == pQueue->tail ) //队列已空
    {
        return -1;
    }
    *pValue = pQueue->buffer[ pQueue->head ];
    pQueue->head = (pQueue->head + 1) % (QUEUE_SIZE + 1);
    return 0;
}


/*******************************************************************************
    获取队列长度
 @param: 
        pQueue      队列
 @return:
        队列的长度
*******************************************************************************/
int get_queue_length( queue_type *pQueue )
{
    return (pQueue->tail + QUEUE_SIZE + 1 - pQueue->head) % (QUEUE_SIZE + 1);
}

/*******************************************************************************
    获取队列长度
 @param: 
        pQueue      队列
 @return:
        队列的剩余空间
*******************************************************************************/
int get_queue_available_length( queue_type *pQueue )
{
    uint16_t queue_length = 0;
    
    queue_length = get_queue_length( pQueue );
    return QUEUE_SIZE - queue_length;
}
