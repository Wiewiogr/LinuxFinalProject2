#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "common.h"
#include <signal.h>
#include <time.h>
#include <fcntl.h>

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

void timerHandler(int sig, siginfo_t *si, void *uc)
{
    printf("handler!!1\n");
    killpg(workerGroupPid, SIGALRM);
}

void onExit()
{
    killpg(workerGroupPid, SIGKILL);
    printf("on exit!!!\n");
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

    while ((opt = getopt(argc, argv, "i:n:t:")) != -1)
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
        }
    }

    if((workerGroupPid = fork()) == 0)
    {
        setpgid(0,0);
        exit(1);
    }

    sleep(1);

    atexit(onExit);

    char buffer[256] = {0};
    {
        int sockfd = connectToSocket("registrationChannel\0");

        write(sockfd,brigadeId,strlen(brigadeId));
        read(sockfd,buffer,255);
            perror("read");
        close(sockfd);
        printf("read : %s\n", buffer);
    }

    struct sockaddr_un privateAddr = createAbstractSockaddr(buffer);
    int newSock;
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
        createWorker(response,pipeFd);
    }

    char writtenText[] = "dupa Dupa DUpa DUPa DUPA";
    write(pipeFd[1],&writtenText,sizeof(writtenText));

    createTimerAndRegisterHandler(&timerId,timerHandler);
    setTimer(timerId,&timeStamp);

    int status;
    while(1)
    {
        int res = waitpid(-workerGroupPid,&status,0);
        if(res == -1)
            printf("zjeblo sie\n");
        if (WIFEXITED(status))
        {
            if(WEXITSTATUS(status) == 0)
            {
                if(--numberOfWorkers == 0)
                {
                    write(newSock,"1done", 5);
                    break;
                }
            }
        }

    }

    while(1)
        pause();
    close(newSock);
    return 0;
}
