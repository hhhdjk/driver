#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

/* 
    void FD_CLR(int fd , fd_set *set);
    void FD_ISSET(int fd , fd_set *set);
    void FD_SET(int fd , fd_set *set);
    void FD_ZERO(fd_set *set);
 */

int fd;
struct input_event event;

void sighandler_t(int signal)
{
    int err;
    while((err = read(fd , &event , sizeof(event))) == sizeof(event)){

        if(err == sizeof(event)){
            printf("get event :type=0x%x , code=0x%x , value=0x%x\n" , event.type ,  event.code, event.value);
        }
    }

}


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
    int err;
    struct input_id id;
    unsigned int evbit;
    int i;
    int flag;

    if(0 == strcmp(argv[2] , "nonblock")){
        fd = open(argv[1] , O_RDWR | O_NONBLOCK);
    }else {
        fd = open(argv[1] , O_RDWR);
    }

    if(fd < 0){
        printf("%s open error\r\n" , argv[1]);
    }

    /* 应用进程id通知给驱动 */
    fcntl(fd, F_SETOWN, getpid());

    /* 设置文件属性 */
    flag = fcntl(fd , F_GETFL);
    if(fcntl(fd , F_SETFL , flag | FASYNC) < 0){
        printf("fcntl F_STEFL err\n");
        exit(-1);
    }

    /* 注册信号处理 */    
    if(signal(SIGIO, sighandler_t) == SIG_ERR){
        printf("register signal err\n");
        exit(-1);
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
        printf("main is running\n");
        sleep(1);
    }

    close(fd);

    return 0;
}
