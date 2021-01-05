/*******************************************************************************

    UART驱动代码
    基于STM32 HAL库 和 freeRTOS
    提供阻塞式串口收发接口

*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "uart.h"
#include "circular_queue.h"

/* Typedef -------------------------------------------------------------------*/

typedef struct
{
    UART_HandleTypeDef *huart;  //hal库的串口指针
    uint8_t is_half_duplex; //是否半双工
    
    //接收相关
    osSemaphoreId_t *pRx_sem; //接收信号量
    uint8_t rx_temp;    //接收临时变量
    queue_type rx_queue;    //接收缓冲区
    
    //发送相关
    osSemaphoreId_t *pTx_sem; //发送信号量
    uint8_t tx_temp;    //发送临时变量
    queue_type tx_queue;    //发送缓冲区
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
    串口发送函数
 @param: 
        eUARTx      串口号
        msg         待发送报文
        length      报文长度
        timeout_ms  发送超时时间，单位：毫秒。
                    为0时，不等待发送完成，报文拷贝到发送缓冲区后直接返回。
                    不为0时，等待发送完成，直到超时。
 @return:
        < 0 发送失败。参数错误，或报文长度超过缓冲区剩余空间
        = 0 发送成功。只是将报文成功拷贝到发送缓冲区
        > 0 发送成功，并且已经全部发送完成
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
    
    //检查发送缓冲区的可用空间，若空间不够，则返回错误
    available_length = get_queue_available_length( &pUart->tx_queue );
    if ( length > available_length )
    {
        return -1;
    }
    
    //将报文拷贝到发送缓冲区
    for ( int i = 0; i < length; i++ )
    {
        enqueue( &pUart->tx_queue, msg[ i ] );
    }
    
    //若串口发送处于空闲状态，启动发送
    if ( HAL_UART_STATE_READY == pUart->huart->gState )
    {
        if ( dequeue( &pUart->tx_queue, &pUart->tx_temp ) >= 0 )
        {
            HAL_UART_Transmit_IT( pUart->huart, &pUart->tx_temp, 1 );
        }
    }
    
    //若有超时不为0，则等待发送完成，直到超时
    if ( 0 != timeout_ms )
    {
        osSemaphoreAcquire( *pUart->pTx_sem, 0 );   //清除发送信号量
        if ( osOK == osSemaphoreAcquire( *pUart->pTx_sem, timeout_ms ) )
        {
            return 1;
        }
    }
    return 0;
}
/*******************************************************************************
    串口接收函数
 @param: 
        eUARTx      串口号
        buffer      报文缓冲区
        buffer_size 缓冲区大小
        timeout_ms  接收超时时间，单位：毫秒。若timeout_ms内未收到任何报文，超时返回
        interval_ms 帧间超时时间，单位：毫秒。最后1个字节后interval_ms内未收到新字节，超时返回
 @return:
         < 0 参数错误
        >= 0 接收到的字节数
*******************************************************************************/
int uart_recv( uart_enum eUARTx, uint8_t *pBuffer, uint16_t buffer_size, uint16_t timeout_ms, uint16_t interval_ms )
{
    uint16_t received_length = 0;   //收到的报文长度
    uart_type *pUart = 0;
    
    if ( eUARTx >= UART_NUM
        || 0 == pBuffer
            || 0 == buffer_size )
    {
        return -1;
    }
    pUart = &uart_profile[ eUARTx ];
    
    //从缓冲区拷贝报文
    while ( received_length < buffer_size )
    {
        if ( dequeue( &pUart->rx_queue, &pBuffer[ received_length ] ) < 0 ) //接收缓冲队列为空
        {
            
            HAL_UART_Receive_IT( pUart->huart, &pUart->rx_temp, 1 );
            break;
        }
        received_length++;
    }
    
    //等待回复报文，直到超时
    if ( 0 == received_length )
    {
        osSemaphoreAcquire( *pUart->pRx_sem, 0 );   //清除接收信号量
        if ( osOK != osSemaphoreAcquire( *pUart->pRx_sem, timeout_ms ) )  //未等到信号量
        {
            return 0;
        }
    }
    
    //若有报文，等待帧间间隔结束
    while ( received_length < buffer_size )
    {
        if ( dequeue( &pUart->rx_queue, &pBuffer[ received_length ] ) < 0 )
        {
            if ( osOK != osSemaphoreAcquire( *pUart->pRx_sem, interval_ms ) )  //未等到信号量
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

//获取uart_profile的指针
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
        //全部发送完成，释放发送信号量
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
    osSemaphoreRelease( *pUart->pRx_sem );  //接收到新字节，释放接收信号量
    HAL_UART_Receive_IT( pUart->huart, &pUart->rx_temp, 1 );
}

