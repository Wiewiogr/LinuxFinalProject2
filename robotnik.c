#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"

void timerHandler(int sig, siginfo_t *si, void *uc)
{
    printf("dostalem sygnal!!!\n");
    exit(1);
}

int main(int argc, char* argv[])
{
    int opt;
    char socketName[25];

    while ((opt = getopt(argc, argv, "s:")) != -1)
    {
        switch (opt)
        {
        case 's':
            strcpy(socketName,optarg);
            break;
        }
    }
    registerHandler(SIGALRM,timerHandler);
    printf("my pid : %d, my group : %d my socket : %s\n",getpid(),  getpgrp(), socketName);
    char buffer[25];
    read(0,buffer,sizeof(buffer));
    printf("%d - from pipe : %s\n", getpid(), buffer);
    while(1)
    {
        int number;
        read(0,&number,sizeof(number));
        printf("%d - from pipe : %d\n", getpid(), number);
        sleep(1);
    }
        pause();
    return 0;
}
