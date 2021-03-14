/*
 * File:   main_byte_conv.c
 * Author: Vincent S
 *
 * Created on 22 februari 2021, 11:23
 */
#define true 1
#define false 0

#include <xc.h>

/****Example 1
volatile char byte2 = 0b10000111;    // Dec=135; Hex=87
volatile char byte1 = 0b11001101;    // Dec=205; Hex=CD
    // End result(11bit variable) = -1924°C
*/

/****Example 2*/
volatile char byte2 = 0b01000111;
volatile char byte1 = 0b11001101;
    // End result(11bit variable) = 1148°C


/****Example 3
volatile char byte2 = 0b00000000;
volatile char byte1 = 0b00000000;
    // End result(11bit variable) = 0°C
*/

volatile char sign;

/*Prototypes*/
short int convertTemp(char byte2, char byte1);

void main(void) {
    volatile short int temp = convertTemp(byte2,byte1);
    
    return;
}

short int convertTemp(char byte2, char byte1){
    volatile char result = byte2 & (0b10000000);
    if(result != 0){
        sign = true;
        byte2 = byte2 & (0b01111111);   // Clear sign(will be used later)
    }
    
    volatile short int temp = (byte2<<4) | (byte1>>4);
    if(sign){
        temp = temp - 2048; // sign bit: -2^11=-2048
    }
    
    return temp;
}