/*******************************************************************************

    ѭ������
    �����ȳ������ò���ȫ������ʽ��

*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "circular_queue.h"

/* Exported functions --------------------------------------------------------*/
/*******************************************************************************
    ��Ӻ���
 @param: 
        pQueue      ����
        value       ��Ҫ��ӵ�ֵ
 @return:
        < 0 ���ʧ��
        = 0 ��ӳɹ�
*******************************************************************************/
int enqueue( queue_type *pQueue, uint8_t value )
{
    if ( (pQueue->tail + 1) % (QUEUE_SIZE + 1) == pQueue->head )   //��������
    {
        return -1;
    }
    pQueue->buffer[ pQueue->tail ] = value;
    pQueue->tail = (pQueue->tail + 1) % (QUEUE_SIZE + 1);
    return 0;
}
/*******************************************************************************
    ���Ӻ���
 @param: 
        pQueue      ����
        pValue      ���ӵ�ֵ
 @return:
        < 0 ����ʧ��
        = 0 ���ӳɹ�
*******************************************************************************/
int dequeue( queue_type *pQueue, uint8_t *pValue )
{
    if ( pQueue->head == pQueue->tail ) //�����ѿ�
    {
        return -1;
    }
    *pValue = pQueue->buffer[ pQueue->head ];
    pQueue->head = (pQueue->head + 1) % (QUEUE_SIZE + 1);
    return 0;
}


/*******************************************************************************
    ��ȡ���г���
 @param: 
        pQueue      ����
 @return:
        ���еĳ���
*******************************************************************************/
int get_queue_length( queue_type *pQueue )
{
    return (pQueue->tail + QUEUE_SIZE + 1 - pQueue->head) % (QUEUE_SIZE + 1);
}

/*******************************************************************************
    ��ȡ���г���
 @param: 
        pQueue      ����
 @return:
        ���е�ʣ��ռ�
*******************************************************************************/
int get_queue_available_length( queue_type *pQueue )
{
    uint16_t queue_length = 0;
    
    queue_length = get_queue_length( pQueue );
    return QUEUE_SIZE - queue_length;
}
