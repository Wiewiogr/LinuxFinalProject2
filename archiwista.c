#include <stdio.h>
#include <time.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "common.h"

int createNewWorkerSocket(char name[], char position)
{
    struct sockaddr_un workerAddr = createAbstractSockaddr(name);
    int sockfd;
    if(position == 'l')
    {
        if((sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
            perror("socket");
    }
    else if(position == 'w')
    {
        printf("creating worker socket %s\n",name);
        if((sockfd = socket(AF_UNIX, SOCK_DGRAM , 0)) < 0)
            perror("socket");
    }
    if (bind(sockfd, (struct sockaddr *) &workerAddr, sizeof(struct sockaddr_un)) < 0)
        perror("bind");
    return sockfd;
}

int acceptConnection(int fd)
{
    struct sockaddr_un cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    return accept(fd, (struct sockaddr *) &cli_addr, &clilen);
}


void updatePollfd(struct pollfd* pollFds, int index, int fd)
{
    pollFds[index].fd = fd;
    pollFds[index].events = POLLIN;
    pollFds[index].revents = 0;
}

struct Message* createMessageFromString(char* buffer)
{
    struct Message* msg = malloc(sizeof(struct Message));
    msg->value = buffer[0];
    char *sec;
    msg->sec = strtoll(buffer+1,&sec,10);
    msg->nsec = strtoll(sec+1,NULL,10);
    return msg;
}

int getGroupIndex(char* groupId, struct Messages*  msgs, int numberOfGroups )
{
    for(int i = 0 ; i < numberOfGroups ; i++)
    {
        printf("group id %s ? %s\n", msgs[i].group, groupId);
        if(strncmp(msgs[i].group,groupId, strlen(msgs[i].group)-1) == 0)
        {
            printf("szukana grupa to : %d\n", i);
            return i;
        }
    }
    printf("jebac grupy\n");
}



void removeFromPollFd(struct pollfd * pollfds, int fd, int size)
{
    int target = 0;
    for(int i = 1; i < size; i++)
    {
        printf("%d ? %d\n", pollfds[i].fd,fd);
        if(pollfds[i].fd == fd)
        {
            target = i;
            break;
        }
    }
    if(target != 0)
    {
        for(int i = target; i < size-1; i++)
        {
            pollfds[i] = pollfds[i+1];
        }
    }
//    printf("after\n");
//    for(int i = 0; i < size; i++)
//        printf("pollfd[%d] = %d\n",i,pollfds[i].fd);
    if(target == 0)
        printf("nie znalazlo takiego fd!!!\n");

}

struct Worker
{
    char position;
    char groupId[25];
    int socketFd;
    int originSocketFd;
};

void removeFromMessages(struct Messages * msgs, int index, int size)
{
    printf("before\n");
    for(int i = 0; i < size; i++)
        printf("msgs[%d] = %s\n",i,msgs[i].group);
    msgs[index].numberOfMessages = 0;
    for(int i = index; i < size-1; i++)
    {
        msgs[i] = msgs[i+1];
    }
    printf("after\n");
    for(int i = 0; i < size; i++)
        printf("msgs[%d] = %s\n",i,msgs[i].group);
}

void removeFromWorkers(struct Worker * workers, int index, int size)
{
    printf("before\n");
    for(int i = 0; i < size; i++)
        printf("workers[%d] = %d\n",i,workers[i].socketFd);
    for(int i = index; i < size-1; i++)
    {
        workers[i] = workers[i+1];
    }
    printf("after\n");
    for(int i = 0; i < size; i++)
        printf("workers[%d] = %d\n",i,workers[i].socketFd);
}


int numberOfConnections;
int numberOfGroups;
struct Worker workers[35];

int main(int argc, char* argv[])
{
    struct Messages messages[5];
    int sockfd, newsockfd;
    struct sockaddr_un serv_addr = {AF_UNIX, "registrationChannel"};

    if((sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
        perror("ERROR opening socket");
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR on binding");

    struct pollfd pollFds[25];

    updatePollfd(pollFds,numberOfConnections++, sockfd);
    listen(sockfd,5);
    int res;
    while(1)
    {
        printf("numberOfConnections : %d\n", numberOfConnections);
        listen(sockfd,5);
        res = poll(pollFds,numberOfConnections,-1);
        if(pollFds[0].revents & POLLIN) // registrationChannel
        {
            printf("publiczny socket\n");
            int newsockfd = acceptConnection(pollFds[0].fd);
            char buffer[20] = {0};
            read(newsockfd,buffer,20);

            printf("przeczytałem %s\n", buffer);

            if (write(newsockfd,buffer,strlen(buffer)) < 0)
                perror("write");
            close(newsockfd);

            int newSock = createNewWorkerSocket(buffer,'l');
            printf("new Sock created : %d\n", newSock);

            strcpy(messages[numberOfGroups++].group, buffer);
            strcpy(workers[numberOfConnections-1].groupId,buffer);
            workers[numberOfConnections-1].socketFd = newSock;
            workers[numberOfConnections-1].originSocketFd = 0;
            workers[numberOfConnections-1].position = 'l';
            updatePollfd(pollFds,numberOfConnections++, newSock);

            listen(newSock, 2);

//            if(messages == NULL)
//            {
//                messages = (struct Messages*)malloc(sizeof(struct Messages));
//                printf("malloc!!!!");
//            }
//            else
//            {
//                messages = realloc(messages,numberOfGroups * sizeof(struct Messages));
//            }
        }
        else
        {
            for(int i = 1; i < numberOfConnections; i++)
            {
                if(pollFds[i].revents & POLLIN)
                {
                    printf("pollin!!!!\n");
                    if(workers[i-1].position == 'l')
                    {
                        if(workers[i-1].originSocketFd == 0)
                        {
                            int newsockfd = acceptConnection(workers[i-1].socketFd);
                            if(newsockfd == -1)
                            {
                                printf("zly fd\n");
                                exit(1);
                            }
                            workers[i-1].originSocketFd = workers[i-1].socketFd;
                            workers[i-1].socketFd = newsockfd;
                            updatePollfd(pollFds,numberOfConnections-1, newsockfd);
                        }
                        char buffer[50] = {0};
                        read(workers[i-1].socketFd,buffer,sizeof(buffer));
                        printf("received : %s\n",buffer);
                        char* command;
                        int number = strtol(buffer,&command,10);
                        printf("command : %s\n", command);
                        if(strcmp(command, "create") == 0)
                        {
                            printf("create command, with number %d\n",number);
                            char workersSocketPath[35];
                            sprintf(workersSocketPath,"%s%d",workers[i-1].groupId,number);
                            write(workers[i-1].socketFd,workersSocketPath,sizeof(workersSocketPath));
                            int newSock = createNewWorkerSocket(workersSocketPath,'w');

                            strcpy(workers[numberOfConnections-1].groupId,workersSocketPath);
                            workers[numberOfConnections-1].socketFd = newSock;
                            workers[numberOfConnections-1].originSocketFd = 0;
                            workers[numberOfConnections-1].position = 'w';

                            updatePollfd(pollFds,numberOfConnections++, newSock);

                        }
                        else if(strcmp(command, "done") == 0)
                        {
                            int index = getGroupIndex(workers[i-1].groupId, messages, numberOfGroups);
                            qsort(messages[index].messsages,
                                    messages[index].numberOfMessages,
                                    sizeof(struct Message),messageComp);

                            for(int i = index ; i < messages[index].numberOfMessages; i++)
                            {
                                printf("%c",messages[index].messsages[i].value);
                                printf(" %ld.",messages[index].messsages[i].sec);
                                printf("%ld\n",messages[index].messsages[i].nsec);
                            }
                            removeFromMessages(messages, index, numberOfGroups--);
                            printf("\n");

                            int numberOfWorkers = numberOfConnections;
                            char groupName[25];
                            strcpy(groupName, workers[i-1].groupId);
                            for(int j = 0 ; j < numberOfWorkers; j++)
                            {
                                printf("%s ? %s\n",workers[j].groupId,groupName);
                                if(strncmp(workers[j].groupId,groupName,strlen(groupName)-1)==0)
                                {
                                    printf("zamykam socket\n");
                                    close(workers[j].socketFd);
                                    if(workers[j].position == 'l')
                                    {
                                        close(workers[j].originSocketFd);
                                    }
    printf("before\n");
    for(int i = 0; i < numberOfConnections; i++)
        printf("pollfd[%d] = %d\n",i,pollFds[i].fd);
                                    removeFromPollFd(pollFds, workers[j].socketFd,numberOfConnections--);
    printf("after\n");
    for(int i = 0; i < numberOfConnections+1; i++)
        printf("pollfd[%d] = %d\n",i,pollFds[i].fd);
                                    removeFromWorkers(workers,j,numberOfConnections);

                                }

                            }
                            //exit(1);

//                            for(int i = 0 ; i < messages[index].numberOfMessages; i++)
//                            {
//                                printf("%c",messages[index].messsages[i].value);
//                            }
//                            printf("\n");
                        }
                    }
                    else if(workers[i-1].position == 'w')
                    {
                        int index = getGroupIndex(workers[i-1].groupId, messages, numberOfGroups);
                        char buffer[50];
                        read(workers[i-1].socketFd,buffer,sizeof(buffer));
                        printf("message from worker : %s, strlen = %d, od worker fd %d od pollfd %d\n", buffer, strlen(buffer), workers[i-1].socketFd, pollFds[i].fd);
                        struct Message* newMessage = createMessageFromString(buffer);
                        printf("new message :  %c %ld.%ld\n", newMessage->value, newMessage->sec, newMessage->sec);
                        messages[index].messsages[messages[index].numberOfMessages] = *newMessage;

                        printf("from poll : %c %ld.%ld\n",messages[index].messsages[messages[index].numberOfMessages].value, messages[index].messsages[messages[index].numberOfMessages].sec, messages[index].messsages[messages[index].numberOfMessages].nsec);
                        messages[index].numberOfMessages++;
                    }
                }
                else if(pollFds[i].revents & POLLHUP)
                {
                    printf("POOLLHUP\n");
                }
            }
        }


    }
    close(sockfd);
    return 0;
}
