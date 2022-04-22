#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>


#define OPEN            _IO('r', 0x00000001)
#define CLOSE           _IO('r', 0x00000002)
#define CHANGE_TIMES    _IOWR('r', 0x00000003, int)

int value;
int arg;
int fd;


int main(int agrc, char *argv[])
{
    if(agrc < 2)
    {
        printf("usage: %s #device file\r\n", argv[0]);
        exit(1);
    }
	
    int arg = 50;
    pthread_t kth;
    fd = open(argv[1], O_RDWR);
    if(fd < 0){
        printf("open file error\r\n");
        exit(1);
    }else{
        printf("%s open ok\r\n" , argv[1]);
    }

    while (1)
    {
        printf("entry cmd:");
        scanf("%d", &value);
        getchar();
        switch (value)
        {
            case 1:ioctl(fd,OPEN, arg );
                break;
            
            case 2:ioctl(fd,CLOSE, arg);
                break;

            case 3:printf("entry times:");
                    scanf("%d", &arg);
                    getchar();
                    ioctl(fd,CHANGE_TIMES, arg);
                break;
            
            default:
                break;
        }
    }

    close(fd);

    return 0;

}
