/*    
    Copyright (c) 2021 EIIT S.A.

    @file          stripLed.h
    @project       ATE.2000210
    @version       2.2.0
    @date          July 2021
    @author        Rachid Souky
*/

#include "support.h"

/***** Frames *****/

#ifdef __cplusplus
extern "C" {
#endif
extern const char stl_factMode[10];
extern const char stl_rgbPWM[40];
extern const char stl_rgbLUV[40];
extern const char stl_sync[10];
extern const char stl_progMode[10];
extern const char stl_moveMemPointer[10];
extern const char stl_readEEPROM[10];
extern const char stl_readResponse12[2];
extern const char stl_modData_0x4[14];
extern const char stl_modData_0x5[14];
extern const char stl_modData_0x6[14];
extern const char stl_modData_0x7[14];
extern const char stl_modData_0x8[14];
extern const char stl_modData_0x9[14];
extern const char stl_modData_0xA[14];
extern const char stl_modData_0xB[14];
extern const char stl_modData_0xC[14];
extern const char stl_modData_0xD[14];
#ifdef __cplusplus
}
#endif

/***** Function prototypes *****/

void compose_rgbPWM100_frame(char *array, unsigned int arraySize,
    unsigned int slaveAdd, unsigned int ledNum, unsigned int rgbVal);
void compose_rgbLUV100_frame(char *array, unsigned int arraySize,
    unsigned int slaveAdd, unsigned int ledNum, unsigned int L, unsigned int u, unsigned int v);
void compose_memPointer_frame(char *array, unsigned int arraySize,
    unsigned int slaveAdd, unsigned int memAddress);