#include "mbed.h"
#include "MODSERIAL.h"
#include <cstdint>
#define MAXIMUM_BUFFERING_SIZE 24
MODSERIAL puerto1(p9,p10,2048,2048);
MODSERIAL puerto2(p13,p14,2048,2048);
const int DataSize=23;

unsigned char mssg[]={0x01, 0xc8, 0x20, 0x43, 0x06, 0xc8, 0x00,0x00,0x02, 0xc8, 0x20, 0x43, 0x06, 0xc8, 0x00,0x00,0xC6};
unsigned char header[]={0x55, 0x00, 0xE8, 0xD7};
unsigned char mssgred[]={0x01,0x00,0x00,0x40,0x06,0x00,0x00,0x01,0x02,0x00,0x00,0x40,0x06,0x00,0x00,0x01,0x1B};
unsigned char mssgreen[]={0x01,0x00,0x00,0x40,0x06,0x00,0x00,0x02,0x02,0x00,0x00,0x40,0x06,0x00,0x00,0x02,0xF8};
unsigned char mssgblue[]={0x01,0x00,0x00,0x40,0x06,0x00,0x00,0x03,0x02,0x00,0x00,0x40,0x06,0x00,0x00,0x03,0x7A};
unsigned char headersync[]={0x55, 0x00, 0x6C, 0xF0};
unsigned char sync_on[]={0x64,0x64,0xD2};
unsigned char sync_off[]={0x00,0x00,0x1D};

unsigned char headerstat1[]={0x55,0x01,0xD0,0x03};
unsigned char headerstat2[]={0x55,0x02,0xD0,0xBB};
unsigned char headerstat3[]={0x55,0x03,0xd0,0xd3};
unsigned char headerstat4[]={0x55,0x04,0xd0,0xff};
int read_port(MODSERIAL &puerto, char buff[MAXIMUM_BUFFERING_SIZE], int DataLength){
    int count=0,contador=0;
    if(puerto.readable()){
        //printf("TRAZA 1");
        while(puerto.rxBufferGetCount()<DataLength && contador<=1000){
            wait(0.00001);
            contador++;
        }
        count=puerto.rxBufferGetCount();
        //printf("Count: %d",count);
        for(int i=0;i<count;i++){
            buff[i]=puerto.getc();
            //printf("%02x",buff[i]);
        }
}
    return count;
}
void flush_port(MODSERIAL &puerto){
    int rxcontent=0;
    if(puerto.readable()){
        rxcontent=puerto.rxBufferGetCount();
        for(int i=0;i<rxcontent;i++){
            puerto.getc();
        }
    }
}

