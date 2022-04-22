#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc , char *argv[])
{
    if(argc < 2 ){
        printf("[usag:] filename w/r string/num\r\n");
        return(-1);
    }

    int fd;

     char rbuf[100] = {0};
    int r_count = 0;
     char usrdata[] = "user data";

    if((fd = open(argv[1], O_RDWR)) < 0)
        printf("open failed\r\n");
    ssize_t ret;

        if( strcmp(argv[2] ,"read") == 0 )
        {
            r_count = atoi(argv[3]);
            if(read(fd, rbuf, r_count) < 0)
                printf("read failed\r\n");
            else
            {
                printf("APP read kernel ok:%s\r\n", rbuf);
            }
        }
        else if( strcmp(argv[2] , "write") == 0 )
        {
            //memcpy(wbuf, usrdata, sizeof(usrdata));
            
            if(write(fd ,usrdata, sizeof(usrdata)) < 0)
                printf("write failed\r\n");
            else printf("write kernel seccssed\r\n");
            printf("size:%d\r\n", sizeof(usrdata));
        }

    close(fd);

    return 0;
}