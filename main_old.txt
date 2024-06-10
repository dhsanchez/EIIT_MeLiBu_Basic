/*    
    Copyright (c) 2021 EIIT S.A.

    @file          main.cpp
    @project       ATE.2000210
    @version       3.6.4
    @date          July 2021
    @author        Rachid Souky
*/

/*
- Incorporar tratamiento de errores.
- Documentar código.
- Deshabilitar periféricos que no se utilizan.
- Mejorar la verificación de la recepción del mensaje de PC.
- Controlar que el buffer de recepción o transmisión no se desborde.
- Controlar que la estructura Mail no se desborde.
*/

/*
Mail.ID -> 1 Byte
    * 0: Message from PC
    * 1: Message from MLB1
    * 2: Message from MLB2
Mail.Data -> Up to 92 bytes
    -> Message Function: Up to 3 bytes
        * MxC: Clear Tx/Rx Buffer of MLBx (x=1-2).
            Parameters: ClearBufferMask (0b01: Clear Tx Buffer, 0b10: Clear Rx Buffer) [1 byte].
        * MxW: Write Full Frame to MLBx (x=1-2). 
            Parameters: WriteDatasize [2 bytes], Message Separator (.) [1 byte], MeLiBu Message [up to 80 bytes].
        * MxH: Write Header Frame to MLBx (x=1-2).
            Command Parameters: WriteDatasize [2 bytes], ReadDatasize [2 bytes], ReadTimeout [3 bytes], Message Separator (.) [1 byte], MeLiBu Message [up to 4 bytes].
            Response Parameters: ActualReadDatasize [2 bytes], ActualReadTime [3 bytes], Message Separator (.) [1 byte], MeLiBu Message [limited by MLB Rx buffer size].
        * MxR: Read MLBx (x=1-2) Buffer.
            Command Parameters: ReadDatasize [2 bytes], ReadTimeout [3 bytes].
            Response Parameters: ReadDatasize [2 bytes], ReadTimeout [3 bytes], Message Separator (.) [1 byte], MeLiBu Message [limited by MLB Rx buffer size].
        * BCx: Bulk Clear Tx/Rx Buffer of MLBx (x=1-3, where 3 implies operating on ports 1 and 2).
            Parameters: WaitAfterNext_ms [2 bytes], ClearBufferMask (1->Tx, 2->Rx, 3->Tx/Rx) [1 byte].
        * BWx: Bulk Write Full Frame to MLBx (x=1-3, where 3 implies operating on ports 1 and 2).
            Parameters: WaitAfterNext_ms [2 bytes], WriteDatasize [2 bytes], Message Separator (.) [1 byte], MeLiBu Message [up to 80 bytes].
        * BHx: Bulk Write Header Frame to MLBx (x=1-3, where 3 implies operating on ports 1 and 2).
            Parameters: WaitAfterNext_ms [2 bytes], WriteDatasize [2 bytes], ReadDatasize [2 bytes], ReadTimeout [2 bytes], Message Separator (.) [1 byte], MeLiBu Message [up to 4 bytes].
            Response Parameters: ActualReadDatasize [2 bytes], ActualReadTime [2 bytes], Message Separator (.) [1 byte], MeLiBu Message [limited by MLB Rx buffer size].
        * BRx: Bulk Read MLBx (x=1-3, where 3 implies operating on ports 1 and 2).
            Command Parameters: WaitAfterNext_ms [2 bytes], ReadDatasize [2 bytes], ReadTimeout [2 bytes].
            Response Parameters: ActualReadDatasize [2 bytes], ActualReadTime [2 bytes], Message Separator (.) [1 byte], MeLiBu Message [limited by MLB Rx buffer size].
        * BI: Intialize Bulk variables.
        * BE: Execute Bulk operations.
        * V0: Verify communication.
    -> Special Message Function: Up to 8 bytes
        * Fxyz: Execute specific function operation.
            - x : function number (1-255). [2 bytes]
            - y : MLB mask (1-3). [1 byte]
            - z : parameter data. [Up to 4 bytes]

MeLiBu Message:
    -> MeLiBu ID: 4 bytes (ASCII Representation)
    -> MeLiBu DATA: up to 72 bytes (ASCII Representation)
    -> MeLiBu CRC: 4 bytes (ASCII Representation)
*/

#include "mbed.h"
#include "support.h"
#include "stripLed.h"
#include "ConfigFile.h"
#include "rtos.h"
#include "MODSERIAL.h"
#include "Watchdog.h"
#include <string>
#include <stdint.h>