int main() {
DigitalOut led=LED1;
//MODSERIAL puerto_serie(USBTX,USBRX);
MODSERIAL puerto_serie(p28,p27);
puerto_serie.baud(115200);
puerto_serie.format(8,SerialBase::None,1);
puerto2.baud(1000000);
puerto2.format(8,SerialBase::Even,1);
puerto1.baud(1000000);
puerto1.format(8,SerialBase::Even,1);
int r=1;
int ContenidoRxBuffer;
while(true){
    //printf("ACK\n");
    int contador=0,i=0;
    led=!led;
    char mail[24]={0};
    char mensaje[MAXIMUM_BUFFERING_SIZE]={0};
    int count=0;
    count=read_port(puerto_serie,mensaje,24);
    switch(mensaje[0]){
    case '1':
        printf("ACK1\n");
        switch (mensaje[1]) 
        {
            case 'C':
                unsigned char headeraux[4];
                unsigned char mssgaux[17];
                for(int i=0;i<4;i++){
                    headeraux[i]=mensaje[i+2];
                }
                for(int i=0;i<17;i++){
                    mssgaux[i]=mensaje[i+6];
                }
               puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(headeraux[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto1.putc(mssgaux[i]);
                }
                wait(0.00003);
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(headersync[i]); 
                }
                wait(0.000010);
                for(int i=0;i<3;i++){
                    puerto1.putc(sync_on[i]);
                }
            break;
            case 'R':
                //printf("SWITCH CASE R\n");
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(header[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto1.putc(mssgred[i]);
                }
                wait(0.00003);
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(headersync[i]); 
                }
                        wait(0.000010);
                for(int i=0;i<3;i++){
                     puerto1.putc(sync_on[i]);
                }
            break;
            case 'G':
                //printf("SWITCH CASE Green\n");
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(header[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto1.putc(mssgreen[i]);
                }
                wait(0.00003);
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(headersync[i]); 
                }
                        wait(0.000010);
                for(int i=0;i<3;i++){
                     puerto1.putc(sync_on[i]);
                }
            break;
            case 'B':
                //printf("SWITCH CASE Blue\n");
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(header[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto1.putc(mssgblue[i]);
                }
                wait(0.00003);
                puerto1.set_break();
                wait(0.000005);
                puerto1.clear_break();
                for(int i=0;i<4;i++){
                    puerto1.putc(headersync[i]); 
                }
                        wait(0.000010);
                for(int i=0;i<3;i++){
                     puerto1.putc(sync_on[i]);
                }
            break;
            case 'T':
                switch(mensaje[6]){
                    case '1':
                        //printf("SWITCH CASE STAT1\n");
                        flush_port(puerto1);
                        puerto1.set_break();
                        wait(0.000005);
                        puerto1.clear_break();
                        for(int i=0;i<4;i++){
                            puerto1.putc(headerstat1[i]); 
                        }
                        read_port(puerto1,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                    case '2':
                        //printf("SWITCH CASE STAT2\n");
                        flush_port(puerto1);
                        puerto1.set_break();
                        wait(0.000005);
                        puerto1.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headerstat2[i]); 
                        }
                        read_port(puerto1,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                    case '3':
                        //printf("SWITCH CASE STAT3\n");
                        flush_port(puerto1);
                        puerto1.set_break();
                        wait(0.000005);
                        puerto1.clear_break();
                        for(int i=0;i<4;i++){
                            puerto1.putc(headerstat3[i]); 
                        }
                        read_port(puerto1,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                    case '4':
                        //printf("SWITCH CASE STAT4\n");
                        flush_port(puerto1);
                        puerto1.set_break();
                        wait(0.000005);
                        puerto1.clear_break();
                        for(int i=0;i<4;i++){
                            puerto1.putc(headerstat4[i]); 
                        }
                        read_port(puerto1,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                }
            break;
            case 'S':
                switch(mensaje[7]){
                    case 'N':
                        //printf("SWITCH CASE ON\n");
                        puerto1.set_break();
                        wait(0.000005);
                        puerto1.clear_break();
                        for(int i=0;i<4;i++){
                            puerto1.putc(headersync[i]); 
                        }
                        wait(0.000010);
                        for(int i=0;i<3;i++){
                            puerto1.putc(sync_on[i]);
                         }
                    break;
                    break;
                    case 'F':
                        //printf("SWITCH CASE OFF\n");
                        puerto1.set_break();
                        wait(0.000005);
                        puerto1.clear_break();
                        for(int i=0;i<4;i++){
                            puerto1.putc(headersync[i]); 
                        }
                        wait(0.000010);
                        for(int i=0;i<3;i++){
                            puerto1.putc(sync_off[i]);
                        
                         }
                    break;
                    break;
                }
            break;
        }
        break;
        case '2':
        printf("ACK2\n");
        switch (mensaje[1]) 
        {
            case 'C':
                unsigned char headeraux[4];
                unsigned char mssgaux[17];
                for(int i=0;i<4;i++){
                    headeraux[i]=mensaje[i+2];
                }
                for(int i=0;i<17;i++){
                    mssgaux[i]=mensaje[i+6];
                }
               puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(headeraux[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto2.putc(mssgaux[i]);
                }
                wait(0.00003);
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(headersync[i]); 
                }
                wait(0.000010);
                for(int i=0;i<3;i++){
                    puerto2.putc(sync_on[i]);
                }
            break;
            case 'R':
                //printf("SWITCH CASE R\n");
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(header[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto2.putc(mssgred[i]);
                }
                wait(0.00003);
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(headersync[i]); 
                }
                wait(0.000010);
                for(int i=0;i<3;i++){
                    puerto2.putc(sync_on[i]);
                }
            break;
            case 'G':
                //printf("SWITCH CASE Green\n");
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(header[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto2.putc(mssgreen[i]);
                }
                wait(0.00003);
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(headersync[i]); 
                }
                wait(0.000010);
                for(int i=0;i<3;i++){
                    puerto2.putc(sync_on[i]);
                }
            break;
            case 'B':
                //printf("SWITCH CASE Blue\n");
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(header[i]); 
                }
                wait(0.000010);
                for(int i=0;i<17;i++){
                    puerto2.putc(mssgblue[i]);
                }
                wait(0.00003);
                puerto2.set_break();
                wait(0.000005);
                puerto2.clear_break();
                for(int i=0;i<4;i++){
                    puerto2.putc(headersync[i]); 
                }
                wait(0.000010);
                for(int i=0;i<3;i++){
                    puerto2.putc(sync_on[i]);
                }
            break;
            case 'T':
                switch(mensaje[6]){
                    case '1':
                        //printf("SWITCH CASE STAT1\n");
                        flush_port(puerto2);
                        puerto2.set_break();
                        wait(0.000005);
                        puerto2.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headerstat1[i]); 
                        }
                        read_port(puerto2,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                    case '2':
                        //printf("SWITCH CASE STAT2\n");
                        flush_port(puerto2);
                        puerto2.set_break();
                        wait(0.000005);
                        puerto2.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headerstat2[i]); 
                        }
                        read_port(puerto2,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                    case '3':
                        //printf("SWITCH CASE STAT3\n");
                        flush_port(puerto2);
                        puerto2.set_break();
                        wait(0.000005);
                        puerto2.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headerstat3[i]); 
                        }
                        read_port(puerto2,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                    case '4':
                        //printf("SWITCH CASE STAT4\n");
                        flush_port(puerto2);
                        puerto2.set_break();
                        wait(0.000005);
                        puerto2.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headerstat4[i]); 
                        }
                        read_port(puerto2,mail,DataSize);
                        for(int i=0;i<DataSize;i++){
                            printf("%02x ",mail[i]);
                        }
                        printf("\n");
                    break;
                }
            break;
            case 'S':
                switch(mensaje[7]){
                    case 'N':
                        //printf("SWITCH CASE ON\n");
                        puerto2.set_break();
                        wait(0.000005);
                        puerto2.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headersync[i]); 
                        }
                        wait(0.000010);
                        for(int i=0;i<3;i++){
                            puerto2.putc(sync_on[i]);
                         }
                    break;
                    break;
                    case 'F':
                        //printf("SWITCH CASE OFF\n");
                        puerto2.set_break();
                        wait(0.000005);
                        puerto2.clear_break();
                        for(int i=0;i<4;i++){
                            puerto2.putc(headersync[i]); 
                        }
                        wait(0.000010);
                        for(int i=0;i<3;i++){
                            puerto2.putc(sync_off[i]);
                        
                         }
                    break;
                    break;
                }
            break;
        }
        break;

        }
    }
    wait(1);
}

