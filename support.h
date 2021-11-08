/*    
    Copyright (c) 2021 EIIT S.A.

    @file          support.h
    @project       ATE.2000210
    @version       2.1.0
    @date          July 2021
    @author        Rachid Souky
*/

#include <stdint.h>

/***** Parameters *****/

#ifndef PC_BAUDRATE
#define PC_BAUDRATE    115200                   //Baudrate in bps
#endif

#ifndef PC_TX_BUFFER_SIZE
#define PC_TX_BUFFER_SIZE    255                //Buffer size in bytes
#endif

#ifndef PC_RX_BUFFER_SIZE
#define PC_RX_BUFFER_SIZE    255                //Buffer size in bytes
#endif

#ifndef PC_RX_ENDCHAR
#define PC_RX_ENDCHAR    0x0A                //Char
#endif

#ifndef MLB_BAUDRATE
#define MLB_BAUDRATE    1000000                //Baudrate in bps
#endif

#ifndef MLB_TX_BUFFER_SIZE
#define MLB_TX_BUFFER_SIZE    255              //Buffer size in bytes
#endif

#ifndef MLB_RX_BUFFER_SIZE
#define MLB_RX_BUFFER_SIZE    255              //Buffer size in bytes
#endif

#ifndef WATCHDOG_TIMEOUT                        
#define WATCHDOG_TIMEOUT   5.0                  //Timeout interval in seconds
#endif

#ifndef MAINLOOP_WAIT                        
#define MAINLOOP_WAIT   1000                    //Timeout interval in milliseconds
#endif

#ifndef READLOOP_WAIT                        
#define READLOOP_WAIT   1                         //Timeout interval in milliseconds
#endif

#ifndef MAILMAIN_MAX_SIZE
#define MAILMAIN_MAX_SIZE   16                    //Max size in number of elements
#endif

#ifndef MAILCMD_DATA_SIZE
#define MAILCMD_DATA_SIZE   100                    //Data size in bytes
#endif

#ifndef BULK_DATA_SIZE
#define BULK_DATA_SIZE   255                    //Max size in number of elements
#endif

/***** Function prototypes *****/

unsigned char reverse_byte(unsigned char x);
uint16_t calc_crc16_ccitt_false(char *array, unsigned int arraySize);
uint16_t set_mlbID_parity(uint16_t mlbID);
unsigned int count_set_bits(unsigned int n);
void compose_melibu_full_frame(char *array, unsigned int arraySize, unsigned int slaveAdd);
void compose_melibu_header_frame(char *array, unsigned int arraySize, unsigned int slaveAdd);