typedef struct MailMainCmd {
    char Id;
    char Data[MAILCMD_DATA_SIZE];
} MailMainCmd_t;

typedef struct MeLiBuMsg {
    char ID[2];
    char DATA[36];   
    char CRC[2];
} MeLiBuMsg_t;

typedef struct BulkData {
    unsigned int OpType;                //0->ClearBuffer, 1->WriteFullFrame, 2->WriteHeaderFrame, 3->ReadBuffer
    unsigned int Port;
    unsigned int WaitAfterNext_ms;
    unsigned int WriteMsgSize;
    unsigned int ReadMsgSize;
    unsigned int ReadMsgTimeout_ms;
    unsigned int ClearBufferMask; 
    char MeLiBuMsg[40];
} BulkData_t;

Mail<MailMainCmd_t, MAILMAIN_MAX_SIZE> mailMain;

BulkData_t bulkData[BULK_DATA_SIZE];
unsigned int bulkDataSize;

DigitalOut ledMain(LED1), ledPcRx(LED2), ledMlb1Rx(LED3), ledMlb2Rx(LED4);
MODSERIAL pc(p28, p27, PC_TX_BUFFER_SIZE, PC_RX_BUFFER_SIZE);
//MODSERIAL pc(USBTX, USBRX, PC_TX_BUFFER_SIZE, PC_RX_BUFFER_SIZE);
MODSERIAL mlb1(p9, p10, MLB_TX_BUFFER_SIZE, MLB_RX_BUFFER_SIZE);
MODSERIAL mlb2(p13, p14, MLB_TX_BUFFER_SIZE, MLB_RX_BUFFER_SIZE);

void commEventHandler(void);
void pcRxSerialIntr(MODSERIAL_IRQ_INFO *info);
void loadMail(char id, char *data);
void write_Mlb_Full_Msg(char port, char *mlbMsg_raw, unsigned int dataSize);
void write_Mlb_Response_Msg(char port, char *mlbMsg_raw, unsigned int dataSize);
void read_Mlb_Full_Msg(char port, char *mlbMsg_raw, unsigned int dataSize, unsigned int timeout_ms, const char responseHeader[]);
void flush_Mlb_Buffer(char port, char clearMask);
bool detect_header_frame(char port, unsigned int header);

int main() {
    
    bulkDataSize = 0;
    
    Thread threadMain(osPriorityRealtime, DEFAULT_STACK_SIZE, NULL);
    Watchdog wd;
    
    pc.baud(PC_BAUDRATE);
    pc.format(8,SerialBase::None,1);
    pc.attach(&pcRxSerialIntr, MODSERIAL::RxAutoDetect);
    pc.autoDetectChar(PC_RX_ENDCHAR);
    
    mlb1.baud(MLB_BAUDRATE);
    mlb1.format(8,SerialBase::None,1);
    
    mlb2.baud(MLB_BAUDRATE);
    mlb2.format(8,SerialBase::None,1);
    
    threadMain.start(commEventHandler);
    //wd.Configure(WATCHDOG_TIMEOUT);
    
    while(true){
        ledMain = !ledMain;
        //wd.Service();
        Thread::wait(MAINLOOP_WAIT);   
    }
}

