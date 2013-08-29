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
File        : LCDConf.c
Purpose     : Display controller configuration

              Display:        Ampire 240320L8
              Controller:     Ilitek ILI9320

              Display:        Ampire 240320LD
              Controller:     OriseTech SPFD5408,
                              Raydium RM68050

              Display:        Ampire 240320LG
              Controller:     Himax HX8347-D

              Display driver: GUIDRV_FlexColor
              Operation mode: 16bpp, 16-bit interface or SPI

---------------------------END-OF-HEADER------------------------------
*/

#include "GUI.h"
#include "GUIDRV_FlexColor.h"

/*********************************************************************
*
*       Layer configuration
*
**********************************************************************
*/
//
// Physical display size
//
#define XSIZE_PHYS 240
#define YSIZE_PHYS 320

//
// Color conversion
//
#define COLOR_CONVERSION GUICC_565

//
// Display driver
//
#define DISPLAY_DRIVER GUIDRV_FLEXCOLOR

//
// Buffers / VScreens
//
#define NUM_BUFFERS   1
#define NUM_VSCREENS  1

//
// Display orientation
//
//#define DISPLAY_ORIENTATION  0
//#define DISPLAY_ORIENTATION               (GUI_MIRROR_X | GUI_MIRROR_Y)
  #define DISPLAY_ORIENTATION (GUI_SWAP_XY | GUI_MIRROR_Y)
//#define DISPLAY_ORIENTATION (GUI_SWAP_XY | GUI_MIRROR_X)

//
// Touch screen
//
#define USE_TOUCH   0
//
// Touch screen calibration
#define TOUCH_X_MIN 0x00E0
#define TOUCH_X_MAX 0x0F40
#define TOUCH_Y_MIN 0x00C0
#define TOUCH_Y_MAX 0x0F60

/*********************************************************************
*
*       Configuration checking
*
**********************************************************************
*/
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   COLOR_CONVERSION
  #error Color conversion not defined!
#endif
#ifndef   DISPLAY_DRIVER
  #error No display driver defined!
#endif
#ifndef   NUM_VSCREENS
  #define NUM_VSCREENS 1
#else
  #if (NUM_VSCREENS <= 0)
    #error At least one screeen needs to be defined!
  #endif
#endif
#if (NUM_VSCREENS > 1) && (NUM_BUFFERS > 1)
  #error Virtual screens and multiple buffers are not allowed!
#endif
#ifndef   DISPLAY_ORIENTATION
  #define DISPLAY_ORIENTATION  0
#endif

#if ((DISPLAY_ORIENTATION & GUI_SWAP_XY) != 0)
#define LANDSCAPE   1
#else
#define LANDSCAPE   0
#endif

#if (LANDSCAPE == 1)
#define WIDTH       YSIZE_PHYS  /* Screen Width (in pixels)         */
#define HEIGHT      XSIZE_PHYS  /* Screen Hight (in pixels)         */
#else
#define WIDTH       XSIZE_PHYS  /* Screen Width (in pixels)         */
#define HEIGHT      YSIZE_PHYS  /* Screen Hight (in pixels)         */
#endif

#if ((DISPLAY_ORIENTATION & GUI_SWAP_XY) != 0)
  #if ((DISPLAY_ORIENTATION & GUI_MIRROR_X) != 0)
    #define TOUCH_TOP    TOUCH_X_MAX
    #define TOUCH_BOTTOM TOUCH_X_MIN
  #else
    #define TOUCH_TOP    TOUCH_X_MIN
    #define TOUCH_BOTTOM TOUCH_X_MAX
  #endif
  #if ((DISPLAY_ORIENTATION & GUI_MIRROR_Y) != 0)
    #define TOUCH_LEFT   TOUCH_Y_MAX
    #define TOUCH_RIGHT  TOUCH_Y_MIN
  #else
    #define TOUCH_LEFT   TOUCH_Y_MIN
    #define TOUCH_RIGHT  TOUCH_Y_MAX
  #endif
