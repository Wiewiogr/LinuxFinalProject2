#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "common.h"
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

struct Worker
{
    int pid;
    char socketName[25];
};

int workerGroupPid;
char brigadeId[10] = "aa";
int numberOfWorkers;
int newSock;

void timerHandler(int sig, siginfo_t *si, void *uc)
{
    killpg(workerGroupPid, SIGALRM);
}

void interruptHandler(int sig, siginfo_t *si, void *uc)
{
    killpg(workerGroupPid, SIGKILL);
    write(newSock,"1doneINT",8);
    exit(1);
}

void onExit()
{
    killpg(workerGroupPid, SIGKILL);
}

void createWorker(struct Worker* workers, int index, char* socketName, int pipeFd[2])
{
    printf("Creating worker...\n");
    char socketArg[25];
    sprintf(socketArg,"-s%s", socketName);
    char * newArgs[] =
    {
        "./robotnik.out",
        socketArg,
        (char *) 0
    };
    int pid;
    if((pid = fork()) == 0)
    {
        dup2(pipeFd[0],0);
        close(pipeFd[1]);
        setpgid(0,workerGroupPid);
        execvp("./robotnik.out",newArgs);
        exit(1);
    }
    workers[index].pid = pid;
    strcpy(workers[index].socketName, socketName);
}

int connectToSocket(char name[])
{
    int sockfd;
    struct sockaddr_un serv_addr = {AF_UNIX, ""};
    strcpy(serv_addr.sun_path,name);
    if(sockfd = socket(AF_UNIX, SOCK_STREAM, 0))
        perror("socket");
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        perror("connect");
    return sockfd;
}

int main(int argc, char* argv[])
{
    struct itimerspec timeStamp;
    timer_t timerId;
    int opt;
    char message[256] = "message";
    char publicChannel[50] = "registrationChannel";

    while ((opt = getopt(argc, argv, "i:n:t:m:c:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            strcpy(brigadeId,optarg);
            break;
        case 'n':
            numberOfWorkers = atoi(optarg);
            break;
        case 't':
            convertFloatToTimeSpec(strtof(optarg,NULL),&timeStamp.it_value);
            timeStamp.it_interval = timeStamp.it_value;
            break;
        case 'm':
            strcpy(message,optarg);
            break;
        case 'c':
            strcpy(publicChannel,optarg);
            break;
        }
    }

    struct Worker * workers = malloc(numberOfWorkers*sizeof(struct Worker));

    if((workerGroupPid = fork()) == 0)
    {
        setpgid(0,0);
        exit(1);
    }

    sleep(1);
    atexit(onExit);

    char buffer[50] = {0};
    while(1)
    {
        int sockfd = connectToSocket(publicChannel);
        write(sockfd,brigadeId,strlen(brigadeId));
        if(read(sockfd,buffer,255) == -1)
            perror("read");
        close(sockfd);
        if(strcmp(buffer,"again")!=0)
            break;
        else
        {
            printf("Trying again.\n");
            memset(buffer,0, 50 *sizeof(char));
        }
    }

    struct sockaddr_un privateAddr = createAbstractSockaddr(buffer);
    if((newSock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        perror("socket");
    if (connect(newSock,(struct sockaddr *) &privateAddr,sizeof(privateAddr)) < 0)
        perror("connect");

    int pipeFd[2];
    pipe2(pipeFd,O_NONBLOCK);

    for(int i = 0 ; i < numberOfWorkers; i++)
    {
        char request[35];
        sprintf(request,"%dcreate",i);
        write(newSock,request, sizeof(request));
        char response[35];
        read(newSock,response, sizeof(response));
        sleep(1);
        createWorker(workers,i,response,pipeFd);
    }

    write(pipeFd[1],&message,strlen(message));

    registerHandler(SIGINT,interruptHandler);
    createTimerAndRegisterHandler(&timerId,timerHandler);
    setTimer(timerId,&timeStamp);

    int status;

    while(1)
    {
        int res = waitpid(-workerGroupPid,&status,0);

        if (WIFEXITED(status))
        {
            if(WEXITSTATUS(status) == 0)
            {
                printf("Worker finished his job.\n");
                if(--numberOfWorkers == 0)
                {
                    char res[90] = {0};
                    strcpy(res,getMD5sum(message));
                    char finalMessage[110] = {0};
                    sprintf(finalMessage, "1done%s", res);
                    write(newSock,finalMessage,strlen(finalMessage));
                    break;
                }
            }
        }
        else if(WIFSIGNALED(status))
        {
            printf("Worker killed...\n");
            for(int i = 0; i < numberOfWorkers; i++)
            {
                if(workers[i].pid == res)
                {
                    createWorker(workers, i, workers[i].socketName, pipeFd);
                    break;
                }
            }
        }
    }

    free(workers);
    close(newSock);
    return 0;
}
