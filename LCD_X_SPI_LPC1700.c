/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2012  SEGGER Microcontroller GmbH & Co. KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.16 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software has been licensed to  ARM LIMITED whose registered office
is situated at  110 Fulbourn Road,  Cambridge CB1 9NJ,  England solely
for  the  purposes  of  creating  libraries  for  ARM7, ARM9, Cortex-M
series,  and   Cortex-R4   processor-based  devices,  sublicensed  and
distributed as part of the  MDK-ARM  Professional  under the terms and
conditions  of  the   End  User  License  supplied  with  the  MDK-ARM
Professional. 
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : LCD_X_SPI_LPC1700.c
Purpose     : Port routines for LPC1700 SPI
----------------------------------------------------------------------
*/

#include "GUI.h"

/*********************************************************************
*
*       Hardware configuration
*
**********************************************************************
*/

#include <LPC17xx.h>

/* SPI Interface: SPI1
 
   PINS: 
   - CS  = P0.6 (GPIO pin)
   - SCK = P0.7 (SCK1)
   - SDO = P0.8 (MISO1)
   - SDI = P0.9 (MOSI1)
*/

/* TWI Interface: GPIO
   
   PINS: 
   - CS  = P0.6 (GPIO pin)
   - SCL = P0.7 (GPIO pin)
   - SDA = P0.9 (GPIO pin)
*/

#define PIN_CS      (1 << 6)
#define PIN_SCL     (1 << 7)
#define PIN_SDA     (1 << 9)

// SPI_SR - bit definitions
#define TFE         0x01
#define RNE         0x04
#define BSY         0x10

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static unsigned short DevID;    /* Device ID */

/*********************************************************************
*
*       Static code
*
*********************************************************************
*/

#define LCD_CS(x)  ((x) ? (LPC_GPIO0->FIOSET  = PIN_CS)  : (LPC_GPIO0->FIOCLR  = PIN_CS))
#define LCD_SCL(x) ((x) ? (LPC_GPIO0->FIOSET  = PIN_SCL) : (LPC_GPIO0->FIOCLR  = PIN_SCL))
#define LCD_SDO(x) ((x) ? (LPC_GPIO0->FIOSET  = PIN_SDA) : (LPC_GPIO0->FIOCLR  = PIN_SDA))

#define LCD_SDI()        ((LPC_GPIO0->FIOPIN  & PIN_SDA) != 0)

#define LCD_DIR(x) ((x) ? (LPC_GPIO0->FIODIR |= PIN_SDA) : (LPC_GPIO0->FIODIR &= ~PIN_SDA))

#define LCD_DLY(x) ((x) ? __nop(), __nop(), __nop(), __nop(), __nop() : __nop())

#ifdef  SPI_SLOWCLOCK
#define SPI_SlowClock(x) (LPC_SSP1->CPSR = x ? 0x04 : 0x02)
#else
#define SPI_SlowClock(x)
#endif

/*********************************************************************
*
*       SPI_Transfer
*
* Purpose:
*   Transfer (Write/Read) 1 byte, MSB first
*/
static unsigned char SPI_Transfer(unsigned char byte) {
  LPC_SSP1->DR = byte;
  while (!(LPC_SSP1->SR & RNE));        // Wait for send to finish
  return (LPC_SSP1->DR);
}

/*********************************************************************
*
*       TWI_Send
*
* Purpose:
*   Send 1 byte, MSB first
*/
static void TWI_Send(unsigned char byte) {
  unsigned int  mask;

  for (mask = 0x80; mask != 0; mask >>= 1) {
    LCD_SCL(0);
    LCD_SDO(byte & mask);
    LCD_DLY(0);
    LCD_SCL(1);
    LCD_DLY(0);
  }
}

/*********************************************************************
*
*       TWI_Recv
*
* Purpose:
*   Receive 1 byte, MSB first
*/
static unsigned char TWI_Recv(void) {
  unsigned char val;
  unsigned int  mask;

  val = 0;
  for (mask = 0x80; mask != 0; mask >>= 1) {
    LCD_SCL(0);
    LCD_DLY(1);
    if (LCD_SDI()) val |= mask;
    LCD_SCL(1);
    LCD_DLY(1);
  }
  return (val);
}

/*********************************************************************
*
*       TWI_Select
*
* Purpose:
*   Select/Unselect TWI
*/
static void TWI_Select(unsigned int select) {

  if (select) {
    // SCL is GPIO output set to high
    LPC_GPIO0->FIOSET     =  PIN_SCL;
    LPC_GPIO0->FIODIR    |=  PIN_SCL;
    // Set MOSI, MISO and SCK as GPIO pins, with pull-down/pull-up disabled
    LPC_PINCON->PINSEL0  &= ~((3 << 18) | (3 << 16) | (3 << 14));
    LPC_PINCON->PINMODE0 |=  0x000AA000;
  } else {
    // Connect MOSI, MISO, and SCK to SSP peripheral
    LPC_GPIO0->FIODIR    &= ~PIN_SCL;
    LPC_PINCON->PINSEL0  |=  ((2 << 18) | (2 << 16) | (2 << 14));
    LPC_PINCON->PINMODE0 &= ~0x000FF000;
  }
}

