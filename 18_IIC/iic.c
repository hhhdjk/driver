/*
 *
 * Copyright (c) 1999-2002 Vojtech Pavlik
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <dt-bindings/soc/s5p4418-base.h>
#include <linux/i2c.h>

#define     WIDTH       128
#define     HEIGHT        64

#define     SSD1306_WHITE        1
#define     SSD1306_BLACK        0
#define     SSD1306_INVERSE      2

uint8_t rotation;

#define OLED_CMD        0x00	//写命令
#define OLED_DATA       0x40	//写数据

#define     CMD_CLR_PANEL(len)           _IOC(_IOC_NONE, 'O', 0x1, len)
#define     CMD_FULL_PANEL(len)          _IOC(_IOC_NONE, 'O', 0x2, len)
#define     CMD_GET_PANNEL_INFO(len)     _IOC(_IOC_READ, 'O', 0x3, len)
#define     CMD_SET_PANNEL_INFO(len)     _IOC(_IOC_WRITE, 'O', 0x4, len)

static unsigned char buffer[WIDTH*HEIGHT/8];  /* 1K 显存 */
#define ssd1306_swap(a, b)                                                     \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

#define pgm_read_byte(addr)   (*(const unsigned char *)(addr)) 

int 	major;		/* 主设备号 */
int 	minor;		/* 次设备还 */
dev_t 	d_id;		/* 设备id */

struct cdev 	d_cdev;			/* 字符设备 */
struct class 	*d_class;		/* 字符设备类 */
struct device 	*d_device;		/* 字符设备节点 */

#define		D_NAME	"oled_ssd1306"		/* 字符设备节点名 */
#define 	minor_num	1

struct device * oled_devices;
struct device_node *nd; //设备节点
struct property *comppro; //属性值
int gpio_num;
struct timer_list time;
struct i2c_client *oled_client;

const uint8_t splash1_data[] = {
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000011,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000111,0b11000000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000111,0b11000000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001111,0b11000000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00011111,0b11100000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00011111,0b11100000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00111111,0b11100000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00111111,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01111111,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00011111,0b11111000,0b01111111,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00111111,0b11111110,0b01111111,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00111111,0b11111111,0b01111111,0b11110000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00011111,0b11111111,0b11111011,0b11100000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00001111,0b11111111,0b11111001,0b11111111,0b11000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00001111,0b11111111,0b11111001,0b11111111,0b11111000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000111,0b11111111,0b11110001,0b11111111,0b11111111,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000011,0b11111100,0b01110011,0b11111111,0b11111111,0b10000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000001,0b11111110,0b00111111,0b11111111,0b11111111,0b10000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b11111111,0b00011110,0b00001111,0b11111111,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b01111111,0b11111110,0b00011111,0b11111100,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00111111,0b11111111,0b11111111,0b11111000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00001111,0b11011111,0b11111111,0b11100000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00011111,0b00011001,0b11111111,0b11000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00111111,0b00111100,0b11111111,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b01111110,0b01111100,0b11111000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b01111111,0b11111110,0b01111100,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b11111111,0b11111111,0b11111100,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b11111111,0b11111111,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b11111111,0b11111111,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000001,0b11111111,0b11101111,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000001,0b11111111,0b11001111,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000011,0b11111111,0b00000111,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000011,0b11111100,0b00000111,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000011,0b11110000,0b00000011,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000001,0b10000000,0b00000000,0b11111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00111110,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001100,0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000111,0b10000000,0b00000000,0b11111100,0b00000000,0b00000000,0b00000011,0b11000000,0b00000000,
  0b00000000,0b00000000,0b00000111,0b10000000,0b00000001,0b11111100,0b00000000,0b00000000,0b00000011,0b11000000,0b00000000,
  0b00000000,0b00000000,0b00000111,0b10000000,0b00000001,0b11111100,0b00000000,0b00000000,0b00000011,0b11000000,0b00000000,
  0b00000000,0b00000000,0b00000111,0b10000000,0b00000001,0b11100000,0b00000000,0b00000000,0b00000000,0b00011110,0b00000000,
  0b00000000,0b00000000,0b00000111,0b10000000,0b00000001,0b11100000,0b00000000,0b00000000,0b00000000,0b00011110,0b00000000,
  0b01111111,0b11100011,0b11110111,0b10011111,0b11111001,0b11111101,0b11100111,0b01111000,0b01111011,0b11011111,0b11000000,
  0b11111111,0b11110111,0b11111111,0b10111111,0b11111101,0b11111101,0b11111111,0b01111000,0b01111011,0b11011111,0b11000000,
  0b11111111,0b11110111,0b11111111,0b10111111,0b11111101,0b11111101,0b11111111,0b01111000,0b01111011,0b11011111,0b11000000,
  0b11110000,0b11110111,0b10000111,0b10111100,0b00111101,0b11100001,0b11111111,0b01111000,0b01111011,0b11011110,0b00000000,
  0b11110000,0b11110111,0b10000111,0b10111100,0b00111101,0b11100001,0b11110000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b00000000,0b11110111,0b10000111,0b10000000,0b00111101,0b11100001,0b11100000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b01111111,0b11110111,0b10000111,0b10011111,0b11111101,0b11100001,0b11100000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b11111111,0b11110111,0b10000111,0b10111111,0b11111101,0b11100001,0b11100000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b11110000,0b11110111,0b10000111,0b10111100,0b00111101,0b11100001,0b11100000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b11110000,0b11110111,0b10000111,0b10111100,0b00111101,0b11100001,0b11100000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b11110000,0b11110111,0b10000111,0b10111100,0b00111101,0b11100001,0b11100000,0b01111000,0b01111011,0b11011110,0b00000000,
  0b11111111,0b11110111,0b11111111,0b10111111,0b11111101,0b11100001,0b11100000,0b01111111,0b11111011,0b11011111,0b11000000,
  0b11111111,0b11110111,0b11111111,0b10111111,0b11111101,0b11100001,0b11100000,0b01111111,0b11111011,0b11011111,0b11000000,
  0b01111100,0b11110011,0b11110011,0b10011111,0b00111101,0b11100001,0b11100000,0b00111110,0b01111011,0b11001111,0b11000000,
  0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
  0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11000000,
  0b11111111,0b11111111,0b11111111,0b11111111,0b11111101,0b01101000,0b11011011,0b00010001,0b00011010,0b00110001,0b11000000,
  0b11111111,0b11111111,0b11111111,0b11111111,0b11111101,0b00101011,0b01011010,0b11111011,0b01101010,0b11101111,0b11000000,
  0b11111111,0b11111111,0b11111111,0b11111111,0b11111101,0b01001011,0b01011011,0b00111011,0b00011010,0b00110011,0b11000000,
  0b11111111,0b11111111,0b11111111,0b11111111,0b11111101,0b01101011,0b01011011,0b11011011,0b01101010,0b11111101,0b11000000,
};

