#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc , char *argv[])
{

    int fd;
    static char rbuf[100],wbuf[100];
    static char usrdata[] = "user data!";
    char *filename = argv[1];
    if((fd = open(filename, O_RDWR)) < 0)
        printf("open failed\r\n");
    ssize_t ret;

    if(atoi(argv[2]) == 1)
        {
            ret = read(fd, rbuf, 20);
            if(ret < 0)
                printf("read failed\r\n");
            else
            {
                printf("APP read%s\r\n", rbuf);
                printf("read seccssed\r\n");    
            }
        }
        else if(atoi(argv[2]) == 2)
        {
            memcpy(wbuf, usrdata, sizeof(usrdata));
            ret = write(fd ,wbuf, sizeof(wbuf));
            if(ret < 0)
                printf("write failed\r\n");
            else printf("write seccssed\r\n");
        }

    close(fd);

    return 0;
}