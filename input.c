#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc , char *argv[])
{
    if(argc < 2){
        printf("Usage: %s <dev>\r\n" , argv[0]);
        exit(-1);
    }

    char *ev_name[] = {"EV_SYN",
                        "EV_KEY",
                        "EV_REL",
                        "EV_ABS",
                        "EV_MSC",
                        "EV_SW",};
    int fd;
    int err;
    struct input_id id;
    struct input_event event;
    unsigned int evbit;
    int i;

    if(!strcmp(argv[2] , "nonblock")){
        fd = open(argv[1] , O_RDWR | O_NONBLOCK);
    }else {
        fd = open(argv[1] , O_RDWR);
    }

    if(fd < 0){
        printf("%s open error\r\n" , argv[1]);
    }

    err = ioctl(fd , EVIOCGID , &id);
    if(err == 0){
        printf("bustype 0x%x\n" ,id.bustype);
        printf(" vendor 0x%x\n" ,id.vendor);
        printf("product 0x%x\n" ,id.product);
        printf("version 0x%x\n" ,id.version);
    }

    err = ioctl(fd , EVIOCGBIT(0,sizeof(evbit)) , &evbit);
    printf("%d\n" , err);

    printf("support ev type: ");
    for(i=0 ; i<6  ; i++){
        if((evbit >> i) & 0x01){
            printf("%s  " , ev_name[i]);
        }
    }
    printf("\n");

    while(1){
        err = read(fd , &event , sizeof(event));
        if(err < 0){
            printf("read err %d\n" , err);
        }

        if(err == sizeof(event)){
            printf("get event :type=0x%x , code=0x%x , value=0x%x\n" , event.type ,  event.code, event.value);
        }
    }


    close(fd);

    return 0;
}

static unsigned char ssd_1306_init_sequence[] = {	
	0x00,0xAE,//--turn off oled panel
	0x00,0x00,//---set low column address
	0x00,0x10,//---set high column address
	0x00,0x40,//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	0x00,0x81,//--set contrast control register
	0x00,0xCF,// Set SEG Output Current Brightness
	0x00,0xA1,//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	0x00,0xC8,//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	0x00,0xA6,//--set normal display
	0x00,0xA8,//--set multiplex ratio(1 to 64)
	0x00,0x3f,//--1/64 duty
	0x00,0xD3,//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	0x00,0x00,//-not offset
	0x00,0xd5,//--set display clock divide ratio/oscillator frequency
	0x00,0x80,//--set divide ratio, Set Clock as 100 Frames/Sec
	0x00,0xD9,//--set pre-charge period
	0x00,0xF1,//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	0x00,0xDA,//--set com pins hardware configuration
	0x00,0x12,
	0x00,0xDB,//--set vcomh
	0x00,0x40,//Set VCOM Deselect Level
	0x00,0x20,//-Set Page Addressing Mode (0x00/0x01/0x02)
	0x00,0x02,//
	0x00,0x8D,//--set Charge Pump enable/disable
	0x00,0x14,//--set(0x10) disable
	0x00,0xA4,// Disable Entire Display On (0xa4/0xa5)
	0x00,0xA6,// Disable Inverse Display On (0xa6/a7) 
	0x00,0xAF
};
