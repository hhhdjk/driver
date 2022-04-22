#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <signal.h>
#include <poll.h>



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
    struct pollfd fds;
    unsigned int evbit;
    int i;


    if(0 == strcmp(argv[2] , "nonblock")){
        fd = open(argv[1] , O_RDWR | O_NONBLOCK);
    }else {
        fd = open(argv[1] , O_RDWR);
    }

    if(fd < 0){
        printf("%s open error\r\n" , argv[1]);
    }

    fds.fd = fd;
    fds.events = POLLIN;

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
        fds.revents = 0;
        err = poll(&fds, 1, 5000);
        if(err > 0){
            if(fds.revents | POLLIN){
                while ((err=read(fd , &event , sizeof(event))) == sizeof(event))
                {
                    if(err ){
                    printf("get event :type=0x%x , code=0x%x , value=0x%x\n" , 
                                    event.type ,  event.code, event.value);
                    }
                }

               
            }

        }else if(err == 0){
            printf("timeout\n");
        }else {
            printf("poll err\n");            
        }

    }


    close(fd);

    return 0;
}