/*********************************************************************
*
*       Exported code
*
*********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Init
*
* Purpose:
*   This routine should be called from your application program
*   to set port pins to their initial values
*/
void LCD_X_Init(void) {

  // Enable power to GPIO and SSP1, clock = CCLK / 2
  LPC_SC->PCONP       |= 0x00008400;
  LPC_SC->PCLKSEL0    |= 0x00200000;

  // Configure the LCD Control pins
  LPC_PINCON->PINSEL9 &= 0xF0FFFFFF;
  LPC_GPIO4->FIOSET    = 0x20000000;
  LPC_GPIO4->FIODIR   |= 0x30000000;

  // SSEL1 is GPIO output set to high
  LPC_GPIO0->FIOSET    = 0x00000040;
  LPC_GPIO0->FIODIR   |= 0x00000040;

  // Connect MOSI, MISO, and SCK to SSP peripheral
  LPC_PINCON->PINSEL0 &= ~((3 << 18) | (3 << 16) | (3 << 14));
  LPC_PINCON->PINSEL0 |=  ((2 << 18) | (2 << 16) | (2 << 14));

  // Enable SPI in Master Mode, CPOL=1, CPHA=1
  // Max. 12.5 MBit used for Data Transfer @ 100MHz
  LPC_SSP1->CR0        = 0x01C7;
  LPC_SSP1->CPSR       = 0x02;
  LPC_SSP1->CR1        = 0x02;

  // Backlight on
  LPC_GPIO4->FIOSET = 0x10000000;

  // Detect Device (TWI)
  LCD_CS(0);
  SPI_Transfer(0x70);           // Start + WR Register
  SPI_Transfer(0);
  LCD_CS(1);
  TWI_Select(1);
  LCD_CS(0);
  LCD_DIR(1);
  TWI_Send(0x73);               // Start + RD Data
  LCD_DIR(0);
  DevID = TWI_Recv();
  LCD_CS(1);
  TWI_Select(0);

  if (DevID != 0) return;

  // Detect Device (SPI)
  LCD_CS(0);
  SPI_Transfer(0x73);           // Start + RD Data
  SPI_Transfer(0);              // Dummy RD
  DevID  = SPI_Transfer(0) << 8;
  DevID |= SPI_Transfer(0);
  LCD_CS(1);
}

/*********************************************************************
*
*       LCD_X_Write00_16
*
* Purpose:
*   Write to controller, with A0 = 0
*/
void LCD_X_Write00_16(U16 c) {

  LCD_CS(0);
  SPI_Transfer(0x70);           // Start + WR Register
  if (DevID != 0x47) {
    SPI_Transfer((U8)(c >> 8));
  }
  SPI_Transfer((U8)(c));
  LCD_CS(1);
}

/*********************************************************************
*
*       LCD_X_Write01_16
*
* Purpose:
*   Write to controller, with A0 = 1
*/
void LCD_X_Write01_16(U16 c) {

  LCD_CS(0);
  SPI_Transfer(0x72);           // Start + WR Data
  SPI_Transfer((U8)(c >> 8));
  SPI_Transfer((U8)(c));
  LCD_CS(1);
}

/*********************************************************************
*
*       LCD_X_Read01_16
*
* Purpose:
*   Read from controller, with A0 = 1
*/
U16 LCD_X_Read01_16(void) {
  U16 c;

  if (DevID == 0x47) {          // Himax HX8347
    TWI_Select(1);
    LCD_CS(0);
    LCD_DIR(1);
    TWI_Send(0x73);             // Start + RD Data
    LCD_DIR(0);
    c = TWI_Recv();
    LCD_CS(1);
    TWI_Select(0);
  } else {
    LCD_CS(0);
    SPI_Transfer(0x73);         // Start + RD Data
    SPI_Transfer(0);            // Dummy RD
    c  = SPI_Transfer(0) << 8;
    c |= SPI_Transfer(0);
    LCD_CS(1);
  }

  return c;
}

/*********************************************************************
*
*       LCD_X_WriteM01_16
*
* Purpose:
*   Write multiple bytes to controller, with A0 = 1
*/
void LCD_X_WriteM01_16(U16 * pData, int NumWords) {

  LCD_CS(0);
  SPI_Transfer(0x72);           // Start + WR Data
  for (; NumWords; NumWords--) {
    SPI_Transfer((U8)(*pData >> 8));
    SPI_Transfer((U8)(*pData));
    pData++;
  }
  LCD_CS(1);
}

/*********************************************************************
*
*       LCD_X_ReadM01_16
*
* Purpose:
*   Read multiple bytes from controller, with A0 = 1
*/
void LCD_X_ReadM01_16(U16 * pData, int NumWords) {
  U32 v;

  if (DevID == 0x47) {          // Himax HX8347
    if (NumWords != 4) return;
    TWI_Select(1);
    LCD_CS(0);
    LCD_DIR(1);
    TWI_Send(0x73);             // Start + RD Data
    LCD_DIR(0);
    TWI_Recv();                 // Dummy RD
    v = (TWI_Recv() << 16) |
        (TWI_Recv() <<  8) |
        (TWI_Recv() <<  0);
     pData++;
    *pData++ = (v & 0x03F000) >> 10;
    *pData++ = (v & 0xFC0000) >> 16;
    *pData++ = (v & 0x000FC0) >>  4;    
    LCD_CS(1);
    TWI_Select(0);
  } else {
    LCD_CS(0);
    SPI_SlowClock(1);
    SPI_Transfer(0x73);         // Start + RD Data
    SPI_Transfer(0);            // Dummy RD
    SPI_Transfer(0);            // Dummy RD
    SPI_Transfer(0);            // Dummy RD
    SPI_Transfer(0);            // Dummy RD
    SPI_Transfer(0);            // Dummy RD
    for (; NumWords; NumWords--) {
      *pData  = SPI_Transfer(0) << 8;
      *pData |= SPI_Transfer(0);
      *pData++;
    }
    SPI_SlowClock(0);
    LCD_CS(1);
  }
}

/*************************** End of file ****************************/
