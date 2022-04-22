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


/* 
    void FD_CLR(int fd , fd_set *set);
    void FD_ISSET(int fd , fd_set *set);
    void FD_SET(int fd , fd_set *set);
    void FD_ZERO(fd_set *set);
 */

#define KEYON		1
#define KEYOFF		0

int main(int agrc, char *argv[])
{
    if(agrc < 2)
    {
        printf("usage: %s #device file\n", argv[0]);
        exit(1);
    }

    int keyvalue;
    int ret = 0 ;
    int err=0;
    int fd;
    fd_set readfd;


    struct pollfd fds[1];
   

    fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if(ret < 0){
        printf("open file error\n");
        exit(1);
    }else{
        printf("file open ok\n");
    }


     fds[0].fd = fd;
     fds[0].events = POLLIN;

#if 0
    while (1)
    {
        FD_ZERO(&readfd);
        FD_SET(fd , &readfd);

        err = select(fd + 1 ,&readfd , NULL , NULL , NULL);
        printf("err:%d\r\n", err);
        if(err == 0){
            //printf("timeout\r\n");
        }else if(err == -1) {
             printf("error \r\n");
        }else if(FD_ISSET(fd , &readfd)) {
            ret = read(fd, &keyvalue, sizeof(keyvalue));
            if(ret < 0){
                printf("read error\n");
            }
            if(keyvalue == KEYON){
                printf("key press keyvalue=%d , err:%d\r\n", keyvalue , err);
            }
        }
        
    }
#endif

    while (1)
    {
        err = poll(fds, 1 , 2000);
        
        printf("err:%d\r\n", err);
        if(err == 0){
            printf("timeout\r\n");
        }else if(err == -1) {
             printf("error \r\n");
        }else if( fds[0].revents | POLLIN ) {
            ret = read(fd, &keyvalue, sizeof(keyvalue));
            if(ret < 0){
                printf("read error\n");
            }
            if(keyvalue == KEYON){
                printf("key press keyvalue=%d , err:%d\r\n", keyvalue , err);
            }
        }
        
    }

    close(fd);

}
