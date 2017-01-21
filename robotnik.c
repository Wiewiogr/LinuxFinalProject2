#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

int sockfd;

void timerHandler(int sig, siginfo_t *si, void *uc)
{
    char byte;
    if(read(0,&byte,1) == -1)
        if(errno == EAGAIN)
            exit(0);
    struct timespec currentTime;
    clock_gettime(CLOCK_REALTIME,&currentTime);
    char message[50];
    sprintf(message,"%c%ld.%ld",byte, currentTime.tv_sec,currentTime.tv_nsec);
    printf("dostalem sygnal!!!\n");
    write(sockfd,&message,sizeof(message));
    perror("write");
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

    struct sockaddr_un serv_addr = createAbstractSockaddr(socketName);

    if(sockfd = socket(AF_UNIX, SOCK_DGRAM, 0))
        perror("socket");
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(struct sockaddr_un)) < 0)
        perror("connect");

    while(1)
        pause();
    return 0;
}
