/*
 * File:   main_sleep.c
 * Author: Vincent S
 *
 * Created on 9 maart 2021, 15:37
 */


#define true 1
#define false 0
#define _XTAL_FREQ 16000000

#include <xc.h>
#include "mcc_generated_files/mcc.h"

/*Prototypes*/
void init_chip();
void init_interrupts();
void init_TMR0();
void nap();
void spi_init();
void spi_send(char data);
char spi_read();
short int read_MAX31856_temp();
short int convertTemp(char byte2, char byte1);
char init_MAX31856();
void UART_TX_Init();
void UART_Write(char data);

/*Global variables*/
char cycle = false;
char drdy = false;
volatile char status = 0;

volatile char byte0 = 0;     // low byte(temp)
volatile char byte1 = 0;     // middle byte(temp)
volatile char byte2 = 0;     // high byte(temp)
volatile char value = 0;
volatile char value2 = 0;

void __interrupt (high_priority) high_ISR(void){
    
    if(INTCONbits.IOCIF == 1){    // In case of IOC interrupt
        cycle = true;
        LATAbits.LA3 = 1;
        INTCONbits.IOCIF = 0; // Clear the flag
    }
    
    if(PIR1bits.SSPIF == 1){    // In case of SPI interrupt
        drdy = true;  
        //LATAbits.LA2 = 1;
        PIR1bits.SSPIF = 0; // Clear the flag
    }
    
}

void main(void) {
    // Initialize the device
    SYSTEM_Initialize();
    init_chip();
    spi_init();
    
    // Initialize all status LED outputs
    TRISAbits.RA1 = 0;
    TRISAbits.RA2 = 0;
    TRISAbits.RA3 = 0;
    TRISCbits.RC1 = 0;
    TRISCbits.RC2 = 0;
        
    // Init MAX for temp conversion
    status = init_MAX31856();           // assign result to status to avoid being optimized out
    // Read the temperature from the MAX
    volatile short int temp = read_MAX31856_temp();
    // Light up some status LEDs
    if(temp < 30)  LATCbits.LC1 = 1;
    if(temp > 10)  LATCbits.LC2 = 1;
    
    // Init UART communication
    UART_TX_Init();
    
    // Send the temperature over UART
    UART_Write(byte2);  // send MSB
    UART_Write(byte1);  // send LSB
    
    // Set the wake up
    init_interrupts();
    
    _delay(30000000);   // wait a bit so it's better visual for the eye
    
    LATAbits.LA1 = 1;
    
    _delay(10000000);   // wait a bit so it's better visual for the eye
    
    // Go to sleep
    nap();
     
    
    while(1){
        if(cycle){  // Cycle once when an IOC is fired
            // Init SPI again
            spi_init();
            // Init MAX for temp conversion
            status = init_MAX31856();           // assign result to status to avoid being optimized out
        
            // Read the temperature from the MAX
            temp = read_MAX31856_temp();  
            // Light up some status LEDs
            if(temp < 30)  LATCbits.LC1 = 1;
            if(temp > 10)  LATCbits.LC2 = 1;
            LATAbits.LA1 = 1;
            
            // Init UART communication
            UART_TX_Init();

            // Send the temperature over UART
            UART_Write(byte2);  // send MSB
            UART_Write(byte1);  // send LSB
            
            // Reset the flag
            cycle = false;
            
            _delay(20000000);   // wait a bit so it's better visual for the eye
            
            // Set wake-up
            init_interrupts();
            // Go to sleep
            nap();
        }       
    }
    return;
}

void init_chip(){
    LATA = 0;
    LATB = 0;
    LATC = 0;
    
    ANSELBbits.ANSB3 = 0;
}

void init_interrupts(){
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
    value = spi_read();         // dummy read
     
// bits are shifted so byte2 only arrives now!!!    
    spi_send(0xFF);             // dummy send to shift data
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    byte2 = spi_read();
    
// read second reg
    spi_send(0xFF);             // dummy send to shift data
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    byte1 = spi_read();         // dummy read

// read third reg   
    spi_send(0xFF);             // dummy send to shift data
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    byte0 = spi_read();
    
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

void UART_TX_Init(void){
  TXSTA1bits.SYNC = 0;              // Asynchronous mode
  RCSTA1bits.SPEN = 1;              // Enable Serial port
  TXSTA1bits.BRGH = 0;              // Set For High-Speed Baud Rate
  BAUDCON1bits.BRG16 = 0;           // 8-bit Baud Rate Generator
  SPBRG1 = 25;                      // Set The Baud Rate To Be 9600 bps
  
  
  // Set The RX-TX Pins to be in UART mode 
  TRISCbits.RC6 = 1; 
  TRISCbits.RC7 = 1; 
  
  TXSTA1bits.TXEN = 1; // Enable UART Transmission
}

void UART_Write(char data){
  while(!TXSTA1bits.TRMT);  // While TSR register is not empty 
  TXREG1 = data;            // Load the data to the transmit buffer
}