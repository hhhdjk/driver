#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <poll.h>

#define _GNU_SOURCE          /* See feature_test_macros(7) */


int main(int agrc, char *argv[])
{

    int tempvalue;
    int ret = 0 ;
    int err=0;
    int fd;
    // struct pollfd fds[1];
    // fds[0].fd = fd;
    // fds[0].events = POLLIN;

    fd = open("/dev/Hc_Sr04.0", O_RDWR);
    if(ret < 0){
        printf("open /dev/Hc_Sr04.0 error\n");
        exit(1);
    }else   printf("/dev/Hc_Sr04.0 open ok\n");

    while (1)
    {
        // err = poll(fds, 1 , 2000);
        // printf("err:%d\r\n", err);
        // if(err == 0){
        //     printf("timeout\r\n");
        // }else if(err == -1) {
        //      printf("error \r\n");
        // }else if( fds[0].revents | POLLIN ) {
        //     ret = read(fd, &tempvalue, sizeof(tempvalue));
        //     if(ret < 0){
        //         printf("read error\n");
        //     }
        //     printf("Temp =[ %d ] , err:[%d]\r\n", tempvalue , err);, (((float)tempvalue)/100)*340/20
        // }
        
        ret = read(fd, &tempvalue, sizeof(4));
        if(ret < 0){
                printf("read error\n");
        }
        printf("Distance =[ (%d)   %fcm ]\r\n", tempvalue , ((float)tempvalue)/1000*340/2/10);
        usleep(500000);
    }

    close(fd);

}
