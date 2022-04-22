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