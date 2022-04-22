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
#include <fcntl.h>

#define _GNU_SOURCE          /* See feature_test_macros(7) */


/* 
    void FD_CLR(int fd , fd_set *set);
    void FD_ISSET(int fd , fd_set *set);
    void FD_SET(int fd , fd_set *set);
    void FD_ZERO(fd_set *set);
 */

#define KEYON		1
#define KEYOFF		0

int fd;

void signal_handler (int signal)
{

    int keyvalue;
    int ret = 0 ;
    printf("signal:%d\r\n" , signal);

     ret = read(fd, &keyvalue, sizeof(keyvalue));
    if(ret < 0){
        printf("read error\n");
    }
    if(keyvalue == KEYON){
        printf("key press keyvalue=%d\r\n", keyvalue);
    }
    return;
}

int main(int agrc, char *argv[])
{
    if(agrc < 2)
    {
        printf("usage: %s #device file\n", argv[0]);
        exit(1);
    }

    int flag;
    fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if(fd < 0){
        printf("open file error\n");
        exit(1);
    }else{
        printf("file open ok\n");
    }

    fcntl(fd , F_SETOWN , getpid());
    flag = fcntl(fd , F_GETFL);
    if( fcntl(fd , F_SETFL , flag | FASYNC) < 0 ){
        printf("fcntl error\r\n");
    }

    if( signal (SIGIO , signal_handler) == SIG_ERR){
        printf("signal login failed!\r\n");
        exit(-1);
    }
    
    while (1)
    {
        // printf("main is running\r\n");
        // sleep(4);
    }

    close(fd);

}
