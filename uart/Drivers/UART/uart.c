/*******************************************************************************

    UART��������
    ����STM32 HAL�� �� freeRTOS
    �ṩ����ʽ�����շ��ӿ�

*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "uart.h"
#include "circular_queue.h"

/* Typedef -------------------------------------------------------------------*/

typedef struct
{
    UART_HandleTypeDef *huart;  //hal��Ĵ���ָ��
    uint8_t is_half_duplex; //�Ƿ��˫��
    
    //�������
    osSemaphoreId_t *pRx_sem; //�����ź���
    uint8_t rx_temp;    //������ʱ����
    queue_type rx_queue;    //���ջ�����
    
    //�������
    osSemaphoreId_t *pTx_sem; //�����ź���
    uint8_t tx_temp;    //������ʱ����
    queue_type tx_queue;    //���ͻ�����
}uart_type;

/* Extern variable prototypes ------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern osSemaphoreId_t UART1_txCplt_BinarySemHandle;
extern osSemaphoreId_t UART1_rxCplt_BinarySemHandle;
extern osSemaphoreId_t UART2_txCplt_BinarySemHandle;
extern osSemaphoreId_t UART2_rxCplt_BinarySemHandle;
extern osSemaphoreId_t UART3_txCplt_BinarySemHandle;
extern osSemaphoreId_t UART3_rxCplt_BinarySemHandle;
extern osSemaphoreId_t UART4_txCplt_BinarySemHandle;
extern osSemaphoreId_t UART4_rxCplt_BinarySemHandle;
extern osSemaphoreId_t UART5_txCplt_BinarySemHandle;
extern osSemaphoreId_t UART5_rxCplt_BinarySemHandle;

/* Private variables ---------------------------------------------------------*/
static uart_type uart_profile[ UART_NUM ] = 
{
    [ UART_1 ] =
    {
        .huart = &huart1,
        .pRx_sem = &UART1_rxCplt_BinarySemHandle,
        .pTx_sem = &UART1_txCplt_BinarySemHandle,
    },
    [ UART_2 ] = 
    {
        .huart = &huart2,
        .pRx_sem = &UART2_rxCplt_BinarySemHandle,
        .pTx_sem = &UART2_txCplt_BinarySemHandle,
    },
    [ UART_3 ] =
    {
        .huart = &huart3,
        .pRx_sem = &UART3_rxCplt_BinarySemHandle,
        .pTx_sem = &UART3_txCplt_BinarySemHandle,
    },
    [ UART_4 ] = 
    {
        .huart = &huart4,
        .pRx_sem = &UART4_rxCplt_BinarySemHandle,
        .pTx_sem = &UART4_txCplt_BinarySemHandle,
    },
    [ UART_5 ] =
    {
        .huart = &huart5,
        .pRx_sem = &UART5_rxCplt_BinarySemHandle,
        .pTx_sem = &UART5_txCplt_BinarySemHandle,
    },
};


