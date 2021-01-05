/*******************************************************************************

    Ä¬ÈÏÈÎÎñ

*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "uart.h"

/* Exported functions --------------------------------------------------------*/
void default_task()
{
    int length = 0;
    uint8_t buffer[ 10 ] = {1};
    for (;;)
    {
        length = uart_recv( UART_2, buffer, sizeof(buffer), 10, 0 );
        if ( length > 0 )
        {
            length = uart_send( UART_4, buffer, length, 10 );
        }
        osDelay(1);
    }
}

void my_task()
{
    int length = 0;
    uint8_t buffer[ 10 ] = {1};
    for (;;)
    {
        length = uart_recv( UART_4, buffer, sizeof(buffer), 10, 0 );
        if ( length > 0 )
        {
            length = uart_send( UART_2, buffer, length, 10 );
        }
        osDelay(1);
    }
}
