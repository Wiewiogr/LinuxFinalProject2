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
    //printf("%c %ld.%ld\n", msg.value, msg.time.tv_sec, msg.time.tv_nsec);
    return msg;
}



//void updateMessages(struct Messages** msgs, int index, char* buffer)
//{
//    printf("from updateMessage : %c\n", buffer[0]);
//    int numberOfMsg = msgs[index]->numberOfMessages++;
//    msgs[index]->messsages[numberOfMsg].value = buffer[0];
//    char *sec;
//    msgs[index]->messsages[numberOfMsg].time.tv_sec = strtoll(buffer+1,&sec,10);
//    msgs[index]->messsages[numberOfMsg].time.tv_nsec = strtoll(sec+1,NULL,10);
//    //printf("from updateMessage : %ld.%ld\n",msgs[index].messsages[numberOfMsg].time.tv_sec,msgs[index].messsages[numberOfMsg].time.tv_nsec);
//    
//}
struct Worker
{
    char position;
    char groupId[25];
    int socketFd;
    int originSocketFd;
};

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
        res = poll(pollFds,numberOfConnections,-1);
        if(pollFds[0].revents & POLLIN) // registrationChannel
        {
            printf("publiczny socket\n");
            int newsockfd = acceptConnection(pollFds[0].fd);
            char buffer[20];
            read(newsockfd,buffer,20);

            printf("przeczytałem %s\n", buffer);

            if (write(newsockfd,buffer,strlen(buffer)) < 0)
                perror("write");
            close(newsockfd);

            int newSock = createNewWorkerSocket(buffer,'l');
            printf("new Sock created : %d\n", newSock);

            strcpy(workers[numberOfConnections-1].groupId,buffer);
            workers[numberOfConnections-1].socketFd = newSock;
            workers[numberOfConnections-1].originSocketFd = 0;
            workers[numberOfConnections-1].position = 'l';
            updatePollfd(pollFds,numberOfConnections++, newSock);

            listen(newSock, 2);

            numberOfGroups++;
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
                            printf("craete command, with number %d\n",number);
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
                            qsort(messages[0].messsages,
                                    messages[0].numberOfMessages,
                                    sizeof(struct Message),messageComp);

                            for(int i = 0 ; i < messages[0].numberOfMessages; i++)
                            {
                                printf("%c",messages[0].messsages[i].value);
                                printf(" %ld.",messages[0].messsages[i].sec);
                                printf("%ld\n",messages[0].messsages[i].nsec);
                            }
                            printf("\n");
//                            for(int i = 0 ; i < messages[0].numberOfMessages; i++)
//                            {
//                                printf("%c",messages[0].messsages[i].value);
//                            }
//                            printf("\n");
                        }
                    }
                    else if(workers[i-1].position == 'w')
                    {
                        char buffer[50];
                        read(workers[i-1].socketFd,buffer,sizeof(buffer));
                        printf("message from worker : %s\n", buffer);
                        struct Message* newMessage = createMessageFromString(buffer);
                        printf("new message :  %c %ld.%ld\n", newMessage->value, newMessage->sec, newMessage->sec);
                        messages[0].messsages[messages[0].numberOfMessages] = *newMessage;

                        printf("from poll : %c %ld.%ld\n",messages[0].messsages[messages[0].numberOfMessages].value, messages[0].messsages[messages[0].numberOfMessages].sec, messages[0].messsages[messages[0].numberOfMessages].nsec);
                        messages[0].numberOfMessages++;
                        //messages[0].numberOfMessages++;
                    }
                }
            }
        }


    }
    close(sockfd);
    return 0;
}
