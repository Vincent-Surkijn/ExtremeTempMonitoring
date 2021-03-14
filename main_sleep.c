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

/*Global variables*/
char cycle = false;
char drdy = false;
volatile char status = 0;

void __interrupt (high_priority) high_ISR(void){
    
    if(INTCONbits.IOCIF == 1){    // In case of IOC interrupt
        cycle = true;
        //LATAbits.LA1 = 1;
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
    TRISAbits.RA1 = 0;
    LATAbits.LA1 = 1;
    
    // Init MAX for temp conversion
    status = init_MAX31856();           // assign result to status to avoid being optimized out
    
    volatile short int temp = read_MAX31856_temp();
    if(temp == 0)  LATAbits.LA2 = 1;
    if(temp > 0)  LATAbits.LA3 = 1;
    
    init_interrupts();
    
    _delay(300000);
    
    LATAbits.LA1 = 1;
    
    _delay(100000);
    
    //nap();  // go to sleep
     
    
    while(1){
        //spi_send(0x0F);
        /*if(cycle){
            // Init MAX for temp conversion
            status = init_MAX31856();           // assign result to status to avoid being optimized out
        
            temp = read_MAX31856_temp();
            if(temp < 30)  LATAbits.LA2 = 1;
            if(temp > 15)  LATAbits.LA3 = 1;
            LATAbits.LA1 = 1;
            cycle = false;
            _delay(200000);
            nap();
        }
        
        /*LATAbits.LA1 = 1;
        _delay(100000);
        LATAbits.LA1 = 0;
        _delay(100000);*/
        
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
    TRISBbits.RB5 = 1;  // interrupt pin
    
    INTCONbits.GIE = 0;     // disable interrupts
    RCONbits.IPEN = 1;      // enable interrupt priorities
    INTCONbits.IOCIE = 1;   // enable interrupt on change
    INTCON2bits.IOCIP = 1;  // interrupt priority = high
    IOCBbits.IOCB5 = 1;     // enable interrupt on change of RB5
    
    INTCONbits.IOCIF = 0;   // clear flag
    
    INTCONbits.GIE = 1;     // enable interrupts
}

/*void init_TMR0(){   // nog niet getest!!!
    INTCONbits.GIE = 0;     // disable interrupts
    INTCONbits.TMR0IE = 1;  // enable tmr0 interrupts
    RCONbits.IPEN = 1;      // enable interrupt priorities
    INTCON2bits.TMR0IP = 1; // interrupt priority = high
    T0CON = 0b0001 0111     // Fosc/4, prescaler = 256  --> 4.2s
}*/

void nap(){
// clear all pins    
    LATA = 0;
    LATB = 0;
    LATC = 0;
    
    SLEEP();
}

void spi_init(){       
    TRISAbits.RA5 = 0;      // CS pin ==> output
    TRISBbits.RB0 = 1;      // PIC SDI (SDO of MAX) ==> input
    TRISBbits.RB1 = 0;      // SCK pin ==> output because this is Master
    TRISBbits.RB3 = 0;      // SDO pin
    TRISBbits.RB4 = 1;      // DRDY pin ==> input
    //TRISCbits.RC7 = 0;      // SDO pin ==> output
    
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