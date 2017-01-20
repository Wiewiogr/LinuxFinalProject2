#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>


int createNewWorkerSocket(char name[], char position)
{
    int sockfd;
    struct sockaddr_un workerAddr = {AF_UNIX, name};
    strcpy(workerAddr.sun_path, name);
    if(position == 'l')
    {
        if((sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
            perror("socket");
    }
    else
    {
        printf("worker\n");

    }
    if (bind(sockfd, (struct sockaddr *) &workerAddr, sizeof(workerAddr)) < 0)
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


struct Worker
{
    char position;
    char groupId;
    int socketFd;
    int originSocketFd;
};

int numberOfConnections;
struct Worker workers[25];

int main(int argc, char* argv[])
{
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


            workers[numberOfConnections-1].socketFd = newSock;
            workers[numberOfConnections-1].originSocketFd = 0;
            workers[numberOfConnections-1].position = 'l';
            updatePollfd(pollFds,numberOfConnections++, newSock);
            listen(newSock, 2);
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
                        int number;
                        read(workers[i-1].socketFd,&number,sizeof(number));
                        printf("przeczytałem %d\n", number);
                    }
                }
            }
        }


    }
    close(sockfd);
    return 0;
}
