/*
 * EE 3310 Final Project, Spring 2019
 * Patrick Cannell, 1001453921
 * 
 * File: multimeterMain.c
 * Author: Patrick Cannell
 * Professor: Dr. Gibbs
 * Class: EE3310
 * 
 * 
 * 
 * DISCLAIMER: 
 * All work in this project was performed in accordance
 * with instructions given above and in compliance with
 * the University of Texas at Arlington?s Honor Code.
 */

// Include Files
#include "../3310header_2192.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include <xc.h>

// #define
#define LCD_data LATB
#define LCD_rs   _LATA2
#define LCD_en   _LATA3
#define LCD_rw   _LATA1

// Global Variables
double Vref = 3.287 ;
int  u16_adcVal= 0 ; 
double f_adcVal =0  ; 

void IO_Config(void) 
{
    TRISA = 0b00011;
    LATA = 0b00000;
    TRISB = 0b1100011100000000; //0000 0011 0000 0000
    _CN22PUE = 1 ; _CN21PUE = 1 ; _CN16PUE = 1;
    LATB = 0x0000;
}

void T1_Config(void) { // 37 usec * 3 = 111 usec
    T1CONbits.TCKPS = 0b00;
    PR1 = 443;
    _T1IF = 0;
    _T1IE = 0;
    TMR1 = 0;
    T1CONbits.TON = 0;
}

void T2_Config(void) { // Varies with need.  TCKPS and PR2 will be set before using T2.
    T2CONbits.TCKPS = 0b00;
    _T2IF = 0;
    _T2IE = 0;
    TMR2 = 0;
    T2CONbits.TON = 0;
}

void T3_Config(void) {
    T3CONbits.TCKPS = 0b01;
    PR3 = 59999;
    _T3IF = 0;
    _T3IE = 0;
    TMR3 = 0;
    T3CONbits.TON = 0;
}

void LCD_short_busy(void) { // 100 usec
    TMR1 = 0;
    T1CONbits.TON = 1;
    while (!_T1IF);
    T1CONbits.TON = 0;
    TMR1 = 0;
    _T1IF = 0;
} // end LCD_short_busy

void LCD_long_busy(void) { // Set TCKPS and PR2 before calling
    TMR2 = 0;
    T2CONbits.TON = 1;
    while (!_T2IF);
    T2CONbits.TON = 0;
    TMR2 = 0;
    _T2IF = 0;
} // end LCD_long_busy

void LCD_loop_delay(void) {
    TMR3 = 0;
    T3CONbits.TON = 1;
    while (!_T3IF);
    T3CONbits.TON = 0;
    TMR3 = 0;
    _T3IF = 0;
} // end LCD_loop_delay

void LCD_senddata(unsigned char var) { //display single character to assigned address
    LCD_data = var;
    LCD_rs = 1;
    LCD_en = 1;
    LCD_short_busy(); //delay
    LCD_en = 0;
    LCD_short_busy(); //delay
}

void LCD_sendstring(char *var) { //display string to assigned address
    while (*var) {
        LCD_senddata(*var++);
    }//
    LCD_short_busy();
}

void LCD_command(unsigned char var) { //command for LCD register
    LCD_data = var;
    LCD_rs = 0;
    LCD_en = 1;
    LCD_short_busy();
    LCD_en = 0;
    LCD_short_busy();
}

void LCD_init() { // From page 45 of the Hitachi HD44780 data sheet
    // Wait more that 40 msec ( * 3 ) == 120 msec 
    T2CONbits.TCKPS = 0b01;
    PR2 = 59999;
    LCD_long_busy();
    LCD_rs = 0;
    LCD_command(0x0038); //8bit interface, 2 lines display and 5x10dots Font

    // Wait more than 4.1 msec ( * 3) = 12.3 msec
    T2CONbits.TCKPS = 0b00;
    PR2 = 49199;
    LCD_long_busy(); //delay
    LCD_command(0x0038); //8bit interface, 2 lines display and 5x10dots Font 

    // Wait for more than 100 usec ( * 3) = 300 usec
    T2CONbits.TCKPS = 0b00;
    PR2 = 1199;
    LCD_long_busy();
    LCD_command(0x0038); //8bit interface, 2 lines display and 5x10dots Font 

    // Wait for more than 37 * 3 usec = 111 usec
    LCD_short_busy();
    LCD_command(0x0038); //8bit interface, 2 lines display and 5x10dots Font
    LCD_short_busy();
    LCD_command(0x0008); //display on, cursor on, blinking on 
    LCD_short_busy();
    LCD_command(0x0001); //clear display  
    LCD_short_busy();
    LCD_command(0x0007); //assign cursor moving direction and enable shift?
    LCD_short_busy();
}

