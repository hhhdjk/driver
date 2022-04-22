#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int agrc, char *argv[])
{
    if(agrc < 2)
    {
        printf("usage: %s #device file\n", argv[0]);
        exit(1);
    }

    unsigned char keyvalue;
    int ret = 0;
    int fd = open(argv[1], O_RDWR);
    if(ret < 0)
    {
        printf("open file error\n");
        exit(1);
    }else{
        printf("file open ok\n");
    }

    while (1)
    {
        ret = read(fd, &keyvalue, sizeof(keyvalue));
        if(ret < 0)
        {
            printf("read error\n");
        }
        if(keyvalue == 0x0f)
        {
            printf("key press keyvalue=%d\n", keyvalue);
        }
        if(keyvalue == 0xf0)printf("key unpress keyvalue=%d\n", keyvalue);
    }
    

    close(fd);

}
