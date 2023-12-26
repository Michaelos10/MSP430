
//******************************************************************************
//  MSP430FR4133  - eUSCI_B0 I2C Master TX bytes to PCF8574 Slave
//
//  Description: This code connects MSP430 and PCF8574 via the I2C bus.
//  The master transmits to the I2C slave address 0x27
//
//  ACLK = REFO = 32768Hz, MCLK = SMCLK = default DCO = ~1MHz
//  Use with MSP430G6021_uscib0_i2c_16.c
//
//                                /|\  /|\
//                   PCF8574       10k  10k     MSP430FR4133
//                   slave         |    |        master
//             -----------------   |    |   -----------------
//            |              SDA|<-|----|->|P5.2/UCB0SDA     |
//            |                 |  |       |                 |
//            |                 |  |       |                 |
//            |              SCL|<-|------>|P5.3/UCB0SCL     |
//            |                 |          |                 |
//
//  Sunday O. Michael
//  Dec. 2023
//  Built on Code Composer Studio v12.2
//******************************************************************************

#include <msp430.h>

#define SlaveAddress 0x27


unsigned char TXByteCtr;
unsigned char data_t[4];
unsigned char SlaveFlag = 0;

//unsigned char data =0xff;
int i=0;
char data_u, data_l,dat;


/*****************DECLARED FUNCTIONS**********/
void Start_Send(void);
void lcd_send_data (char data);

void display_clear(void);
void lcd_put_cur(int row, int col);
void lcd_send_string (char *str);
void i2c_init(void);



int main(void)
{
     WDTCTL = WDTPW | WDTHOLD;                         // Stop watchdog timer

     i2c_init();

    lcd_init();
    __delay_cycles(500000);
    display_clear();

   while(1)
   {
       lcd_put_cur(0,0);
          __delay_cycles(100);
             lcd_send_string("  PREMAUDA DES-");
             lcd_put_cur(1,0);
            lcd_send_string(" IGNS & SYSTEMS");
            __delay_cycles(5000000);
              display_clear();

              lcd_put_cur(0,0);
              lcd_send_string("   DESIGNED BY ");
              lcd_put_cur(1,0);
             lcd_send_string("SUNDAY O. MIKE");
             __delay_cycles(5000000);
              display_clear();
   }

}
void i2c_init(void){
    // Configure Pins for I2C
    P5SEL0 |= BIT2 | BIT3;                            // I2C pins
    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;
    // Configure USCI_B0 for I2C mode
    UCB0CTLW0 |= UCSWRST;                             // put eUSCI_B in reset state
    UCB0CTLW0 |= UCMODE_3 | UCMST;                    // I2C master mode, SMCLK
    UCB0BRW = 0x8;                                    // baudrate = SMCLK / 8
    UCB0TBCNT = 0x0004;                     // number of bytes to be transmitted
    UCB0CTLW0 &=~ UCSWRST;                            // clear reset register
    UCB0IE |= UCTXIE0 | UCNACKIE;                     // transmit and NACK interrupt enable
    UCB0I2CSA =SlaveAddress;             // configure slave address
}

void Start_Send(void){
  __delay_cycles(100);                             // Delay between transmissions
    TXByteCtr = 0;                                    // Load TX byte counter
    while (UCB0CTLW0 & UCTXSTP);                      // Ensure stop condition got sent
    UCB0CTLW0 |= UCTR | UCTXSTT;                      // I2C TX, start condition
    __bis_SR_register(LPM0_bits | GIE);               // Enter LPM0 w/ interrupts
}

