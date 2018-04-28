/*test RDA5807M   OK */  
// functional

/* This program communicate with the RDA5807M via I2C. */
// inspired from Arduino example:  https://forum.arduino.cc/index.php?topic=221368.0
// I2C inspiration from book:      TI ARM Peripherals Programming and Interfacing-Muhammad Ali Mazidi
/*
  I2C1SCL PA6
  I2C1SDA PA7 */


/*   Connections
  RDA5807M     TIVA board
   +3V  ----->  +3.3V
	 GND  ----->  GND
	 SCL  ----->  PA6
	 SDA  ----->  PA7

*/


#include "TM4C123GH6PM.h"

#define SLAVE_ADDR 0x11   // Device address 0x11 (random access)

unsigned char freqH = 0, freqL = 0, c, d, i;
unsigned int frequencyB, freq, freqB;

void I2C1_init(void);
char I2C1_burstWrite(int slaveAddr, char memAddr, int byteCount, char* data); //  adresa modulRDA(slave)+adresa registrului din RDA+ 
                                                                              //  +nr octeti care se transmit+date
void delayMs(int n);


int main(void)
{
    I2C1_init(); 
	  freq = 887;                            // 88.7 MHz   FM radio station
    freqB = freq - 870;
    freqH = freqB>>2;
    freqL = (freqB&3)<<6;                  // Shift channel selection for matching register 0x03

	  char RDA_registers_1[] = {0xC0, 0x03};    // soft reset, enable
    char RDA_registers_2[] = {0xC0, 0x0D};    // write 0xC00D (RDS on etc.) 
    char RDA_registers_3[] = {freqH, freqL + 0x10};  //write frequency into bits 15:6, set tune bit
    
    I2C1_burstWrite(SLAVE_ADDR, 0x02, 2, &RDA_registers_1[0]);
	  delayMs(500);
	  I2C1_burstWrite(SLAVE_ADDR, 0x02, 2, &RDA_registers_2[0]);
    delayMs(500);
	  I2C1_burstWrite(SLAVE_ADDR, 0x03, 2, &RDA_registers_3[0]);
    for (;;)
    {
    }
}

/* initialize I2C1 as master and the port pins */
void I2C1_init(void)
{
    SYSCTL->RCGCI2C |= 0x02;    /* enable clock to I2C1 */
    SYSCTL->RCGCGPIO |= 0x01;   /* enable clock to GPIOA */

    /* PORTA 7, 6 for I2C1 */
    GPIOA->AFSEL |= 0xC0;       /* PORTA 7, 6 for I2C1 */
    GPIOA->PCTL &= ~0xFF000000; /* PORTA 7, 6 for I2C1 */
    GPIOA->PCTL |= 0x33000000;
    GPIOA->DEN |= 0xC0;         /* PORTA 7, 6 as digital pins */
    GPIOA->ODR |= 0x80;         /* PORTA 7 as open drain */

    I2C1->MCR = 0x10;           /* master mode */
    I2C1->MTPR = 7;             /* 100 kHz @ 16 MHz */
}


/* Wait until I2C master is not busy and return error code */
/* If there is no error, return 0 */
static int I2C_wait_till_done(void)
{
    while(I2C1->MCS & 1);   /* wait until I2C master is not busy */
    return I2C1->MCS & 0xE; /* return I2C error code */
}

/* Use burst write to write multiple bytes to consecutive locations */
/* burst write: S-(saddr+w)-ACK-maddr-ACK-data-ACK-data-ACK-...-data-ACK-P */
char I2C1_burstWrite(int slaveAddr, char memAddr, int byteCount, char* data)
{   
    char error;
    
    if (byteCount <= 0)
        return -1;                  /* no write was performed */

    /* send slave address and starting address */
    I2C1->MSA = slaveAddr << 1;
    I2C1->MDR = memAddr;
    I2C1->MCS = 3;                  /* S-(saddr+w)-ACK-maddr-ACK */

    error = I2C_wait_till_done();   /* wait until write is complete */
    if (error) return error;

    /* send data one byte at a time */
    while (byteCount > 1)
    {
        I2C1->MDR = *data++;             /* write the next byte */
        I2C1->MCS = 1;                   /* -data-ACK- */
        error = I2C_wait_till_done();
        if (error) return error;
        byteCount--;
    }
    
    /* send last byte and a STOP */
    I2C1->MDR = *data++;                 /* write the last byte */
    I2C1->MCS = 5;                       /* -data-ACK-P */
    error = I2C_wait_till_done();
    while(I2C1->MCS & 0x40);             /* wait until bus is not busy */
    if (error) return error;

    return 0;       /* no error */
}

/* delay n milliseconds (16 MHz CPU clock) */
void delayMs(int n)
{
    int i, j;
    for(i = 0 ; i < n; i++)
        for(j = 0; j < 3180; j++)
            {}  /* do nothing for 1 ms */
}
