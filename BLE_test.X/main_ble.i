#line 1 "main_ble.c"
#line 1 "main_ble.c"

#line 19 "main_ble.c"
 


#line 1 "./mcc_generated_files/mcc.h"

#line 22 "./mcc_generated_files/mcc.h"
 


#line 45 "./mcc_generated_files/mcc.h"
 


#line 49 "./mcc_generated_files/mcc.h"










#line 68 "./mcc_generated_files/mcc.h"
 
void SYSTEM_Initialize(void);


#line 81 "./mcc_generated_files/mcc.h"
 
void OSCILLATOR_Initialize(void);

#line 85 "./mcc_generated_files/mcc.h"

#line 87 "./mcc_generated_files/mcc.h"
 #line 22 "main_ble.c"


 
void UART_TX_Init();
void UART_Write(char data);
void UART_Write_String(char *buffer);


#line 31 "main_ble.c"
 
void main(void)
{
    
    SYSTEM_Initialize();
    UART_TX_Init();

    
    
    

    
    

    
    

    
    

    
    
    
    UART_Write('$');                
    UART_Write('+');                
    UART_Write_String("SS,C0");     
    UART_Write_String("S-,help");   
    UART_Write_String("R,1");       
    
    TRISBbits.RB0 = 0;
    LATBbits.LB0 = 1;

    while (1)
    {
        
        LATBbits.LB0 = 1;
        _delay(1000000);
        LATBbits.LB0 = 0;
        _delay(1000000);
    }
}

void UART_TX_Init(void){
  TXSTA1bits.SYNC = 0;              
  RCSTA1bits.SPEN = 1;              
  TXSTA1bits.BRGH = 0;              
  BAUDCON1bits.BRG16 = 0;           
  SPBRG1 = 25;                      
  
  
  
  TRISCbits.RC6 = 1; 
  TRISCbits.RC7 = 1; 
  
  TXSTA1bits.TXEN = 1; 
}

void UART_Write(char data){
  while(!TXSTA1bits.TRMT);  
  TXREG1 = data;            
}

void UART_Write_String(char *buffer){
    for(int i = 0 ; i<sizeof(buffer);i++){
      UART_Write(buffer[i]);
    }
}

#line 100 "main_ble.c"
 