void config_ADC(void) 
{
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    AD1CSSL = 0;
    AD1PCFG = 0xF9FC; //0b1111 1001 1111 1100             
    AD1CON1 = 0x00E0;               
    AD1CON2 = 0x0000;                                           
    AD1CON3 = 0x9F3E;               
    _AD1IF = 0;                     
    AD1CON1bits.ADON = 1;           
}

int run_ADC(int ch) 
{
    _CH0SA = ch;                                            
    AD1CON1bits.SAMP = 1;                                   
    while (!AD1CON1bits.DONE);                              
    AD1CON1bits.DONE = 0;                                   
    _AD1IF = 0;                                             
    return ADC1BUF0;  
}

void printVoltmeter(void) //takes average of 100 voltage measurements
{
    int i = 0 ;
    double filterVolts = 0;
    double Vpin = 0;
    double Vin = 0 ; //Vin is actually the ADC value of the pin
    double R5 = 857400 ; //859700 
    double R6 = 130060 ;//100320
    char v[] = "Voltmeter";
    char volts[80];
    LCD_command(0x0001);LCD_long_busy();
    //turn on LCD
    LCD_command(0b00001100);LCD_long_busy();   
    //Top left
    LCD_command(0x0080);LCD_long_busy();

    LCD_sendstring(v);LCD_long_busy();  LCD_long_busy(); 
    
    /* START MEASUREMENT */
    for(i = 0 ; i < 1000 ; i ++)
    {
        u16_adcVal = run_ADC(1);                                               
        f_adcVal = u16_adcVal * 1.0;
        filterVolts += f_adcVal;
    }
    Vin = filterVolts/1000.0 ;
    Vpin = 0.024816 * Vin + 0.049245 ;
    //Print dummy value
    sprintf(volts, "%4.4f au", Vpin); //2.4f
    LCD_command(0x0080 + 0x0040);LCD_long_busy();
    LCD_sendstring(volts);LCD_long_busy();
    
}

void printMultimeter(void)
{
    char m[] = "Multimeter";

    LCD_command(0x0001);LCD_long_busy();
    //turn on LCD
    LCD_command(0b00001100);LCD_long_busy();   
    //Top left
    LCD_command(0x0080);LCD_long_busy();

    LCD_sendstring(m);LCD_long_busy();                   
}

void printAmmeter(void)
{
    int vc_adc = 0 ;
    int vd_adc = 0 ;
    char a[] = "Ammeter";
    char amps[80];
    LCD_command(0x0001);LCD_long_busy();
    //turn on LCD
    LCD_command(0b00001100);LCD_long_busy();   
    //Top left
    LCD_command(0x0080);LCD_long_busy();

    LCD_sendstring(a);LCD_long_busy();      
}

