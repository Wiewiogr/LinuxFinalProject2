#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

int main(int argc, char* argv[])
{
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_un serv_addr = {AF_UNIX, "registrationChannel"};
    struct sockaddr_un cli_addr;
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");
    //bzero((char *) &serv_addr, sizeof(serv_addr));
//    serv_addr.sun_family = AF_UNIX;
//    serv_addr.sun_path = "registrationChannel\0";
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
        perror("ERROR on accept");
    //bzero(buffer,256);
    int n;
    n = read(newsockfd,buffer,255);
    if (n < 0) perror("ERROR reading from socket");
        printf("Here is the message: %s\n",buffer);
    n = write(newsockfd,"I got your message",18);
    if (n < 0) perror("ERROR writing to socket");
    close(newsockfd);
    close(sockfd);
    return 0;
}
