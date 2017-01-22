#define _GNU_SOURCE
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

struct Worker
{
    char position;
    char groupId[25];
    int socketFd;
    int originSocketFd;
};

void updateWorkers(struct Worker* workers, int newSock, char position, char * groupId, int index)
{
    strcpy(workers[index-1].groupId,groupId);
    workers[index-1].socketFd = newSock;
    workers[index-1].originSocketFd = 0;
    workers[index-1].position = position;
}

void removeFromWorkers(struct Worker * workers, int index, int size)
{
    for(int i = index; i < size-1; i++)
    {
        workers[i] = workers[i+1];
    }
}

int removeFromWorkerAndPollFd(struct Worker * workers, struct pollfd * pollFds, int numberOfConnections,char * groupId)
{
    int numberOfWorkers = numberOfConnections-1;
    for(int j = numberOfWorkers ; j > 0; j--)
    {
        if(strcmp(workers[j-1].groupId,groupId)==0)
        {
            close(workers[j-1].socketFd);
            if(workers[j-1].position == 'l')
            {
                close(workers[j-1].originSocketFd);
            }
            removeFromPollFd(pollFds, workers[j-1].socketFd,numberOfConnections--);
            removeFromWorkers(workers,j-1,numberOfConnections);
        }
    }
    return numberOfConnections;
}


int numberOfConnections;
int numberOfGroups;
struct Worker workers[35];

int main(int argc, char* argv[])
{
    struct Messages messages[5];
    int sockfd, newsockfd;
    char publicChannel[50] = "registrationChannel";

    int opt;
    while ((opt = getopt(argc, argv, "c:")) != -1)
    {
        switch (opt)
        {
        case 'c':
            strcpy(publicChannel,optarg);
            break;
        }
    }

    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, publicChannel);
    unlink(publicChannel);

    if((sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
        perror("socket");
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("bind");

    struct pollfd pollFds[25];
    updatePollfd(pollFds,numberOfConnections++, sockfd);
    listen(sockfd,5);

    while(1)
    {
        if(numberOfConnections <=  0)
            exit(1);
        listen(sockfd,5);
        int res = poll(pollFds,numberOfConnections,-1);
        if(pollFds[0].revents & POLLIN)
        {
            int newsockfd = acceptConnection(pollFds[0].fd);
            char buffer[20] = {0};
            read(newsockfd,buffer,20);
            if(getGroupIndex(buffer,messages, numberOfGroups) != -1)
            {
                if (write(newsockfd,"again\0",6) < 0)
                    perror("write");
                printf("There is group with that id!\n");
                close(newsockfd);
                int index = getGroupIndex(buffer, messages, numberOfGroups);
                removeFromMessages(messages, index, numberOfGroups--);
                numberOfConnections = removeFromWorkerAndPollFd(workers,pollFds,numberOfConnections,buffer);
            }
            else
            {
                printf("Creating group with id : %s\n",buffer);

                if (write(newsockfd,buffer,strlen(buffer)) < 0)
                    perror("write");
                close(newsockfd);

                int newSock = createNewWorkerSocket(buffer,'l');

                strcpy(messages[numberOfGroups++].group, buffer);
                updateWorkers(workers, newSock, 'l', buffer, numberOfConnections);
                updatePollfd(pollFds,numberOfConnections++, newSock);

                listen(newSock, 2);
            }
        }
        else
        {
            for(int i = 1; i < numberOfConnections; i++)
            {
                if(pollFds[i].revents & POLLIN)
                {
                    if(workers[i-1].position == 'l')
                    {
                        if(workers[i-1].originSocketFd == 0)
                        {
                            int newsockfd = acceptConnection(workers[i-1].socketFd);
                            if(newsockfd == -1)
                            {
                                exit(1);
                            }
                            workers[i-1].originSocketFd = workers[i-1].socketFd;
                            workers[i-1].socketFd = newsockfd;
                            updatePollfd(pollFds,numberOfConnections-1, newsockfd);
                        }
                        char buffer[100] = {0};
                        read(workers[i-1].socketFd,buffer,sizeof(buffer));
                        char* command;
                        int number = strtol(buffer,&command,10);
                        if(strncmp(command, "create", 6) == 0)
                        {
                            printf("Creating socket for worker with groupName : %s\n",workers[i-1].groupId);
                            char workersSocketPath[35];
                            sprintf(workersSocketPath,"%s%d",workers[i-1].groupId,number);
                            write(workers[i-1].socketFd,workersSocketPath,sizeof(workersSocketPath));
                            int newSock = createNewWorkerSocket(workersSocketPath,'w');
                            updateWorkers(workers, newSock, 'w', workers[i-1].groupId, numberOfConnections);
                            updatePollfd(pollFds,numberOfConnections++, newSock);
                        }
                        else if(strncmp(command, "done",4 ) == 0)
                        {
                            int index = getGroupIndex(workers[i-1].groupId, messages, numberOfGroups);
                            qsort(messages[index].messsages,
                                    messages[index].numberOfMessages,
                                    sizeof(struct Message),messageComp);

                            char * completeMessage = malloc((messages[index].numberOfMessages+1)*sizeof(char));
                            memset(completeMessage,0,(messages[index].numberOfMessages+1)*sizeof(char));
                            for(int i = index ; i < messages[index].numberOfMessages; i++)
                            {
                                completeMessage[i] = messages[index].messsages[i].value;
                            }
                            if(strcmp(command+4, "INT")!=0)
                            {
                                printf("Complete message : %s\n", completeMessage);
                                printf("Received md5sum : \n%s\n",command+4 );
                                char md5sum[90] = {0};
                                strcpy(md5sum,getMD5sum(completeMessage));
                                printf("Calculated md5sum : \n%s\n",md5sum);

                                if(strcmp(command+4, md5sum) == 0)
                                {
                                    printf("Message is complete and correct, checsums match\n");
                                }
                                else
                                {
                                    printf("Message is corrupted, checksums doesn not match\n");
                                }
                            }
                            else
                            {
                                printf("Message transmission was interrupted\n");
                                printf("Incomplete message : %s\n", completeMessage);
                            }

                            removeFromMessages(messages, index, numberOfGroups--);
                            numberOfConnections = removeFromWorkerAndPollFd(workers,pollFds,numberOfConnections,workers[i-1].groupId);
                        }
                    }
                    else if(workers[i-1].position == 'w')
                    {
                        int index = getGroupIndex(workers[i-1].groupId, messages, numberOfGroups);
                        char buffer[50];
                        read(workers[i-1].socketFd,buffer,sizeof(buffer));
                        struct Message* newMessage = createMessageFromString(buffer);
                        messages[index].messsages[messages[index].numberOfMessages] = *newMessage;
                        messages[index].numberOfMessages++;
                    }
                }
                else if(pollFds[i].revents & POLLHUP)
                {
                    int index = getGroupIndex(workers[i-1].groupId, messages, numberOfGroups);
                    removeFromMessages(messages, index, numberOfGroups--);
                    numberOfConnections = removeFromWorkerAndPollFd(workers,pollFds,numberOfConnections,workers[i-1].groupId);
                }
            }
        }


    }
    close(sockfd);
    return 0;
}
