#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define KEYON		1
#define KEYOFF		0
int value;
int fd;

void * key_fn(void *arg)
{
    printf("key_fn is running\r\n");
    printf("arg:%d  id:%lx\r\n", *((int *)arg), pthread_self());
    while(1)
	{
		read(fd, &value, 4);
        if(value == KEYON){
            printf("key pressed value: [%d]\r\n", value);
        }
	}

    return (void *)0;
}

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

    if( pthread_create(&kth, NULL, key_fn, (void * )&arg) != 0)
        printf("pthread_create error\r\n");

    while (1)
    {
        sleep(1);
        printf("main is running\r\n");
    }
    

    pthread_join(kth, NULL);

    close(fd);

    return 0;

}