static unsigned char ssd_1306_init_sequence[] = {	
	0xAE,//--turn off oled panel
	0xd5,//---set low column address
	0x80,//---set high column address
	0xa8,//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    0x3f,
	0xd3,//--set contrast control register
	0x00,// Set SEG Output Current Brightness
	0x40,//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	0x8d,//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	0x14,//--set normal display
	0x20,//--set multiplex ratio(1 to 64)
	0x00,//--1/64 duty
	0xa1,//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	0xc8,//-not offset
	0xda,//--set display clock divide ratio/oscillator frequency
	0x12,//--set divide ratio, Set Clock as 100 Frames/Sec
	0x81,//--set pre-charge period
	0xcf,//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	0xD9,//--set com pins hardware configuration
	0xf1,
	0xD8,//--set vcomh
	0x40,//Set VCOM Deselect Level
	0xa4,//-Set Page Addressing Mode (0x00/0x01/0x02)
	0xa6,//
	0x2e,//--set Charge Pump enable/disable
	0xaf
};


int ssd_1306_write_cmd(u8 cmd)
{
    int ret;
    u8 Send_CMD[2] = {OLED_CMD, cmd};
    struct i2c_msg msgs;
    msgs.addr = oled_client->addr;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = Send_CMD;
    ret = i2c_transfer(oled_client->adapter, &msgs,1);
    if(ret < 0){
            printk("ssd_1306_panel_step i2c_transfer err %d\n", ret);
            return -EINVAL;
    }
    return 0;
}

int ssd_1306_write_dat(u8 dat)
{
    int ret;
    u8 Send_DAT[2] = {OLED_DATA, dat};
    struct i2c_msg msgs;
    msgs.addr = oled_client->addr;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = Send_DAT;
    ret = i2c_transfer(oled_client->adapter, &msgs,1);
    if(ret < 0){
            printk("ssd_1306_panel_step i2c_transfer err %d\n", ret);
            return -EINVAL;
    }
    return 0;
}


int display(void)
{
    int i;
    ssd_1306_write_cmd(0x22);
    ssd_1306_write_cmd(0x00);
    ssd_1306_write_cmd(0xff);
    ssd_1306_write_cmd(0x21);
    ssd_1306_write_cmd(0x00);
    ssd_1306_write_cmd(0x7f);
    for (i=0; i<1024; i++){
        ssd_1306_write_dat(buffer[i]);
    }
    return 0;
}