#else
  #if ((DISPLAY_ORIENTATION & GUI_MIRROR_X) != 0)
    #define TOUCH_LEFT   TOUCH_X_MAX
    #define TOUCH_RIGHT  TOUCH_X_MIN
  #else
    #define TOUCH_LEFT   TOUCH_X_MIN
    #define TOUCH_RIGHT  TOUCH_X_MAX
  #endif
  #if ((DISPLAY_ORIENTATION & GUI_MIRROR_Y) != 0)
    #define TOUCH_TOP    TOUCH_Y_MAX
    #define TOUCH_BOTTOM TOUCH_Y_MIN
  #else
    #define TOUCH_TOP    TOUCH_Y_MIN
    #define TOUCH_BOTTOM TOUCH_Y_MAX
  #endif
#endif

/*********************************************************************
*
*       Driver Port functions
*
**********************************************************************
*/
extern void LCD_X_Init(void);
extern void LCD_X_Write00_16(U16 c);
extern void LCD_X_Write01_16(U16 c);
extern U16  LCD_X_Read01_16 (void);
extern void LCD_X_WriteM01_16(U16 * pData, int NumWords);
extern void LCD_X_ReadM01_16 (U16 * pData, int NumWords);

/*********************************************************************
*
*       Private code
*
**********************************************************************
*/
static void delay (int cnt) {
  cnt <<= 15;
  while (cnt--);
}

static void wr_reg (U16 reg, U16 dat) {
  LCD_X_Write00_16(reg);
  LCD_X_Write01_16(dat);
}

static U16 rd_reg (U16 reg) {
  LCD_X_Write00_16(reg);
  return LCD_X_Read01_16();
}

