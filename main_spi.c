/*
 * File:   main_spi.c
 * Author: Vincent S
 *
 * Created on February 23, 2021, 2:53 PM
 */
#pragma config SDOMX = 0    // Use RC7 as SDO

#define true 1
#define false 0
#define _XTAL_FREQ 16000000

#include <xc.h>

/*Prototypes*/
void chip_init();
void spi_init();
void spi_send(char data);
char spi_read();
short int read_MAX31856_temp();
short int convertTemp(char byte2, char byte1);
char init_MAX31856();

/*Global variables*/
char drdy = false;
volatile char status = 0;

void __interrupt (high_priority) high_ISR(void){
    
    if(PIR1bits.SSPIF == 1){    // In case of SPI interrupt
        drdy = true;  
        //LATAbits.LA2 = 1;
        PIR1bits.SSPIF = 0; // Clear the flag
    }
    
}

void main(void) {
    chip_init();
    spi_init();
    TRISAbits.RA1 = 0;
    TRISAbits.RA2 = 0;
    TRISAbits.RA3 = 0;
    TRISBbits.RB5 = 0;
    TRISBbits.RB6 = 0;    
    
    // Init MAX for temp conversion
    status = init_MAX31856();           // assign result to status to avoid being optimized out
    
    volatile short int temp = read_MAX31856_temp();
    if(temp < 30)  LATBbits.LB6 = 1;
    if(temp > 15)  LATBbits.LB5 = 1;
    if(temp < 15)  LATAbits.LA3 = 1;
    if(temp > 30)  LATAbits.LA2 = 1;
    if(temp > 0)   LATAbits.LA1 = 1;
          
    while(true){
             
        //_delay(10000000);

    }
    return;
}

void chip_init(){
    OSCTUNE = 0x80;
    OSCCON = 0x70;
    OSCCON2 = 0x90;
    
    LATA = 0;
    LATB = 0;
    LATC = 0;
}

void spi_init(){
    TRISAbits.RA5 = 0;      // CS pin ==> output
    TRISBbits.RB0 = 1;      // PIC SDI (SDO of MAX) ==> input
    TRISBbits.RB1 = 0;      // SCK pin ==> output because this is Master
    TRISBbits.RB4 = 1;      // DRDY pin ==> input
    TRISCbits.RC7 = 0;      // SDO pin ==> output
    
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
    
    // Confirm that SPI is enabled by turning on LED at LA1
    //LATAbits.LA1 = SSP1CON1bits.SSPEN;
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
    char value = spi_read();    // dummy read

// bits are shifted so byte2 only arrives now!!!    
    spi_send(0x0C);   
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    char byte2 = spi_read();
    
// read second reg
    spi_send(0x0D);   
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    value = spi_read();         // dummy read

// bits are shifted so byte1 only arrives now!!!    
    spi_send(0x0D);   
    while(!drdy);               // Wait for transmission to complete
    drdy = false;
    char byte1 = spi_read();
    
    LATAbits.LA5 = 1;           // CS to high
    
    return convertTemp(byte2, byte1);
    
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