void lcd_send_data (char data){
    int j;
    char data_u, data_l,dat;
    data_u =  (data&0xf0);          //shifts upper 4bits Data to the upper side of chip's register
    data_l = ((data<<4)&0xf0);      //shifts lower 4bits Data to the upper side of chip's register
    data_t[0] = data_u|0x0D;      //en=1, rs=1
    data_t[1] = data_u|0x09;     //en=0, rs=1
    data_t[2] = data_l|0x0D;  //en=1, rs=1
    data_t[3] = data_l|0x09;  //en=0, rs=1

    Start_Send();    //  Transmit start condition


}
void lcd_send_cmd (char cmd){
  char data_u, data_l;

    data_u = (cmd&0xf0);
    data_l = ((cmd<<4)&0xf0);
    data_t[0] = data_u|0x0C;  //en=1, rs=0
    data_t[1] = data_u|0x08;  //en=0, rs=0
    data_t[2] = data_l|0x0C;  //en=1, rs=0
    data_t[3] = data_l|0x08;  //en=0, rs=0

        Start_Send();    //  Transmit start condition
}

void lcd_init (void)
{
    // 4 bit initialisation
    __delay_cycles(20);  // wait for >40ms
    lcd_send_cmd (0x30);
    __delay_cycles(5);  // wait for >4.1ms
    lcd_send_cmd (0x30);
    __delay_cycles(0.1);  // wait for >100us
    lcd_send_cmd (0x30);
    lcd_send_cmd (0x20);  // 4bit mode
    __delay_cycles(100);

  // dislay initialisation
    lcd_send_cmd (0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)

    lcd_send_cmd (0x01);  // clear display

    lcd_send_cmd (0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)

    lcd_send_cmd (0x0C); //Display on  --> D = 1, C and B = 0. (Cursor and blink, last two bits)
}

void lcd_send_string (char *str){
    while (*str) lcd_send_data (*str++);
}

void lcd_put_cur(int row, int col){
    switch (row){
        case 0:
            col |= 0x80;;
        break;
        case 1:
            col |= 0xC0;
        break;
    }
    lcd_send_cmd(col);
}

void display_clear(void){
    lcd_send_cmd(0x01);
    __delay_cycles(2);
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCIB0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCB0IV,0x1E))
  {
        case USCI_NONE:          break;               // Vector 0: No interrupts break;
        case USCI_I2C_UCALIFG:   break;
        case USCI_I2C_UCNACKIFG:
            UCB0CTL1 |= UCTXSTT;                      //resend start if NACK
          break;                                      // Vector 4: NACKIFG break;
        case USCI_I2C_UCSTTIFG:  break;               // Vector 6: STTIFG break;
        case USCI_I2C_UCSTPIFG:  break;               // Vector 8: STPIFG break;
        case USCI_I2C_UCRXIFG3:  break;               // Vector 10: RXIFG3 break;
        case USCI_I2C_UCTXIFG3:  break;               // Vector 14: TXIFG3 break;
        case USCI_I2C_UCRXIFG2:  break;               // Vector 16: RXIFG2 break;
        case USCI_I2C_UCTXIFG2:  break;               // Vector 18: TXIFG2 break;
        case USCI_I2C_UCRXIFG1:  break;               // Vector 20: RXIFG1 break;
        case USCI_I2C_UCTXIFG1:  break;               // Vector 22: TXIFG1 break;
        case USCI_I2C_UCRXIFG0:  break;               // Vector 24: RXIFG0 break;
        case USCI_I2C_UCTXIFG0:
          if (TXByteCtr<4)                                // Check TX byte counter
           {
            UCB0TXBUF = data_t[SlaveFlag++];            // Load TX buffer
            TXByteCtr++;                              // increment TX byte counter
           }
        else
           {
            UCB0CTLW0 |= UCTXSTP;                     // I2C stop condition
            UCB0IFG &= ~UCTXIFG;                      // Clear USCI_B0 TX int flag
            SlaveFlag=0;                               //Returns to zero
            TXByteCtr=0;
            __bic_SR_register_on_exit(LPM0_bits);     // Exit LPM0
           }
          break;                                      // Vector 26: TXIFG0 break;
        case USCI_I2C_UCBCNTIFG: break;               // Vector 28: BCNTIFG
        case USCI_I2C_UCCLTOIFG: break;               // Vector 30: clock low timeout
        case USCI_I2C_UCBIT9IFG: break;               // Vector 32: 9th bit
        default: break;
  }
}