/* Exported functions --------------------------------------------------------*/
/*******************************************************************************
    ���ڷ��ͺ���
 @param: 
        eUARTx      ���ں�
        msg         �����ͱ���
        length      ���ĳ���
        timeout_ms  ���ͳ�ʱʱ�䣬��λ�����롣
                    Ϊ0ʱ�����ȴ�������ɣ����Ŀ��������ͻ�������ֱ�ӷ��ء�
                    ��Ϊ0ʱ���ȴ�������ɣ�ֱ����ʱ��
 @return:
        < 0 ����ʧ�ܡ��������󣬻��ĳ��ȳ���������ʣ��ռ�
        = 0 ���ͳɹ���ֻ�ǽ����ĳɹ����������ͻ�����
        > 0 ���ͳɹ��������Ѿ�ȫ���������
*******************************************************************************/
int uart_send( uart_enum eUARTx, const uint8_t msg[], uint16_t length, uint16_t timeout_ms )
{
    uint16_t available_length = 0;
    uart_type *pUart = 0;
    
    if ( eUARTx >= UART_NUM
        || 0 == msg
            || 0 == length )
    {
        return -1;
    }
    pUart = &uart_profile[ eUARTx ];
    
    //��鷢�ͻ������Ŀ��ÿռ䣬���ռ䲻�����򷵻ش���
    available_length = get_queue_available_length( &pUart->tx_queue );
    if ( length > available_length )
    {
        return -1;
    }
    
    //�����Ŀ��������ͻ�����
    for ( int i = 0; i < length; i++ )
    {
        enqueue( &pUart->tx_queue, msg[ i ] );
    }
    
    //�����ڷ��ʹ��ڿ���״̬����������
    if ( HAL_UART_STATE_READY == pUart->huart->gState )
    {
        if ( dequeue( &pUart->tx_queue, &pUart->tx_temp ) >= 0 )
        {
            HAL_UART_Transmit_IT( pUart->huart, &pUart->tx_temp, 1 );
        }
    }
    
    //���г�ʱ��Ϊ0����ȴ�������ɣ�ֱ����ʱ
    if ( 0 != timeout_ms )
    {
        osSemaphoreAcquire( *pUart->pTx_sem, 0 );   //��������ź���
        if ( osOK == osSemaphoreAcquire( *pUart->pTx_sem, timeout_ms ) )
        {
            return 1;
        }
    }
    return 0;
}
/*******************************************************************************
    ���ڽ��պ���
 @param: 
        eUARTx      ���ں�
        buffer      ���Ļ�����
        buffer_size ��������С
        timeout_ms  ���ճ�ʱʱ�䣬��λ�����롣��timeout_ms��δ�յ��κα��ģ���ʱ����
        interval_ms ֡�䳬ʱʱ�䣬��λ�����롣���1���ֽں�interval_ms��δ�յ����ֽڣ���ʱ����
 @return:
         < 0 ��������
        >= 0 ���յ����ֽ���
*******************************************************************************/
int uart_recv( uart_enum eUARTx, uint8_t *pBuffer, uint16_t buffer_size, uint16_t timeout_ms, uint16_t interval_ms )
{
    uint16_t received_length = 0;   //�յ��ı��ĳ���
    uart_type *pUart = 0;
    
    if ( eUARTx >= UART_NUM
        || 0 == pBuffer
            || 0 == buffer_size )
    {
        return -1;
    }
    pUart = &uart_profile[ eUARTx ];
    
    //�ӻ�������������
    while ( received_length < buffer_size )
    {
        if ( dequeue( &pUart->rx_queue, &pBuffer[ received_length ] ) < 0 ) //���ջ������Ϊ��
        {
            
            HAL_UART_Receive_IT( pUart->huart, &pUart->rx_temp, 1 );
            break;
        }
        received_length++;
    }
    
    //�ȴ��ظ����ģ�ֱ����ʱ
    if ( 0 == received_length )
    {
        osSemaphoreAcquire( *pUart->pRx_sem, 0 );   //��������ź���
        if ( osOK != osSemaphoreAcquire( *pUart->pRx_sem, timeout_ms ) )  //δ�ȵ��ź���
        {
            return 0;
        }
    }
    
    //���б��ģ��ȴ�֡��������
    while ( received_length < buffer_size )
    {
        if ( dequeue( &pUart->rx_queue, &pBuffer[ received_length ] ) < 0 )
        {
            if ( osOK != osSemaphoreAcquire( *pUart->pRx_sem, interval_ms ) )  //δ�ȵ��ź���
            {
                break;
            }
        }
        else
        {
            received_length++;
        }
    }
    return received_length;
}

//��ȡuart_profile��ָ��
static uart_type *get_uart_profile_point( UART_HandleTypeDef *huart )
{
    uart_type *pUart = 0;
    
    for ( int i = 0; i < UART_NUM; i++ )
    {
        pUart = &uart_profile[ i ];
        if ( huart == pUart->huart )
        {
            return pUart;
        }
    }
    return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_type *pUart = 0;
    
    pUart = get_uart_profile_point( huart );
    if ( 0 == pUart )
    {
        return;
    }
    if ( dequeue( &pUart->tx_queue, &pUart->tx_temp ) < 0 )
    {
        //ȫ��������ɣ��ͷŷ����ź���
        osSemaphoreRelease( *pUart->pTx_sem );
        return;
    }
    HAL_UART_Transmit_IT( pUart->huart, &pUart->tx_temp, 1 );
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_type *pUart = 0;
    
    pUart = get_uart_profile_point( huart );
    if ( 0 == pUart )
    {
        return;
    }
    enqueue( &pUart->rx_queue, pUart->rx_temp );
    osSemaphoreRelease( *pUart->pRx_sem );  //���յ����ֽڣ��ͷŽ����ź���
    HAL_UART_Receive_IT( pUart->huart, &pUart->rx_temp, 1 );
}

