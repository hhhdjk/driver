#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define     CMD_CLR_PANEL(len)           _IOC(_IOC_NONE, 'O', 0x1, len)
#define     CMD_FULL_PANEL(len)          _IOC(_IOC_NONE, 'O', 0x2, len)
#define     CMD_GET_PANNEL_INFO(len)     _IOC(_IOC_READ, 'O', 0x3, len)
#define     CMD_SET_PANNEL_INFO(len)     _IOC(_IOC_WRITE, 'O', 0x4, len)



int fd;

int main(int agrc, char *argv[])
{
    if(agrc < 2)
    {
        printf("usage: %s #device file\r\n", argv[0]);
        exit(1);
    }
    int cmd;
    unsigned long arg;

    fd = open("/dev/oled_ssd13060", O_RDWR);
    if(fd < 0){
        printf("open file error\r\n");
        exit(1);
    }else{
        printf("%s open ok\r\n" , argv[1]);
    }

    while (1)
    {
        printf("entry cmd:");
        scanf("%d", &cmd);
        getchar();
        switch (cmd)
        {
            case 1:ioctl(fd,CMD_GET_PANNEL_INFO(0));
                break;
            
            case 2:ioctl(fd,CMD_CLR_PANEL(0));
                break;

            case 3: printf("entry times:");
                    scanf("%d", &arg);
                    getchar();
                    ioctl(fd,CMD_SET_PANNEL_INFO(0), arg);
            break;
            
            case 4:ioctl(fd,CMD_FULL_PANEL(0));
                break;
            
            default:printf("not suppost cmd\n");
                break;
        }
    }

    close(fd);

    return 0;

}
