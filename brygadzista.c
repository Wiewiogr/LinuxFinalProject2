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


struct Worker
{
    int pid;
    char socketName[25];
};


int workerGroupPid;
char brigadeId[] = "aa\0";
int numberOfWorkers;

void onExit()
{
    killpg(workerGroupPid, SIGKILL);
}

void createWorker(char* socketName, int pipeFd[2])
{
    char socketArg[25];
    sprintf(socketArg,"-s%s", socketName);
    char * newArgs[] =
    {
        "./robotnik.out",
        socketArg,
        (char *) 0
    };

    if(fork() == 0)
    {
        dup2(pipeFd[0],0);
        close(pipeFd[1]);
        setpgid(0,workerGroupPid);
        execvp("./robotnik.out",newArgs);
        exit(1);
    }
}


int main(int argc, char* argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "i:n:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            strcpy(brigadeId,optarg);
            break;
        case 'n':
            numberOfWorkers = atoi(optarg);
            break;
        }
    }
    printf("brygadzista gid : %d\n", getpgrp());
    if((workerGroupPid = fork()) == 0)
    {
        setpgid(0,0);
        exit(1);
    }

    sleep(1);

    atexit(onExit);
    killpg(workerGroupPid, SIGALRM); ///

    int sockfd, portno, n;
    struct sockaddr_un serv_addr = {AF_UNIX, "registrationChannel\0"};

    char buffer[256] = {0};
    if(sockfd = socket(AF_UNIX, SOCK_STREAM, 0))
        perror("socket");
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        perror("connect");
    write(sockfd,brigadeId,strlen(brigadeId));
    read(sockfd,buffer,255);
        perror("read");
    close(sockfd);
    printf("read : %s\n", buffer);

    struct sockaddr_un privateAddr = {AF_UNIX, buffer};
    char* socketName = createAbstractName(buffer);
    strcpy(privateAddr.sun_path,socketName);
    free(socketName);

    int newSock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (newSock < 0)
        perror("ERROR opening socket");
    if (connect(newSock,(struct sockaddr *) &privateAddr,sizeof(privateAddr)) < 0)
        perror("ERROR connecting");
    int pipeFd[2];
    pipe(pipeFd);
    for(int i = 0 ; i < numberOfWorkers; i++)
    {
        char request[25];
        sprintf(request,"%dcreate",i);
        write(newSock,request, sizeof(request));
        char response[35];
        read(newSock,response, sizeof(response));
        createWorker(response,pipeFd);
        write(pipeFd[1],"dupa\0",5);
        sleep(1);
    }

    for(int i = 0 ; i < 100;i++)
    {
        write(pipeFd[1],&i,sizeof(i));
        sleep(1);
    }
    while(1)
        pause();
    close(newSock);
    return 0;
}