void printOhmmeter(void)
{
    char o[] = "Ohmmeter";
    char ohms[80];
    int vc_adc = 9999 ;
    int vd_adc = 9999 ;
    double Vc = 0 ;
    double VR1 = 0 ;
    double current = 0 ;
    double R1 = 102.185 ;
    double R2 = 0 ;
    double R3 = 198.08 ; //low range is 198.08 //medium range is   10099.9                  
    double R4 = 0 ;

    LCD_command(0x0001);LCD_long_busy();
    //turn on LCD
    LCD_command(0b00001100);LCD_long_busy();   
    //Top left
    LCD_command(0x0080);LCD_long_busy();

    LCD_sendstring(o);LCD_long_busy();
    //START OHMMETER ROUNTINE 1 //
    vc_adc = run_ADC(9);
    vd_adc = run_ADC(10);
    while( (vc_adc - vd_adc) != 0 )
    {
        vc_adc = run_ADC(9);
        if( _RB10 == 0 ) //If Vc cannot meet the voltage of the right branch, manually move to Ohmmeter Routine 2
        {
            vc_adc = vd_adc ;
            sprintf(ohms, "%f Vc", Vc); //2.4f
            LCD_command(0x0080);LCD_long_busy();
            LCD_sendstring(ohms);LCD_long_busy();
        }
    }
    /* Part1: get Vc*/
    Vc = vc_adc / 1023.0 * Vref ;
    
    /* Part2: get VR1 */
    VR1 = Vref - Vc ;
    
    /* Part3: get current  */
    current = VR1/R1 ;
    
    /* Part4: get R2 */
     R2 = Vc/current ;
    /* Part5: get R4 */
    R4 = R2 * R3 / R1 ;
    
    sprintf(ohms, "%f Vc", Vc); //2.4f
    LCD_command(0x0080);LCD_long_busy();
    LCD_sendstring(ohms);LCD_long_busy();
    sprintf(ohms, "%f au", R4); //2.4f
    LCD_command(0x0080 + 0x0040);LCD_long_busy();
    LCD_sendstring(ohms);LCD_long_busy();
    // END OHMMETER ROUTINE 1 //
    
    
    //START OHMMETER ROUNTINE 2 //
    if ( Vc > 3.07 )
    {
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring("Vc over 3 V");LCD_long_busy();
        LCD_command(0x0001);LCD_long_busy();
        LCD_command(0b00001100);LCD_long_busy();   
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring("Set SWITCH 5 and POT");LCD_long_busy();
        while(_RB10 != 0 )
        {
            //wait to change DIP switch, the push Ohmmeter button again
        }
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring("Running Ohmmeter on SW5");LCD_long_busy();
        R3 = 10099.9 ;
        vc_adc = run_ADC(9);
        vd_adc = run_ADC(10);
        while( (vc_adc - vd_adc) != 0 )
        {
            vc_adc = run_ADC(9);
            if( _RB10 == 0 ) //If Vc cannot meet the voltage of the right branch, manually move to Ohmmeter Routine 3
            {
                vc_adc = vd_adc ;
                sprintf(ohms, "%f Vc", Vc); //2.4f
                LCD_command(0x0080);LCD_long_busy();
                LCD_sendstring(ohms);LCD_long_busy();
            }
        }
        Vc = vc_adc / 1023.0 * Vref ;
        VR1 = Vref - Vc ;
        current = VR1/R1 ;
        R2 = Vc/current ;
        R4 = R2 * R3 / R1 ;
        sprintf(ohms, "%f Vc", Vc); //2.4f
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring(ohms);LCD_long_busy();
        sprintf(ohms, "%f au", R4); //2.4f
        LCD_command(0x0080 + 0x0040);LCD_long_busy();
        LCD_sendstring(ohms);LCD_long_busy(); 
    }
    // END OHMMETER ROUTINE 2 //
    
    // START OHMMETER ROUTINE 3 //
    if ( Vc > 3.05 &&  R3 == 10099.9 )
    {
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring("Vc over 3 V");LCD_long_busy();
        LCD_command(0x0001);LCD_long_busy();
        LCD_command(0b00001100);LCD_long_busy();   
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring("Set SWITCH 6 and POT");LCD_long_busy();
        while(_RB10 != 0 )
        {
            //wait to change DIP switch, the push Ohmmeter button again
        }
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring("Running Ohmmeter on SW6");LCD_long_busy();
        R3 = 245540;
        vc_adc = run_ADC(9);
        vd_adc = run_ADC(10);
        while( (vc_adc - vd_adc) != 0 )
        {
            vc_adc = run_ADC(9);
        }
        Vc = vc_adc / 1023.0 * Vref ;
        VR1 = Vref - Vc ;
        current = VR1/R1 ;
        R2 = Vc/current ;
        R4 = R2 * R3 / R1 ;
        R4 = R4 *1.05 ;
        sprintf(ohms, "%f Vc", Vc); //2.4f
        LCD_command(0x0080);LCD_long_busy();
        LCD_sendstring(ohms);LCD_long_busy();
        sprintf(ohms, "%f au", R4); //2.4f
        LCD_command(0x0080 + 0x0040);LCD_long_busy();
        LCD_sendstring(ohms);LCD_long_busy(); 
    }
    // END OHMMETER ROUTINE 3 //
}

int main(void) 
{
    char str[] = "Multimeter";
    char value[80];
    IO_Config();
    T1_Config();
    T2_Config();
    T3_Config();
    LCD_init();
    config_ADC();
    LCD_rw = 0;
    
    T2CONbits.TCKPS = 0b10;
    PR2 = 15624;
   
    printMultimeter(); LCD_long_busy();
    while(1)
    {
        if( _RB8 == 0 ) //Voltmeter
        {
            LCD_long_busy(); LCD_long_busy(); //debounce
            printVoltmeter();LCD_long_busy();
        }
        if( _RB9 == 0 ) //Ohmmeter
        {
            LCD_long_busy(); //debounce
            printAmmeter(); LCD_long_busy();
        }
        if( _RB10 == 0 ) //Ammeter
        {
            LCD_long_busy(); //debounce
            printOhmmeter(); LCD_long_busy();
        } 
    }
}