/*********************************************************************
*
*       _InitController
*
* Purpose:
*   Initializes the display controller
*/
static void _InitController(void) {
#ifndef WIN32
  U16 driverCode;

  LCD_X_Init();

  driverCode = rd_reg(0x00);

  if (driverCode == 0x47) {   /* LCD with HX8347-D LCD Controller   */

    /* Driving ability settings ------------------------------------*/
    wr_reg(0xEA, 0x00);       /* Power control internal used (1)    */
    wr_reg(0xEB, 0x20);       /* Power control internal used (2)    */
    wr_reg(0xEC, 0x0C);       /* Source control internal used (1)   */
    wr_reg(0xED, 0xC7);       /* Source control internal used (2)   */
    wr_reg(0xE8, 0x38);       /* Source output period Normal mode   */
    wr_reg(0xE9, 0x10);       /* Source output period Idle mode     */
    wr_reg(0xF1, 0x01);       /* RGB 18-bit interface ;0x0110       */
    wr_reg(0xF2, 0x10);       

    /* Adjust the Gamma Curve --------------------------------------*/
    wr_reg(0x40, 0x01);
    wr_reg(0x41, 0x00);
    wr_reg(0x42, 0x00);
    wr_reg(0x43, 0x10);
    wr_reg(0x44, 0x0E);
    wr_reg(0x45, 0x24);
    wr_reg(0x46, 0x04);
    wr_reg(0x47, 0x50);
    wr_reg(0x48, 0x02);
    wr_reg(0x49, 0x13);
    wr_reg(0x4A, 0x19);
    wr_reg(0x4B, 0x19);
    wr_reg(0x4C, 0x16);

    wr_reg(0x50, 0x1B);
    wr_reg(0x51, 0x31);
    wr_reg(0x52, 0x2F);
    wr_reg(0x53, 0x3F);
    wr_reg(0x54, 0x3F);
    wr_reg(0x55, 0x3E);
    wr_reg(0x56, 0x2F);
    wr_reg(0x57, 0x7B);
    wr_reg(0x58, 0x09);
    wr_reg(0x59, 0x06);
    wr_reg(0x5A, 0x06);
    wr_reg(0x5B, 0x0C);
    wr_reg(0x5C, 0x1D);
    wr_reg(0x5D, 0xCC);

    /* Power voltage setting ---------------------------------------*/
    wr_reg(0x1B, 0x1B);
    wr_reg(0x1A, 0x01);
    wr_reg(0x24, 0x2F);
    wr_reg(0x25, 0x57);
    wr_reg(0x23, 0x88);

    /* Power on setting --------------------------------------------*/
    wr_reg(0x18, 0x36);       /* Internal oscillator frequency adj  */
    wr_reg(0x19, 0x01);       /* Enable internal oscillator         */
    wr_reg(0x01, 0x00);       /* Normal mode, no scrool             */
    wr_reg(0x1F, 0x88);       /* Power control 6 - DDVDH Off        */
    delay(200);
    wr_reg(0x1F, 0x82);       /* Power control 6 - Step-up: 3 x VCI */
    delay(50);                 
    wr_reg(0x1F, 0x92);       /* Power control 6 - Step-up: On      */
    delay(50);
    wr_reg(0x1F, 0xD2);       /* Power control 6 - VCOML active     */
    delay(50);

    /* Color selection ---------------------------------------------*/
    wr_reg(0x17, 0x55);       /* RGB, System interface: 16 Bit/Pixel*/
    wr_reg(0x00, 0x00);       /* Scrolling off, no standby          */

    /* Interface config --------------------------------------------*/
    wr_reg(0x2F, 0x11);       /* LCD Drive: 1-line inversion        */
    wr_reg(0x31, 0x00);
    wr_reg(0x32, 0x00);       /* DPL=0, HSPL=0, VSPL=0, EPL=0       */

    /* Display on setting ------------------------------------------*/
    wr_reg(0x28, 0x38);       /* PT(0,0) active, VGL/VGL            */
    delay(200);
    wr_reg(0x28, 0x3C);       /* Display active, VGL/VGL            */

//  wr_reg(0x16, 0x00);       /* Mem Access Control (MX/Y/V/L,BGR)  */

    /* Display scrolling settings ----------------------------------*/
    wr_reg(0x0E, 0x00);       /* TFA MSB                            */
    wr_reg(0x0F, 0x00);       /* TFA LSB                            */
    wr_reg(0x10, 320 >> 8);   /* VSA MSB                            */
    wr_reg(0x11, 320 & 0xFF); /* VSA LSB                            */
    wr_reg(0x12, 0x00);       /* BFA MSB                            */
    wr_reg(0x13, 0x00);       /* BFA LSB                            */

  } else {

    /* Start Initial Sequence --------------------------------------*/
    wr_reg(0x01, 0x0100);     /* Set SS bit                         */
    wr_reg(0x02, 0x0700);     /* Set 1 line inversion               */
    wr_reg(0x04, 0x0000);     /* Resize register                    */
    wr_reg(0x08, 0x0207);     /* 2 lines front, 7 back porch        */
    wr_reg(0x09, 0x0000);     /* Set non-disp area refresh cyc ISC  */
    wr_reg(0x0A, 0x0000);     /* FMARK function                     */
    wr_reg(0x0C, 0x0000);     /* RGB interface setting              */
    wr_reg(0x0D, 0x0000);     /* Frame marker Position              */
    wr_reg(0x0F, 0x0000);     /* RGB interface polarity             */

    /* Power On sequence -------------------------------------------*/
    wr_reg(0x10, 0x0000);     /* Reset Power Control 1              */
    wr_reg(0x11, 0x0000);     /* Reset Power Control 2              */
    wr_reg(0x12, 0x0000);     /* Reset Power Control 3              */
    wr_reg(0x13, 0x0000);     /* Reset Power Control 4              */
    delay(200);               /* Discharge cap power voltage (200ms)*/
    wr_reg(0x10, 0x12B0);     /* SAP, BT[3:0], AP, DSTB, SLP, STB   */
    wr_reg(0x11, 0x0007);     /* DC1[2:0], DC0[2:0], VC[2:0]        */
    delay(50);                /* Delay 50 ms                        */
    wr_reg(0x12, 0x01BD);     /* VREG1OUT voltage                   */
    delay(50);                /* Delay 50 ms                        */
    wr_reg(0x13, 0x1400);     /* VDV[4:0] for VCOM amplitude        */
    wr_reg(0x29, 0x000E);     /* VCM[4:0] for VCOMH                 */
    delay(50);                /* Delay 50 ms                        */
    wr_reg(0x20, 0x0000);     /* GRAM horizontal Address            */
    wr_reg(0x21, 0x0000);     /* GRAM Vertical Address              */

    /* Adjust the Gamma Curve --------------------------------------*/
    switch (driverCode) {
      case 0x5408:            /* LCD with SPFD5408 LCD Controller   */
        wr_reg(0x30, 0x0B0D);
        wr_reg(0x31, 0x1923);
        wr_reg(0x32, 0x1C26);
        wr_reg(0x33, 0x261C);
        wr_reg(0x34, 0x2419);
        wr_reg(0x35, 0x0D0B);
        wr_reg(0x36, 0x1006);
        wr_reg(0x37, 0x0610);
        wr_reg(0x38, 0x0706);
        wr_reg(0x39, 0x0304);
        wr_reg(0x3A, 0x0E05);
        wr_reg(0x3B, 0x0E01);
        wr_reg(0x3C, 0x010E);
        wr_reg(0x3D, 0x050E);
        wr_reg(0x3E, 0x0403);
        wr_reg(0x3F, 0x0607);
        break;
      case 0x9325:            /* LCD with RM68050 LCD Controller    */
        wr_reg(0x30, 0x0000);
        wr_reg(0x31, 0x0607);
        wr_reg(0x32, 0x0305);
        wr_reg(0x35, 0x0000);
        wr_reg(0x36, 0x1604);
        wr_reg(0x37, 0x0204);
        wr_reg(0x38, 0x0001);
        wr_reg(0x39, 0x0707);
        wr_reg(0x3C, 0x0000);
        wr_reg(0x3D, 0x000F);
        break;
      case 0x9320:            /* LCD with ILI9320 LCD Controller    */
      default:                /* LCD with other LCD Controller      */
        wr_reg(0x30, 0x0006);
        wr_reg(0x31, 0x0101);
        wr_reg(0x32, 0x0003);
        wr_reg(0x35, 0x0106);
        wr_reg(0x36, 0x0B02);
        wr_reg(0x37, 0x0302);
        wr_reg(0x38, 0x0707);
        wr_reg(0x39, 0x0007);
        wr_reg(0x3C, 0x0600);
        wr_reg(0x3D, 0x020B);
        break;
    }

    /* Set GRAM area -----------------------------------------------*/
    wr_reg(0x50, 0x0000);     /* Horizontal GRAM Start Address      */
    wr_reg(0x51, (HEIGHT-1)); /* Horizontal GRAM End   Address      */
    wr_reg(0x52, 0x0000);     /* Vertical   GRAM Start Address      */
    wr_reg(0x53, (WIDTH-1));  /* Vertical   GRAM End   Address      */

    /* Set Gate Scan Line ------------------------------------------*/
    switch (driverCode) {
      case 0x5408:            /* LCD with SPFD5408 LCD Controller   */
      case 0x9325:            /* LCD with RM68050 LCD Controller    */
        wr_reg(0x60, 0xA700);
        break;
      case 0x9320:            /* LCD with ILI9320 LCD Controller    */
      default:                /* LCD with other LCD Controller      */
        wr_reg(0x60, 0x2700);
        break;
    }
    wr_reg(0x61, 0x0001);     /* NDL,VLE, REV                       */
    wr_reg(0x6A, 0x0000);     /* Set scrolling line                 */

    /* Partial Display Control -------------------------------------*/
    wr_reg(0x80, 0x0000);
    wr_reg(0x81, 0x0000);
    wr_reg(0x82, 0x0000);
    wr_reg(0x83, 0x0000);
    wr_reg(0x84, 0x0000);
    wr_reg(0x85, 0x0000);

    /* Panel Control -----------------------------------------------*/
    wr_reg(0x90, 0x0010);
    wr_reg(0x92, 0x0000);
    wr_reg(0x93, 0x0003);
    wr_reg(0x95, 0x0110);
    wr_reg(0x97, 0x0000);
    wr_reg(0x98, 0x0000);

//  wr_reg(0x03, 0x0000);   /* Entry Mode (AM,ID0/1,ORG,BGR)        */

    wr_reg(0x07, 0x0137);   /* 262K color and display ON            */

  }

#endif  /* WIN32 */
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*   
*/
void LCD_X_Config(void) {
  GUI_DEVICE * pDevice;
  GUI_PORT_API PortAPI = {0}; 
  CONFIG_FLEXCOLOR Config = {0};
  
  #if (NUM_BUFFERS > 1)
    GUI_MULTIBUF_Config(NUM_BUFFERS);
  #endif
  //
  // Set display driver and color conversion for 1st layer
  //
  pDevice = GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
  //
  // Common display driver configuration
  //
  if (LCD_GetSwapXY()) {
    LCD_SetSizeEx (0, YSIZE_PHYS, XSIZE_PHYS);
    LCD_SetVSizeEx(0, YSIZE_PHYS * NUM_VSCREENS, XSIZE_PHYS);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS, YSIZE_PHYS);
    LCD_SetVSizeEx(0, XSIZE_PHYS, YSIZE_PHYS * NUM_VSCREENS);
  }
  //
  // Configuration of GUIDRV_FLEX_COLOR
  //
  #ifdef LCD_LG  /* LCD 240320LG with Himax HX8347 */
    //
    // Select hardware interface
    //
    GUIDRV_FlexColor_SetReadFunc66712_B16(pDevice, GUIDRV_FLEXCOLOR_READ_FUNC_II);
    //
    // Set controller and operation mode
    //
    PortAPI.pfWrite16_A0  = LCD_X_Write00_16;
    PortAPI.pfWrite16_A1  = LCD_X_Write01_16;
    PortAPI.pfWriteM16_A1 = LCD_X_WriteM01_16;
    PortAPI.pfReadM16_A1  = LCD_X_ReadM01_16;
    GUIDRV_FlexColor_SetFunc(pDevice, &PortAPI, GUIDRV_FLEXCOLOR_F66712, GUIDRV_FLEXCOLOR_M16C0B16);
    //
    // Orientation and offset of SEG/COM lines
    //
    Config.Orientation   = DISPLAY_ORIENTATION;
    Config.RegEntryMode  = 0;
    GUIDRV_FlexColor_Config(pDevice, &Config);
  #else
    //
    // Set controller and operation mode
    //
    PortAPI.pfWrite16_A0  = LCD_X_Write00_16;
    PortAPI.pfWrite16_A1  = LCD_X_Write01_16;
    PortAPI.pfWriteM16_A1 = LCD_X_WriteM01_16;
    PortAPI.pfReadM16_A1  = LCD_X_ReadM01_16;
    GUIDRV_FlexColor_SetFunc(pDevice, &PortAPI, GUIDRV_FLEXCOLOR_F66708, GUIDRV_FLEXCOLOR_M16C0B16);
    //
    // Orientation and offset of SEG/COM lines
    //
    Config.Orientation   = DISPLAY_ORIENTATION;
    Config.RegEntryMode  = 0;
    GUIDRV_FlexColor_Config(pDevice, &Config);
  #endif
  #if (USE_TOUCH == 1)
    //
    // Set orientation of touch screen
    //
    GUI_TOUCH_SetOrientation(DISPLAY_ORIENTATION);
    //
    // Calibrate touch screen
    //
    GUI_TOUCH_Calibrate(GUI_COORD_X, 0, WIDTH  - 1, TOUCH_LEFT, TOUCH_RIGHT);
    GUI_TOUCH_Calibrate(GUI_COORD_Y, 0, HEIGHT - 1, TOUCH_TOP,  TOUCH_BOTTOM);
  #endif
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if 
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*
* Return Value:
*   < -1 - Error
*     -1 - Command not handled
*      0 - OK
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r;

  switch (Cmd) {
  //
  // Required
  //
  case LCD_X_INITCONTROLLER: {
    //
    // Called during the initialization process in order to set up the
    // display controller and put it into operation. If the display
    // controller is not initialized by any external routine this needs
    // to be adapted by the customer...
    //
    _InitController();
    return 0;
  }
  default:
    r = -1;
  }
  return r;
}

/*************************** End of file ****************************/
