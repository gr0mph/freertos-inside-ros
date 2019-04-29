/*
* GitHub from gr0mph
* https://github.com/gr0mph
*	portable --> FreeRTOS inside GNU Linux ROS
 */

#ifndef PORT_H
#define PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------*/

/*
 * IRQ from counter
 *  External function
 */
void vPortSysTickHandler( int sig );

/*
 *  Start the program
 */
void vStartFreeRTOS( void );

#ifdef __cplusplus
}
#endif

#endif /* PORT_H */