/* oled panel initialize */
int ssd_1306_panel_step(void)
{
    int i;
    int len = sizeof(ssd_1306_init_sequence);
    for (i=0; i<len; i++){
        ssd_1306_write_cmd(ssd_1306_init_sequence[i]);
    }
    return 0;
}


void drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch (rotation) {
    case 1:
      ssd1306_swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      ssd1306_swap(x, y);
      y = HEIGHT - y - 1;
      break;
    }
    switch (color) {
    case SSD1306_WHITE:
      buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
      break;
    case SSD1306_BLACK:
      buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
      break;
    case SSD1306_INVERSE:
      buffer[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
      break;
    }
  }
}

void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                             uint16_t color) {

  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  int16_t dx, dy;
  int16_t ystep;
  int16_t err;
  if (steep) {
    ssd1306_swap(x0, y0);
    ssd1306_swap(x1, y1);
  }

  if (x0 > x1) {
    ssd1306_swap(x0, x1);
    ssd1306_swap(y0, y1);
  }

  
  dx = x1 - x0;
  dy = abs(y1 - y0);

  err = dx / 2;
  

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color) {
  writeLine(x, y, x, y + h - 1, color);
}

void drawFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color) {
  writeLine(x, y, x + w - 1, y, color);
}

void drawCircle(int16_t x0, int16_t y0, int16_t r,
                              uint16_t color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0, y0 + r, color);
  drawPixel(x0, y0 - r, color);
  drawPixel(x0 + r, y0, color);
  drawPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, uint16_t color) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;
  int16_t j;
  for (j = 0; j < h; j++, y++) {
        int16_t i;
    for (i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
      if (byte & 0x80)
        drawPixel(x + i, y, color);
    }
  }
}


void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            uint16_t color) {
  // Update in subclasses if desired!
  if (x0 == x1) {
    if (y0 > y1)
      ssd1306_swap(y0, y1);
    drawFastVLine(x0, y0, y1 - y0 + 1, color);
  } else if (y0 == y1) {
    if (x0 > x1)
      ssd1306_swap(x0, x1);
    drawFastHLine(x0, y0, x1 - x0 + 1, color);
  } else {
    writeLine(x0, y0, x1, y1, color);
  }
}

void testdrawline(void) {
  int16_t i;

  memset(buffer , 0 , 1024); // Clear display buffer

  for(i=0; i<128; i+=4) {
    drawLine(0, 0, i,63, SSD1306_WHITE);
    display(); // Update screen with each newly-drawn line
    
  }
  for(i=0; i<64; i+=4) {
    drawLine(0, 0, 127, i, SSD1306_WHITE);
    display();
    
  }
  mdelay(250);

  memset(buffer , 0 , 1024);

  for(i=0; i<128; i+=4) {
    drawLine(0,63, i, 0, SSD1306_WHITE);
    display();
    
  }
  for(i=64-1; i>=0; i-=4) {
    drawLine(0,63, 127, i, SSD1306_WHITE);
    display();
    
  }
  mdelay(250);

  memset(buffer , 0 , 1024);

  for(i=127; i>=0; i-=4) {
    drawLine(127,63, i, 0, SSD1306_WHITE);
    display();
    
  }
  for(i=64-1; i>=0; i-=4) {
    drawLine(127,63, 0, i, SSD1306_WHITE);
    display();
    
  }
  mdelay(250);

  memset(buffer , 0 , 1024);

  for(i=0; i<64; i+=4) {
    drawLine(127, 0, 0, i, SSD1306_WHITE);
    display();
    
  }
  for(i=0; i<128; i+=4) {
    drawLine(127, 0, i,63, SSD1306_WHITE);
    display();
    
  }

  mdelay(2000); // Pause for 2 seconds
}

int oled_open (struct inode *node, struct file *file)
{
    
    /* init panel */
    if(ssd_1306_panel_step())
        return -EINVAL;
    rotation = 0;
    memset(buffer , 0 , 1024);
    testdrawline();
    if(display())
        return -EINVAL;
	return 0;
}

ssize_t oled_read (struct file *file, char __user *buf, size_t size, loff_t *ppos)
{


	return 0; 
}


