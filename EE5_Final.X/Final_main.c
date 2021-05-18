/**
 * File Name:
 * Final_main.c
 * Author: Vincent S
 * 
*/

/*SNAP IN DEBUGGER*/
#pragma config LVP = ON         // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled if MCLRE is also 1)

#define true 1
#define false 0

#include <xc.h>
#include "mcc.h"
#include <stdio.h>
#include <string.h>

/*  Global variables*/
int ID = 0;
char name[50] = "SENSOR";
volatile char status;
    
char RX_buffer[500];
int ix = 0;

char init = false;
char drdy = false;

char debug;
char og;    //btn status

/*  Prototypes  */
void UART_Init(void);
void UART_Write(char data);
void UART_Write_String(char *buffer);
char RN4870_changeName(char *name);

int packetHandler();

void spi_init();
void spi_send(char data);
char spi_read();
short int read_MAX31856_temp();
short int convertTemp(char byte2, char byte1);
char init_MAX31856();

void init_interrupts();
void nap();
void powerSave();

/*SNAP IN DEBUGGER*/
#pragma config LVP = ON         // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled if MCLRE is also 1)

void __interrupt (high_priority) high_ISR(void)
{
   if(INTCONbits.IOCIF == 1){// In case of IOC interrupt (wake up)
        //LATAbits.LA1 = 1;
        init = true;                // Re-initialize everything
        INTCONbits.IOCIE = 1;       // disable interrupt on change (not needed when awake)
        INTCONbits.IOCIF = 0;       // Clear the flag
   }
   if(PIR1bits.SSPIF == 1){// In case of SPI interrupt
        drdy = true;  
        //LATAbits.LA2 = 1;
        PIR1bits.SSPIF = 0;         // Clear the flag
   }
   if (PIR1bits.RCIF == 1){ // UART receive  
        if(debug){ 
           LATAbits.LA1 = 1;
        }
        RX_buffer[ix] = RCREG1;     // Add to buffer
        ix++;                       // Increment buffer index
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
    
    // turn off unneeded modules to save power
    powerSave();
    
    spi_init();
    UART_Init();
       
    _delay(100000);
  
    /**** Debugging tools on/off ****/
    debug = true;
    TRISAbits.RA1 = 0;
    TRISBbits.RB3 = 0;
    TRISBbits.RB4 = 0;
    TRISBbits.RB5 = 0;
    TRISCbits.RC0 = 1;
    
    UART_Write_String("Start\n");
             
    // Set the wake up
    init_interrupts();
        
    // Go to sleep
    nap();
    
    while (1)
    {
        if(init){
            // turn off unneeded modules to save power
            powerSave();
        
            spi_init();
            UART_Init();
            init = false;
        }
        //UART_Write('$');
        status = packetHandler();
        
    }
}

void UART_Init(void){
    TXSTA1bits.SYNC = 0;              // Asynchronous mode
    RCSTA1bits.SPEN = 1;              // Enable Serial port
    TXSTA1bits.BRGH = 1;              // Set For High-Speed Baud Rate
    BAUDCON1bits.BRG16 = 1;           // 16-bit Baud Rate Generator(better error rate)
    SPBRG1 = 34;                      // Set The Baud Rate To Be 115.2 kbps

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
    while(!TXSTA1bits.TRMT);
    TXREG1 = data;          // Load the data to the transmit buffer
}

void UART_Write_String(char *buffer){
    volatile int size = strlen(buffer);
    for(int i = 0 ; i < size ; i++){
      UART_Write(buffer[i]);
    }
}

char RN4870_changeName(char *name){
    
    UART_Write_String("$$$");                   // Enter command mode
    unsigned long count = 0;
    while(strstr(RX_buffer,"CMD")==NULL){        // wait for answer (not most efficient implementation, but we have to wait anyways)
        count++;            // increment counter
        if(count>75000){    // if time passed, send command again
            UART_Write_String("$$$");
            count = 0;
        }
    }
    // reset UART RX buffer
    memset(RX_buffer,0,strlen(RX_buffer));
    ix = 0;
    
    UART_Write_String("SS,C0\r");               // Support Device Info and UART Transparent services
    count = 0;
    while(strstr(RX_buffer,"AOK")==NULL){        // wait for answer (not most efficient implementation, but we have to wait anyways)
        count++;            // increment counter
        if(count>75000){    // if time passed, send command again
            UART_Write_String("SS,C0\r");
            count = 0;
        }
    }
    // reset UART RX buffer
    memset(RX_buffer,0,strlen(RX_buffer));
    ix = 0;

    UART_Write_String("S-,");
    UART_Write_String(name);
    UART_Write_String("\r");
    count = 0;
    while(strstr(RX_buffer,"AOK")==NULL){       // wait for answer (not most efficient implementation, but we have to wait anyways)
        count++;                                // increment counter
        if(count>75000){                      // if time passed, send command again
            UART_Write_String("S-,");
            UART_Write_String(name);
            UART_Write_String("\r");
            count = 0;
        }
    }
    // reset UART RX buffer
    memset(RX_buffer,0,strlen(RX_buffer));
    ix = 0;    

    UART_Write_String("R,1\r");                 // Reboot device for configuration to take effect
    count = 0;
    while(strstr(RX_buffer,"Rebooting")==NULL){       // wait for answer (not most efficient implementation, but we have to wait anyways)
        count++;                                // increment counter
        if(count>75000){                      // if time passed, send command again
            UART_Write_String("R,1\r");
            count = 0;
        }
    }
    // reset UART RX buffer
    memset(RX_buffer,0,strlen(RX_buffer));
    ix = 0;    
        
    return 1;                                   // Confirm change
}

int packetHandler(){    

    if(strstr(RX_buffer,"hello there")!=NULL){
        UART_Write_String("General Kenobi\n");
                
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }
    else if(strstr(RX_buffer,"getTemp")!=NULL){
        // Init MAX for temp conversion
        status = init_MAX31856();           // assign result to status to avoid being optimized out
        // Read the temperature from the MAX
        volatile short int temp = read_MAX31856_temp();
        
        char answer[20];
        //sprintf(answer,"%d:%i\r",ID,temp);
        sprintf(answer,"%d:TEMPACK:%i\r",ID,temp);
        UART_Write_String(answer);
        
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        
        /*unsigned long count = 0;
        while(strstr(RX_buffer,"tempACK")==NULL){        // wait for answer (not most efficient implementation, but we have to wait anyways)
            count++;            // increment counter
            if(count>75000){    // if time passed, send temperature again
                UART_Write_String(answer);
                UART_Write_String("noACK\n");
                count = 0;
            }
        }*/
        //UART_Write_String("Confirmed");
                
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }
    else if(strstr(RX_buffer,"changeID:")!=NULL){
        char *pos = strstr(RX_buffer,"changeID:") + strlen("changeID:");
        char *end;
        ID = strtol(pos,&end,10);
        
        __delay_ms(500); // wait 500ms for the config device
        
        char answer[50];
        sprintf(answer,"changeID:%d:IDACK\r",ID);
        UART_Write_String(answer);
                
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        return 1;
    }  
    else if(strstr(RX_buffer,"changeName:")!=NULL){
        char *txt = strstr(RX_buffer,"changeName:") + strlen("changeName:");        
        sprintf(name,"%s%d",txt,ID);

        __delay_ms(500); // wait 500ms for the config device
        
        // send back ACK
        char answer[50];
        sprintf(answer,"changeName:nameACK\r");
        UART_Write_String(answer);
        
        __delay_ms(100); // wait 100ms(datasheet) so the RN4870 can separate the data and commands
        
        // change the name of the BLE module as well
        RN4870_changeName(name);
                
        return 1;
    }
    else if(strstr(RX_buffer,"DISCONNECT")!=NULL){             
        // reset UART RX buffer
        memset(RX_buffer,0,strlen(RX_buffer));
        ix = 0;
        
        // Set wake up
        init_interrupts();
        
        // Go to sleep
        nap();
        
        return 1;
    }
    return 0;
}

void spi_init(){       
    TRISAbits.RA5 = 0;      // CS pin ==> output
    TRISBbits.RB0 = 1;      // PIC SDI (SDO of MAX) ==> input
    TRISBbits.RB1 = 0;      // SCK pin ==> output because this is Master
    TRISBbits.RB3 = 0;      // SDO pin
    TRISBbits.RB4 = 1;      // DRDY pin ==> input
    
    // Reset ANSEL bits to enable digital ports
    ANSELAbits.ANSA5 = 0;
    ANSELB = 0x24;
    
    LATAbits.LA5 = 1;       // CS = active low
    
    INTCONbits.GIE = 0;     // Disable interrupts
    
    PMD1bits.MSSPMD = 0;    // Turn on SPI module
    RCONbits.IPEN = 1;      // Enable interrupt priorities
    IPR1bits.SSPIP = 1;     // Set SPI interrupt priority high
    PIE1bits.SSPIE = 1;     // Enable SPI interrupts (PIR1.SSPIF is the related interrupt)
    SSP1CON1bits.CKP = 0;   // Low idle level
    SSP1CON1bits.SSPM = 0b0010; // Master mode + Fosc/12

    PIR1bits.SSPIF = 0;     // Clear interrupt flag
    
    SSP1CON1bits.SSPEN = 1; // Enable SPI
    INTCONbits.GIE = 1;     // Enable interrupts
}

void spi_send(char data){
    LATAbits.LA5 = 0;       // CS to low
    SSP1BUF = data;         // write to buffer for TX
}

char spi_read(){
    return SSP1BUF;
}

short int read_MAX31856_temp(){  
// read first reg
    while(PORTBbits.RB4);       // Wait for dataready     
    spi_send(0x0C);   
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    volatile char value = spi_read();         // dummy read
     
// bits are shifted so byte2 only arrives now!!!    
    spi_send(0xFF);             // dummy send to shift data
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    volatile char byte2 = spi_read();
    
// read second reg
    spi_send(0xFF);             // dummy send to shift data
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    volatile char byte1 = spi_read();         // dummy read

// read third reg   
    spi_send(0xFF);             // dummy send to shift data
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    volatile char byte0 = spi_read();
    
    LATAbits.LA5 = 1;           // CS to high
    
    return convertTemp(byte2, byte1);   // bytes have to be converted to right format
}

short int convertTemp(char byte2, char byte1){
    volatile char result = byte2 & (0b10000000);
    volatile char sign = false;
    
    if(result != 0){
        sign = true;
        byte2 = byte2 & (0b01111111);   // Clear sign(will be used later)
    }
    
    volatile short int temp = (byte2<<4) | (byte1>>4);
    if(sign){
        temp = temp - 2048;         // sign bit: -2^11=-2048
    }
    
    return temp;
}

char init_MAX31856(){
// Init MAX for temp conversion
    spi_send(0x81);                 // write address of Config1 reg
    while(!drdy);                   // Wait for transmission to complete
    drdy = false;
    spi_send(0x43);                 // Select K type + 16 samples average   
    while(!drdy);                   // Wait for transmision to complete
    drdy = false;
    LATAbits.LA5 = 1;               // CS to high
    spi_send(0x80);                 // write address of Config0 reg
    while(!drdy);                   // Wait for transmission to complete
    drdy = false;
    spi_send(0x41);                 // Select rejection of 50Hz + enable 1-shot temp conversion   
    while(!drdy);                   // Wait for transmision to complete
    drdy = false;
        
    LATAbits.LA5 = 1;               // CS to high
    return 1;
}

void init_interrupts(){
    PORTBbits.RB5 = 1;
    TRISBbits.RB5 = 1;      // interrupt pin
    ANSELBbits.ANSB5 = 0;   // enable dig port
    
    INTCONbits.GIE = 0;     // disable interrupts
    RCONbits.IPEN = 1;      // enable interrupt priorities
    INTCONbits.IOCIE = 1;   // enable interrupt on change
    INTCON2bits.IOCIP = 1;  // interrupt priority = high
    IOCBbits.IOCB5 = 1;     // enable interrupt on change of RB5
    
    INTCONbits.IOCIF = 0;   // clear flag
    
    INTCONbits.GIE = 1;     // enable interrupts
}

void nap(){
// clear all pins    
    LATA = 0;
    LATB = 0;
    LATC = 0;
// go to sleep    
    SLEEP();
}

void powerSave(){
// LDO off in sleep
    VREGCON = 0x01;
// turn off all unneeded modules
    PMD0bits.USBMD = 1;     // USB Peripheral Module Disabled
    PMD0bits.ACTMD = 1;     // Active Clock Tuning Peripheral Module Disabled
    PMD0bits.TMR3MD = 1;    // Timer3 Peripheral Module Disabled
    PMD0bits.TMR2MD = 1;    // Timer2 Peripheral Module Disabled
    PMD0bits.TMR1MD = 1;    // Timer1 Peripheral Module Disabled

    PMD1bits.CTMUMD = 1;    // CTMU Peripheral Module Disabled
    PMD1bits.CMP2MD = 1;    // Comparator 2 Peripheral Module Disabled
    PMD1bits.CMP1MD = 1;    // Comparator 1 Peripheral Module Disabled
    PMD1bits.ADCMD = 1;     // Analog-to-Digital Converter Peripheral Module Disabled
    PMD1bits.CCP2MD = 1;    // CCP2 Peripheral Module Disabled
    PMD1bits.CCP1MD = 1;    // CCP1 Peripheral Module Disabled
}
/**
 End of File
*/