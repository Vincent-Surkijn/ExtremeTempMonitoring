/*
 * File:   UART_Sleep_main.c
 * Author: Vincent S
 *
 * Created on 26 april 2021, 13:39
 */

#define true 1
#define false 0
#define _XTAL_FREQ 16000000

#include <xc.h>
#include <stdlib.h>
#include <string.h>

/*Prototypes*/
void init_chip();
void init_interrupts();
void nap();

void UART_Init();
void UART_Write(char data);
void UART_Write_String(char *buffer);

/*Global variables*/
char cycle = false;
char drdy = false;
volatile char status = 0;

char UART_RX;
char RX_buffer[500];
char *expected;
int ix = 0;

void __interrupt (high_priority) high_ISR(void){
    
    if(INTCONbits.IOCIF == 1){    // In case of IOC interrupt (wake up)
        //LATAbits.LA1 = 1;
        INTCONbits.IOCIF = 0; // Clear the flag
    }
    
    if (PIR1bits.RCIF == 1){  // UART receive  

        RX_buffer[ix] = RCREG1;         // Add to buffer
        ix++;                           // Increment buffer index
        LATAbits.LA1 = 1;
         
        if(RCREG1=='\r'){             // Write response to string
            cycle = true;
            ix = 0;                     // Reset buffer
            LATAbits.LA1 = 1;
        }
        //PIR1bits.RCIF = 0;
   }	
    
}

void main(void) {
    init_chip();
    
    // Initialize all status LED outputs
    TRISAbits.RA1 = 0;
    TRISAbits.RA2 = 0;
    TRISAbits.RA3 = 0;
    TRISCbits.RC1 = 0;
    TRISCbits.RC2 = 0;
        
    // Init UART communication
    UART_Init();
    _delay(100000);
    UART_Write_String("Good night\n");
    
    // Set the wake up
    init_interrupts();
    
    //_delay(30000000);
        
    // Go to sleep
    //nap();
     
    while(true){
        if(cycle){
            // Init UART communication
            UART_Init();
            _delay(100000);
            UART_Write_String(RX_buffer);
            UART_Write_String("\nAwake\n");
           // LATAbits.LA1 = 1;
            
            // Reset the flag
            cycle = false;
                        
            // Set wake-up
            init_interrupts();
            
            // Go back to sleep
            //nap();
        }
        
        /*
        LATAbits.LA1 = 1;
        _delay(1000000);
        LATAbits.LA1 = 0;
        _delay(1000000);*/
    }
    
    return;
}

void init_chip(){
    ANSELC = 0x00;
    ANSELB = 0x00;
    ANSELA = 0x00;
    
    LATA = 0;
    LATB = 0;
    LATC = 0;
     
    OSCCON = 0x70;
    OSCCON2 = 0x90;
    OSCTUNE = 0x80;
    ACTCON = 0x00;
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