ssize_t oled_write (struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{

	return size;
}


int oled_release (struct inode *node, struct file *file)
{

	return 0;
}

long oled_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    //struct i2c_client *client = oled_client;

    switch (cmd)
    {
        case CMD_GET_PANNEL_INFO(0):rotation = 1;
        break;

        case CMD_SET_PANNEL_INFO(0):           
        break;

        case CMD_CLR_PANEL(0):memset(buffer , 0x00 , 1024); 
                                                if(display())
                                                    return -EINVAL;
            // ssd_1306_write_cmd(0x27);        //水平向左或者右滚动 26/27
            // ssd_1306_write_cmd(0x00);        //虚拟字节
            // ssd_1306_write_cmd(0x04);        //起始页 0
            // ssd_1306_write_cmd(0x00);        //滚动时间间隔
            // ssd_1306_write_cmd(0x04);        //终止页 7
            // ssd_1306_write_cmd(0x00);        //虚拟字节
            // ssd_1306_write_cmd(0xFF);        //虚拟字节
            // ssd_1306_write_cmd(0x2F);
        break;

        case CMD_FULL_PANEL(0):
            // drawPixel(0,0,SSD1306_WHITE);
            // drawFastHLineInternal(40, 35,60,SSD1306_WHITE);
            // drawFastHLine(20, 10,50,SSD1306_WHITE);
            // drawCircle(40, 20, 20, SSD1306_WHITE);
            drawBitmap(21, 0, splash1_data, 82, 64, SSD1306_WHITE);
            if(display())
                return -EINVAL;
        break;
        
        default:
            break;
    }


	return 0;
}

struct file_operations oled_ops = {
	.owner = THIS_MODULE,
	.open = oled_open,
	.read = oled_read,
	.write = oled_write,
	.release = oled_release,
	.unlocked_ioctl = oled_ioctl,
};

static int i2c_oled_probe(struct i2c_client *client, const struct i2c_device_id *id )
{
    int i, j;
    oled_client = client;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "I2C_FUNC_I2C check failed\n");
        return -EBUSY;
    }

    	/* 创建设备 */
	for(i=0 ; i < minor_num ; i++){
		d_device = device_create(d_class , NULL , MKDEV(major , i) , NULL , D_NAME"%d" , i);
		if(IS_ERR(d_device)){
			for(j=0 ; j < i; j++){
				device_destroy(d_class , MKDEV(major , j));
			}
				class_destroy(d_class);
				cdev_del(&d_cdev);
				unregister_chrdev_region(d_id , 1);
		}
	}

    return 0;
}

static int i2c_oled_remove(struct i2c_client *client )
{
	device_destroy(d_class , MKDEV(major , 0));
	class_destroy(d_class);
	cdev_del(&d_cdev);
	unregister_chrdev_region(d_id , 1);
    return 0;
}


static const struct i2c_device_id oled_id[] = {
    { "oled-i2c", 0 },
    { },
};
MODULE_DEVICE_TABLE(i2c, oled_id);

static struct of_device_id oled_of_match[] = {
    {.compatible = "oled,ssd1306",},
    {}
};

static struct i2c_driver i2c_oled = {
    .probe      = i2c_oled_probe,
    .remove     = i2c_oled_remove,
    .driver     = {
                .owner = THIS_MODULE,
                .name = "oled-i2c",
                .of_match_table = oled_of_match,
    },
    .id_table   = oled_id,
};

static int __init oled_drv_init(void)
{
    int err;
    int ret;
	/* 设备号 */
	if(major){
				d_id = MKDEV(major , minor);
				ret = register_chrdev_region(d_id , 1 ,D_NAME);
	}else{
				ret = alloc_chrdev_region(&d_id , 0 ,minor_num, D_NAME);
				major = MAJOR(d_id);
				minor = MINOR(d_id);
	}
	if(ret  < 0){
				printk("register fail!\n");
				return -EINVAL;
	}

	/* 注册字符设备 */
	d_cdev.owner = oled_ops.owner;
	cdev_init(&d_cdev , &oled_ops);
	if( cdev_add(&d_cdev ,d_id ,  1) ){
		printk("cdev_add err\r\n");
		unregister_chrdev_region(d_id , 1);
		return -EINVAL;
	}

	/* 创建设备类 */
	d_class = class_create(THIS_MODULE , "OLED");
	if(IS_ERR(d_class)){
		printk("class_create err\r\n");
		cdev_del(&d_cdev);
		unregister_chrdev_region(d_id , 1);
		return PTR_ERR(d_class);
	}

    err = i2c_add_driver(&i2c_oled);
    if(err){
        printk("i2c_add_driver err");

    }

	return 0;
}

static void __exit oled_drv_exit(void)
{
    i2c_del_driver(&i2c_oled);
    
	return;
}

module_init(oled_drv_init);
module_exit(oled_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");	 				