void commEventHandler(void) {
    while(true) {
        osEvent evt = mailMain.get(osWaitForever);
        if (evt.status == osEventMail) {
            MailMainCmd_t *mail = (MailMainCmd_t*)evt.value.p;
            switch (mail->Id) {
                case 0:                             //request from PC
                    switch(mail->Data[0]) {
                        case 'B':
                        {   
                            switch(mail->Data[1]) {
                                case 'E':
                                {   
                                    for(int i=0; i<bulkDataSize; i++) {
                                        switch(bulkData[i].OpType) {
                                            case 0:                             // Clear Buffer
                                            {
                                                if((bulkData[i].Port & 0x1) == 1)
                                                    flush_Mlb_Buffer('1', '0'+bulkData[i].ClearBufferMask);
                                                if((bulkData[i].Port & 0x2) == 2)
                                                    flush_Mlb_Buffer('2', '0'+bulkData[i].ClearBufferMask);
                                                break;
                                            }
                                            case 1:                             // Write Full Frame
                                            {
                                                if((bulkData[i].Port & 0x1) == 1)
                                                    write_Mlb_Full_Msg('1', &bulkData[i].MeLiBuMsg[0], bulkData[i].WriteMsgSize);
                                                if((bulkData[i].Port & 0x2) == 2)
                                                    write_Mlb_Full_Msg('2', &bulkData[i].MeLiBuMsg[0], bulkData[i].WriteMsgSize);
                                                break;
                                            }
                                            case 2:                             // Write Header Frame
                                            {
                                                char rMlbMsg_raw1[MLB_RX_BUFFER_SIZE], rMlbMsg_raw2[MLB_RX_BUFFER_SIZE];
                                                if((bulkData[i].Port & 0x1) == 1) {
                                                    write_Mlb_Full_Msg('1', &bulkData[i].MeLiBuMsg[0], bulkData[i].WriteMsgSize);
                                                    read_Mlb_Full_Msg('1', rMlbMsg_raw1, bulkData[i].ReadMsgSize, bulkData[i].ReadMsgTimeout_ms, "BR1%02x%02x.");
                                                }
                                                if((bulkData[i].Port & 0x2) == 2) {
                                                    write_Mlb_Full_Msg('2', &bulkData[i].MeLiBuMsg[0], bulkData[i].WriteMsgSize);
                                                    read_Mlb_Full_Msg('2', rMlbMsg_raw2, bulkData[i].ReadMsgSize, bulkData[i].ReadMsgTimeout_ms, "BR2%02x%02x.");
                                                }
                                                break;                                                                        
                                            }
                                            case 3:                             // Read Buffer
                                            {
                                                char rMlbMsg_raw1[MLB_RX_BUFFER_SIZE], rMlbMsg_raw2[MLB_RX_BUFFER_SIZE];
                                                if((bulkData[i].Port & 0x1) == 1)
                                                    read_Mlb_Full_Msg('1', rMlbMsg_raw1, bulkData[i].ReadMsgSize, bulkData[i].ReadMsgTimeout_ms, "BR1%02x%02x.");
                                                if((bulkData[i].Port & 0x2) == 2)
                                                    read_Mlb_Full_Msg('2', rMlbMsg_raw2, bulkData[i].ReadMsgSize, bulkData[i].ReadMsgTimeout_ms, "BR2%02x%02x.");
                                                break;
                                            }
                                            default:
                                                break;
                                        }
                                        Thread::wait(bulkData[i].WaitAfterNext_ms);
                                    }
                                    bulkDataSize = 0;
                                    pc.printf("OK\n");
                                    break;
                                }
                                case 'I':
                                {
                                    bulkDataSize = 0;
                                    pc.printf("OK\n");
                                    break;
                                }
                                case 'C':
                                {
                                    if(bulkDataSize<BULK_DATA_SIZE) {
                                        bulkData[bulkDataSize].OpType = 0;                                      
                                        sscanf(&mail->Data[2], "%1x", &bulkData[bulkDataSize].Port);
                                        sscanf(&mail->Data[3], "%2x", &bulkData[bulkDataSize].WaitAfterNext_ms);
                                        sscanf(&mail->Data[5], "%1x", &bulkData[bulkDataSize].ClearBufferMask);
                                        bulkDataSize++;
                                    }                                 
                                    break;
                                }
                                case 'W':
                                {
                                    if(bulkDataSize<BULK_DATA_SIZE) {
                                        bulkData[bulkDataSize].OpType = 1;                                      
                                        sscanf(&mail->Data[2], "%1x", &bulkData[bulkDataSize].Port);
                                        sscanf(&mail->Data[3], "%2x", &bulkData[bulkDataSize].WaitAfterNext_ms);
                                        sscanf(&mail->Data[5], "%2x", &bulkData[bulkDataSize].WriteMsgSize);
                            
                                        unsigned int tempData;
                                        for(int i=0; i<bulkData[bulkDataSize].WriteMsgSize; i++) {
                                            sscanf(&mail->Data[(i*2)+8], "%2x", &tempData);
                                            bulkData[bulkDataSize].MeLiBuMsg[i] = (char)tempData;
                                        }
                                        bulkDataSize++;
                                    }                                 
                                    break;
                                }
                                case 'H':
                                {
                                    if(bulkDataSize<BULK_DATA_SIZE) {
                                        bulkData[bulkDataSize].OpType = 2;
                                        sscanf(&mail->Data[2], "%1x", &bulkData[bulkDataSize].Port);
                                        sscanf(&mail->Data[3], "%2x", &bulkData[bulkDataSize].WaitAfterNext_ms);
                                        sscanf(&mail->Data[5], "%2x", &bulkData[bulkDataSize].WriteMsgSize);
                                        sscanf(&mail->Data[7], "%2x", &bulkData[bulkDataSize].ReadMsgSize);
                                        sscanf(&mail->Data[9], "%2x", &bulkData[bulkDataSize].ReadMsgTimeout_ms);
                                        
                                        unsigned int tempData;
                                            for(int i=0; i<bulkData[bulkDataSize].WriteMsgSize; i++) {
                                                sscanf(&mail->Data[(i*2)+12], "%2x", &tempData);
                                                bulkData[bulkDataSize].MeLiBuMsg[i] = (char)tempData;
                                            }
                                        bulkDataSize++;
                                    }
                                    break;
                                }
                                case 'R':
                                {
                                    if(bulkDataSize<BULK_DATA_SIZE) {
                                        bulkData[bulkDataSize].OpType = 3;
                                        sscanf(&mail->Data[2], "%1x", &bulkData[bulkDataSize].Port);
                                        sscanf(&mail->Data[3], "%2x", &bulkData[bulkDataSize].WaitAfterNext_ms);
                                        sscanf(&mail->Data[5], "%2x", &bulkData[bulkDataSize].ReadMsgSize);
                                        sscanf(&mail->Data[7], "%2x", &bulkData[bulkDataSize].ReadMsgTimeout_ms);
                                        bulkDataSize++;
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                            break;
                        }
                        case 'F':
                        {
                            unsigned int funcNumber = 0, mlbMask = 0;
                            sscanf(&mail->Data[1], "%2x", &funcNumber);
                            sscanf(&mail->Data[3], "%1x", &mlbMask);
                            
                            switch(funcNumber)  {
                                case 1:
                                {
                                    /*
                                    Function 1: Execute RGB PWM (100%)
                                        - Param 1: MeLiBu Slave Address (0-63) [2 bytes]
                                        - Param 2: StripLed Led Number (0-6) [1 byte]
                                        - Param 3: RGB Value (Off 0x0, Red 0x1, Green 0x2, Blue 0x3, White 0x4)[1 byte]
                                    */
                                        
                                    unsigned int slaveAdd = 0, ledNum = 0, rgbVal = 0;
                                    sscanf(&mail->Data[4], "%2x", &slaveAdd);
                                    sscanf(&mail->Data[6], "%1x", &ledNum);
                                    sscanf(&mail->Data[7], "%1x", &rgbVal);
                                                                       
                                    char factMode[10], rgbPWM[40], sync[10];
                                    memcpy(factMode, stl_factMode, sizeof(factMode));
                                    memcpy(rgbPWM, stl_rgbPWM, sizeof(rgbPWM));
                                    memcpy(sync, stl_sync, sizeof(sync));
                                    compose_melibu_full_frame(factMode, sizeof(factMode), slaveAdd);
                                    compose_rgbPWM100_frame(rgbPWM, sizeof(rgbPWM), slaveAdd, ledNum, rgbVal);
                                    compose_melibu_full_frame(sync, sizeof(sync), slaveAdd);
                                        
                                    for(int i=1; i<=2; i++) {
                                        if((i == (mlbMask & 0x1)) || (i == (mlbMask & 0x2))) {
                                            write_Mlb_Full_Msg('0'+i, factMode, sizeof(factMode)); Thread::wait(5);
                                            write_Mlb_Full_Msg('0'+i, sync, sizeof(sync)); Thread::wait(5);
                                            write_Mlb_Full_Msg('0'+i, rgbPWM, sizeof(rgbPWM)); Thread::wait(5);
                                            write_Mlb_Full_Msg('0'+i, sync, sizeof(sync)); Thread::wait(5);
                                        }
                                    }
                                    
                                    pc.printf("OK\n");
                                                                                                                                                                                                                 
                                    break;
                                }
                                case 2:
                                {
                                    /*
                                    Function 1: Read Calibration Data
                                        - Param 1: MeLiBu Slave Address Start(4-63) [2 bytes]
                                        - Param 2: MeLiBu Slave Address End(4-63) [2 bytes]
                                    */
                                    
                                    unsigned int slaveAdd_S = 0, slaveAdd_E = 0;
                                    sscanf(&mail->Data[4], "%2x", &slaveAdd_S);
                                    sscanf(&mail->Data[6], "%2x", &slaveAdd_E);
                                    
                                    for(int i=0; i<((slaveAdd_E-slaveAdd_S)+1); i++) {
                                        
                                        char progMode[10], moveMemPointer[10], readEEPROM[10], readResponse12[2];
                                        memcpy(progMode, stl_progMode, sizeof(progMode));
                                        memcpy(moveMemPointer, stl_moveMemPointer, sizeof(moveMemPointer));
                                        memcpy(readEEPROM, stl_readEEPROM, sizeof(readEEPROM));
                                        memcpy(readResponse12, stl_readResponse12, sizeof(readResponse12));
                                        
                                        compose_melibu_full_frame(progMode, sizeof(progMode), slaveAdd_S+i);
                                        compose_memPointer_frame(moveMemPointer, sizeof(moveMemPointer), slaveAdd_S+i, 0x840);
                                        compose_melibu_full_frame(readEEPROM, sizeof(readEEPROM), slaveAdd_S+i);
                                        compose_melibu_header_frame(readResponse12, sizeof(readResponse12), slaveAdd_S+i);
                                        
                                        char rMlbMsg_raw1[MLB_RX_BUFFER_SIZE], rMlbMsg_raw2[MLB_RX_BUFFER_SIZE];
                                        
                                        for(int j=1; j<=2; j++) {
                                            if((j == (mlbMask & 0x1)) || (j == (mlbMask & 0x2))) {
                                                write_Mlb_Full_Msg('0'+j, progMode, sizeof(progMode)); Thread::wait(5);
                                                write_Mlb_Full_Msg('0'+j, moveMemPointer, sizeof(moveMemPointer)); Thread::wait(1);
                                                for(int k=0; k<28; k++) {
                                                    write_Mlb_Full_Msg('0'+j, readEEPROM, sizeof(readEEPROM)); Thread::wait(1);
                                                    flush_Mlb_Buffer('0'+j, '2');
                                                    write_Mlb_Full_Msg('0'+j, readResponse12, sizeof(readResponse12)); Thread::wait(1);
                                                    if(j==1) read_Mlb_Full_Msg('0'+j, rMlbMsg_raw1, 22, 1, "FR1%02x%02x.");
                                                    else if(j==2) read_Mlb_Full_Msg('0'+j, rMlbMsg_raw2, 22, 1, "FR2%02x%02x.");
                                                    Thread::wait(1); 
                                                }
                                            }
                                        }       
                                    }
                                                     
                                    pc.printf("OK\n");
                                    
                                    break;
                                }
                                case 3:
                                {
                                    /*
                                    Function 3: Simulate Slave Address Data
                                    */
                                        
                                    char modData_0x4[14],modData_0x5[14],modData_0x6[14],modData_0x7[14],modData_0x8[14];
                                    char modData_0x9[14],modData_0xA[14],modData_0xB[14],modData_0xC[14],modData_0xD[14];
                                    memcpy(modData_0x4, stl_modData_0x4, sizeof(modData_0x4));
                                    memcpy(modData_0x5, stl_modData_0x5, sizeof(modData_0x5));
                                    memcpy(modData_0x6, stl_modData_0x6, sizeof(modData_0x6));
                                    memcpy(modData_0x7, stl_modData_0x7, sizeof(modData_0x7));
                                    memcpy(modData_0x8, stl_modData_0x8, sizeof(modData_0x8));
                                    memcpy(modData_0x9, stl_modData_0x9, sizeof(modData_0x9));
                                    memcpy(modData_0xA, stl_modData_0xA, sizeof(modData_0xA));
                                    memcpy(modData_0xB, stl_modData_0xB, sizeof(modData_0xB));
                                    memcpy(modData_0xC, stl_modData_0xC, sizeof(modData_0xC));
                                    memcpy(modData_0xD, stl_modData_0xD, sizeof(modData_0xD));

                                    switch(mlbMask) {
                                        case 1:
                                        {
                                            Timer t;
                                            t.start();                                                 
                                            while(t.read_ms()<10000) {                                                                    
                                                if(detect_header_frame('1', 0x1315)) write_Mlb_Response_Msg('1', modData_0x4, 14);
                                                if(detect_header_frame('1', 0x1717)) write_Mlb_Response_Msg('1', modData_0x5, 14);
                                                if(detect_header_frame('1', 0x1B17)) write_Mlb_Response_Msg('1', modData_0x6, 14);
                                                if(detect_header_frame('1', 0x1F15)) write_Mlb_Response_Msg('1', modData_0x7, 14);                                                    else if(detect_header_frame('1', 0x2315)) pc.printf("0x2315");//write_Mlb_Response_Msg('1', modData_0x8, 14);
                                                if(detect_header_frame('1', 0x2717)) write_Mlb_Response_Msg('1', modData_0x9, 14);
                                                if(detect_header_frame('1', 0x2B17)) write_Mlb_Response_Msg('1', modData_0xA, 14);
                                                if(detect_header_frame('1', 0x2F15)) write_Mlb_Response_Msg('1', modData_0xB, 14);
                                                if(detect_header_frame('1', 0x3317)) write_Mlb_Response_Msg('1', modData_0xC, 14);
                                                if(detect_header_frame('1', 0x3715)) write_Mlb_Response_Msg('1', modData_0xD, 14);
                                            }
                                            t.stop();                                                    
                                            break;
                                        }
                                        case 2:
                                        {   
                                            Timer t;
                                            t.start();                                                 
                                            while(t.read_ms()<10000) {                                                                    
                                                if(detect_header_frame('2', 0x1315)) write_Mlb_Response_Msg('2', modData_0x4, 14);
                                                if(detect_header_frame('2', 0x1717)) write_Mlb_Response_Msg('2', modData_0x5, 14);
                                                if(detect_header_frame('2', 0x1B17)) write_Mlb_Response_Msg('2', modData_0x6, 14);
                                                if(detect_header_frame('2', 0x1F15)) write_Mlb_Response_Msg('2', modData_0x7, 14);                                                    else if(detect_header_frame('1', 0x2315)) pc.printf("0x2315");//write_Mlb_Response_Msg('1', modData_0x8, 14);
                                                if(detect_header_frame('2', 0x2717)) write_Mlb_Response_Msg('2', modData_0x9, 14);
                                                if(detect_header_frame('2', 0x2B17)) write_Mlb_Response_Msg('2', modData_0xA, 14);
                                                if(detect_header_frame('2', 0x2F15)) write_Mlb_Response_Msg('2', modData_0xB, 14);
                                                if(detect_header_frame('2', 0x3317)) write_Mlb_Response_Msg('2', modData_0xC, 14);
                                                if(detect_header_frame('2', 0x3715)) write_Mlb_Response_Msg('2', modData_0xD, 14);
                                            }
                                            t.stop();
                                            break;
                                        }
                                        default:
                                            break;                                         
                                    }
                                    pc.printf("OK");
                                }
                                case 4:
                                {
                                    /*
                                    Function 4: Execute RGB LUV (100%)
                                        - Param 1: MeLiBu Slave Address (0-63) [2 bytes]
                                        - Param 2: StripLed Led Number (0-6) [1 byte]
                                        - Param 3: L (0-100)[2 bytes]
                                        - Param 4: u' (0-65535)[4 bytes]
                                        - Param 5: v' (0-65535)[4 bytes]
                                    */
                                        
                                    unsigned int slaveAdd = 0, ledNum = 0, L = 0, u = 0, v = 0;
                                    sscanf(&mail->Data[4], "%2x", &slaveAdd);
                                    sscanf(&mail->Data[6], "%1x", &ledNum);
                                    sscanf(&mail->Data[7], "%3x", &L);
                                    sscanf(&mail->Data[10], "%4x", &u);
                                    sscanf(&mail->Data[14], "%4x", &v);
                                                                       
                                    char rgbLUV[40], sync[10];
                                    memcpy(rgbLUV, stl_rgbLUV, sizeof(rgbLUV));
                                    memcpy(sync, stl_sync, sizeof(sync));
                                    compose_rgbLUV100_frame(rgbLUV, sizeof(rgbLUV), slaveAdd, ledNum, L, u, v);
                                    compose_melibu_full_frame(sync, sizeof(sync), slaveAdd);
                                        
                                    for(int i=1; i<=2; i++) {
                                        if((i == (mlbMask & 0x1)) || (i == (mlbMask & 0x2))) {
                                            write_Mlb_Full_Msg('0'+i, rgbLUV, sizeof(rgbLUV)); Thread::wait(5);
                                            write_Mlb_Full_Msg('0'+i, sync, sizeof(sync)); Thread::wait(5);
                                        }
                                    }
                                    
                                    pc.printf("OK\n");
                                                                                                                                                                                                                 
                                    break;
                                }
                                default:
                                    break;
                            }
                            break;                            
                        }
                        case 'V':
                        {
                            if(mail->Data[1]=='0') pc.printf("OK\n");
                            break;
                        }
                        case 'M':
                            switch(mail->Data[2]) {
                                case 'C':
                                {                                                                        
                                    flush_Mlb_Buffer(mail->Data[1], mail->Data[3]);
                                    pc.printf("OK\n");                        
                                    break;
                                }
                                case 'H':
                                {
                                    unsigned int wDataSize = 0, rDataSize = 0, rTimeout_ms = 0;
                                    sscanf(&mail->Data[3], "%2x",&wDataSize);
                                    sscanf(&mail->Data[5], "%2x", &rDataSize);
                                    sscanf(&mail->Data[7], "%3x", &rTimeout_ms);
                                    
                                    char wMlbMsg_raw[wDataSize];
                                    unsigned int tempData;
                                    for(int i=0; i<wDataSize; i++) {
                                        sscanf(&mail->Data[(i*2)+11], "%2x", &tempData);
                                        wMlbMsg_raw[i] = (char)tempData;
                                    }
                                    
                                    write_Mlb_Full_Msg(mail->Data[1], wMlbMsg_raw, wDataSize);
                                    
                                    char rMlbMsg_raw[MLB_RX_BUFFER_SIZE];
                                                                        
                                    read_Mlb_Full_Msg(mail->Data[1], rMlbMsg_raw, rDataSize, rTimeout_ms, "M1R%02x%03x.");
                                    
                                    pc.printf("OK\n");
                                    
                                    break;
                                }
                                case 'R':
                                {   
                                    unsigned int dataSize = 0, timeout_ms = 0;
                                    sscanf(&mail->Data[3], "%2x", &dataSize);
                                    sscanf(&mail->Data[5], "%3x", &timeout_ms);
                                    
                                    char mlbMsg_raw[MLB_RX_BUFFER_SIZE];
                                                                        
                                    read_Mlb_Full_Msg(mail->Data[1], mlbMsg_raw, dataSize, timeout_ms, "M1R%02x%03x.");
                                    
                                    pc.printf("OK\n");
                                                                  
                                    break;
                                }
                                case 'W':
                                {   
                                    unsigned int dataSize = 0;
                                    sscanf(&mail->Data[3], "%2x",&dataSize);
                                    
                                    char mlbMsg_raw[dataSize];
                                    unsigned int tempData;
                                    for(int i=0; i<dataSize; i++) {
                                        sscanf(&mail->Data[(i*2)+6], "%2x", &tempData);
                                        mlbMsg_raw[i] = (char)tempData;
                                    }
                                    
                                    write_Mlb_Full_Msg(mail->Data[1], mlbMsg_raw, dataSize);
                                    
                                    pc.printf("OK\n");
                                                               
                                    break;
                                }
                                default:
                                    break;
                            }
                        default:
                            break;
                    }
                default:
                    break;
            }
            mailMain.free(mail);
        }
    }
}

void pcRxSerialIntr(MODSERIAL_IRQ_INFO *info) {  
    ledPcRx = !ledPcRx;
    char temp[MAILCMD_DATA_SIZE];
    MODSERIAL *sys = info->serial;

    sys->move(temp, MAILCMD_DATA_SIZE, PC_RX_ENDCHAR);
    loadMail(0, temp);
}

void loadMail(char id, char *data) {
    MailMainCmd_t *mail = mailMain.alloc();
    mail->Id = id;
    snprintf(mail->Data,MAILCMD_DATA_SIZE, "%s", data);
    mailMain.put(mail);
}

void write_Mlb_Full_Msg(char port, char *mlbMsg_raw, unsigned int dataSize) {
    switch(port) {
        case '1':
        {
            mlb1.set_break();
            Thread::wait(1);
            mlb1.clear_break();
            for(int i=0; i<dataSize; i++) {
                mlb1.putc(reverse_byte(*mlbMsg_raw));
                mlbMsg_raw++;
            }
            break;
        }
        case '2':
        {
            mlb2.set_break();
            Thread::wait(1);
            mlb2.clear_break();
            for(int i=0; i<dataSize; i++) {
                mlb2.putc(reverse_byte(*mlbMsg_raw));
                mlbMsg_raw++;
            }
            break;
        }
        default:
            break;
    }
}

void write_Mlb_Response_Msg(char port, char *mlbMsg_raw, unsigned int dataSize) {
    switch(port) {
        case '1':
        {
            for(int i=0; i<dataSize; i++) {
                mlb1.putc(reverse_byte(*mlbMsg_raw));
                mlbMsg_raw++;
            }
            break;
        }
        case '2':
        {
            for(int i=0; i<dataSize; i++) {
                mlb2.putc(reverse_byte(*mlbMsg_raw));
                mlbMsg_raw++;
            }
            break;
        }
        default:
            break;
    }
}

void read_Mlb_Full_Msg(char port, char *mlbMsg_raw, unsigned int dataSize, unsigned int timeout_ms, const char responseHeader[]) {
    
    Timer t;
    int rxBufferCount = 0;
    
    switch(port) {
        case '1':
        {
            ledMlb1Rx = !ledMlb1Rx;
            if(mlb1.readable()) {
                t.start();
                while(mlb1.rxBufferGetCount()<dataSize && t.read_ms()<timeout_ms) {
                    Thread::wait(READLOOP_WAIT);
                }
                t.stop();
                
                rxBufferCount = mlb1.rxBufferGetCount();
                
                //pc.printf("M1R%02x%03x.", rxBufferCount, t.read_ms());
                pc.printf(responseHeader, rxBufferCount, t.read_ms());    
                for(int i=0; i<rxBufferCount; i++) {
                    *mlbMsg_raw = reverse_byte(mlb1.getc());
                    pc.printf("%02x", *mlbMsg_raw);
                    mlbMsg_raw++;
                }
                pc.printf(".");
            }
            break;
        }
        case '2':
        {
            ledMlb2Rx = !ledMlb2Rx;
            if(mlb2.readable()) {
                t.start();
                while(mlb2.rxBufferGetCount()<dataSize && t.read_ms()<timeout_ms) {
                    Thread::wait(READLOOP_WAIT);
                }
                t.stop();
                
                rxBufferCount = mlb2.rxBufferGetCount();
                
                //pc.printf("M2R%02x%03x.", rxBufferCount, t.read_ms());
                pc.printf(responseHeader, rxBufferCount, t.read_ms()); 
                for(int i=0; i<rxBufferCount; i++) {
                    *mlbMsg_raw = reverse_byte(mlb2.getc());
                    pc.printf("%02x", *mlbMsg_raw);
                    mlbMsg_raw++;
                }
                pc.printf(".");
            }
            break;
        }
        default:
            break;
    }
}

void flush_Mlb_Buffer(char port, char clearMask)   {
    switch(port) {
        case '1':
        {
            switch(clearMask)   {
                case '1':
                    mlb1.txBufferFlush();
                    break;
                case '2':
                    while(mlb1.readable()) mlb1.getc();
                    break;
                case '3':
                    mlb1.txBufferFlush();
                    while(mlb1.readable()) mlb1.getc();
                    break;
                default:
                    break;
            }                
        }
        break;
        case '2':
        {
                switch(clearMask)   {
                case '1':
                    mlb2.txBufferFlush();
                    break;
                case '2':
                    while(mlb2.readable()) mlb2.getc();
                    break;
                case '3':
                    mlb2.txBufferFlush();
                    while(mlb2.readable()) mlb2.getc();
                    break;
                default:
                    break;
            }   
        }
        break;
        default:
            break;    
    }
}

bool detect_header_frame(char port, unsigned int header)   {
    
    Timer t;
    char header_h = (char)((header & 0xFF00)>>8);
    char header_l = (char)(header & 0xFF);
    
    switch(port)    {
        case '1':
        {
            if(mlb1.readable()) {
                if(reverse_byte(mlb1.getc())==header_h)   {
                        t.start();
                        while(t.read_us()<12)   {
                            if(mlb1.readable()) {
                                if(reverse_byte(mlb1.getc())==header_l)   {
                                    return true;
                                }
                            }   
                        }
                }
            }     
            break;
        }
        case '2':
        { 
            if(mlb2.readable()) {
                if(reverse_byte(mlb2.getc())==header_h)   {
                        t.start();
                        while(t.read_us()<12)   {
                            if(mlb2.readable()) {
                                if(reverse_byte(mlb2.getc())==header_l)   {
                                    return true;
                                }
                            }   
                        }
                }
            }     
            break;
        }
        default:
            break;
    }
    return false;                                                                         
}