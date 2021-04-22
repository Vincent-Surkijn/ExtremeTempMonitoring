/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.7
        Device            :  PIC18F25K50
        Driver Version    :  2.00
*/

#define true 1
#define false 0

#include <xc.h>
#include "mcc_generated_files/mcc.h"
#include <stdio.h>
#include <string.h>

/*  Global variables*/
int ID = 0;
char name[50] = "SENSOR";
    
char UART_RX;
char RX_buffer[500];
char *expected;
int ix = 0;
char UART_RN4870_mode = true;

char debug;
//har UART_str_ready = false;
char og;    //btn status

/*  Prototypes  */
void UART_Init(void);
void UART_Write(char data);
void UART_Write_String(char *buffer);
char RN4870_changeName(char *name);
int packetHandler();

void __interrupt (high_priority) high_ISR(void)
{
   if (PIR1bits.RCIF == 1){  // UART receive  
        if(debug){ 
           LATAbits.LA1 = 1;
        }
        RX_buffer[ix] = RCREG1;         // Add to buffer
        ix++;                           // Increment buffer index
         
        //if(RCREG1=='\r'){             // Write response to string

            //UART_str_ready = true;
            //ix = 0;                     // Reset buffer
        if(UART_RN4870_mode){
            if(strstr(RX_buffer,expected)!=NULL){   // Search for expected response in buffer(some noise might have occurred before or after)
                UART_RX = true;
                ix = 0;
            }
        }
        //}
        //PIR1bits.RCIF = 0;
   }	
}

/*
                         Main application
 */
void main(void)
{
    volatile char status = 0;
    og = PORTCbits.RC0;    //btn status
    
    // Initialize the device
    SYSTEM_Initialize();
    UART_Init();
       
    _delay(100000);
  
    /**** Debugging tools on/off ****/
    debug = true;
    TRISAbits.RA1 = 0;
    TRISBbits.RB3 = 0;
    TRISBbits.RB4 = 0;
    TRISBbits.RB5 = 0;
    TRISCbits.RC0 = 1;
    
    //status = RN4870_changeName("theBLEatles");
    UART_Write_String("Start\n");
    
    while (1)
    {
        //UART_Write('$');
        status = packetHandler();
        
        // Add your application code
        LATBbits.LB5 = 1;
        _delay(1000000);
        LATBbits.LB5 = 0;
        _delay(1000000);
    }
}

void UART_Init(void){
    TXSTA1bits.SYNC = 0;              // Asynchronous mode
    RCSTA1bits.SPEN = 1;              // Enable Serial port
/*    TXSTA1bits.BRGH = 1;              // Set For High-Speed Baud Rate
    BAUDCON1bits.BRG16 = 1;           // 16-bit Baud Rate Generator(better error rate)
    SPBRG1 = 34;                      // Set The Baud Rate To Be 115.2 kbps
*/
  //115.2k for bootloader upload
  TXSTA1bits.BRGH = 1; // Set For High-Speed Baud Rate
  BAUDCON1bits.BRG16 = 0;
  SPBRG1 = 25; // Set The Baud Rate To Be 115200 bps*/
    
    /*TXSTA1bits.BRGH = 0; // Set For High-Speed Baud Rate
  BAUDCON1bits.BRG16 = 0;
  SPBRG1 = 25; // Set The Baud Rate To Be 9600 bps*/
    
    // Set The RX-TX Pins to be in UART mode 
    TRISCbits.RC6 = 1;                // RC6 --> TX 
    TRISCbits.RC7 = 1;                // RC7 --> RX

    // Enable UART RX interrupts
    INTCONbits.GIE = 0;               // Disable global interrupts
    RCONbits.IPEN =  1;               // Enable interrupt priority
    PIE1bits.RCIE = 1;                // UART Receving Interrupt Enable Bit
    IPR1bits.RCIP = 1;                // UART receive interrupt --> high priority

    RCSTA1bits.CREN = 1;              // Enable Data Continous Reception

    // clear interrupt flags
    PIR1bits.RCIF = 0;
    PIR1bits.TXIF = 0;
    
    TXSTA1bits.TXEN = 1;              // Enable UART Transmission
    INTCONbits.GIE = 1;               // Enable global interrupts
}

void UART_Write(char data){
    //LATBbits.LB4 = 0;      // Pull UART RX low
    
    while(!TXSTA1bits.TRMT);
    TXREG1 = data;          // Load the data to the transmit buffer
    UART_RX = false;        // Start to wait for ack of ble module
}

void UART_Write_String(char *buffer){
    volatile int size = strlen(buffer);
    for(int i = 0 ; i < size ; i++){
      UART_Write(buffer[i]);
    }
}

char RN4870_changeName(char *name){
    UART_RN4870_mode = true;                    // Enter RN4870 mode to compare responses
    
    UART_Write_String("$$$");                   // Enter command mode
    expected = "CMD";                           // Specify expected response 
    while(!UART_RX){
        if(PORTCbits.RC0 != og && debug){        //debug tool
            UART_Write_String(RX_buffer);
            UART_Write_String("!=");
            UART_Write_String(expected);
            UART_Write_String("|");
            _delay(100000);
        }
    }
    
    UART_Write_String("SS,C0\r");               // Support Device Info and UART Transparent services
    expected = "AOK";                           // Specify expected response
    while(!UART_RX);                            // Wait for BLE module to respond

    UART_Write_String("S-,");            // Name device
    UART_Write_String(name);
    UART_Write_String("\r");
    expected = "AOK";                           // Specify expected response
    while(!UART_RX);                            // Wait for BLE module to respond

    UART_Write_String("R,1\r");                 // Reboot device for configuration to take effect   
    expected = "Rebooting";                     // Specify expected response
    while(!UART_RX);                            // Wait for BLE module to respond

    UART_RN4870_mode = false;
        
    return 1;                                   // Confirm change
}

int packetHandler(){    
    if(strstr(RX_buffer,"hey")!=NULL){
        UART_Write_String("hallo\n");
        
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }
    else if(strstr(RX_buffer,"hello there")!=NULL){
        UART_Write_String("General Kenobi\n");
                
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }
    else if(strstr(RX_buffer,"temp")!=NULL){
        short int temp = 25;
        char answer[20];
        sprintf(answer,"The temperature is %i\r",temp);
        UART_Write_String(answer);    
        
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }
    else if(strstr(RX_buffer,"changeID:")!=NULL){
        char *pos = strstr(RX_buffer,"changeID:") + strlen("changeID:");
        char *end;
        ID = strtol(pos,&end,10);
        
        char answer[50];
        //sprintf(answer,"The id is %d\n",ID);
        sprintf(answer,"changeID:IDACK\r");
        UART_Write_String(answer);
                
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }  
    else if(strstr(RX_buffer,"changeName:")!=NULL){
        char *txt = strstr(RX_buffer,"changeName:") + strlen("changeName:");        
        sprintf(name,"%s%d",txt,ID);

        // send back ACK
        char answer[50];
        sprintf(answer,"changeName:nameACK\r");
        UART_Write_String(answer);
        
        __delay_ms(100); // wait 100ms(datasheet) so the RN4870 can separate the data and commands
        
        // change the name of the BLE module as well
        RN4870_changeName(name);
        
        /*char answer[50];
        printf(answer,"The id is now %d\n",ID);
        UART_Write_String(answer);
        sprintf(answer,"The name is %s\n",name);
        UART_Write_String(answer);
        sprintf(answer,"%s%d\n",name,ID);
        UART_Write_String(answer);*/
        
        return 1;
    }    
    return 0;
}
/**
 End of File
*/