#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "common.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h> 

int workerGroupPid;
char brigadeId[] = "aa\0";
int numberOfWorkers = 5;

void onExit()
{
    killpg(workerGroupPid, SIGKILL);
}

int main()
{
    printf("brygadzista gid : %d\n", getpgrp());
    //if((workerGroupPid = fork()) == 0)
    //{
    //    setpgid(0,0);
    //    for(int i = 0; i < 5; i++)
    //    {
    //        char * newArgs[] =
    //        {
    //            (char *) 0
    //        };

    //        if(fork() == 0)
    //        {
    //            execvp("./robotnik.out",newArgs);
    //            exit(1);
    //        }
    //    }
    //    exit(1);
    //}
    //printf("workerGroupPid : %d", workerGroupPid);
    //atexit(onExit);
    //sleep(1);
    //killpg(workerGroupPid, SIGALRM);

    int sockfd, portno, n;
    struct sockaddr_un serv_addr = {AF_UNIX, "registrationChannel\0"};

    char buffer[256] = {0};
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR connecting");
    write(sockfd,brigadeId,strlen(brigadeId));
    read(sockfd,buffer,255);
        perror("read");
    close(sockfd);
    printf("read : %s\n", buffer);
    sleep(1);

    struct sockaddr_un privateAddr = {AF_UNIX, buffer};
    strcpy(privateAddr.sun_path,buffer);

    int newSock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (newSock < 0)
        perror("ERROR opening socket");
    if (connect(newSock,(struct sockaddr *) &privateAddr,sizeof(privateAddr)) < 0) 
        perror("ERROR connecting");

    for(int i = 0 ; i < numberOfWorkers; i++)
    {
        write(newSock,&i, sizeof(i));
        perror("write");
        sleep(1);
    }
    while(1)
        pause();
    close(newSock);
    return 0;
}
