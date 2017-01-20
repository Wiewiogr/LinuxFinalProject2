#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h> 

int workerGroupPid;

void onExit()
{
    killpg(workerGroupPid, SIGKILL);
}

int main()
{
    printf("brygadzista gid : %d", getpgrp());
    if((workerGroupPid = fork()) == 0)
    {
        setpgid(0,0);
        for(int i = 0; i < 5; i++)
        {
            char * newArgs[] =
            {
                (char *) 0
            };

            if(fork() == 0)
            {
                execvp("./robotnik.out",newArgs);
                exit(1);
            }
        }
        exit(1);
    }
    printf("workerGroupPid : %d", workerGroupPid);
    atexit(onExit);
    sleep(1);
    killpg(workerGroupPid, SIGALRM);

    int sockfd, portno, n;
    struct sockaddr_un serv_addr = {AF_UNIX, "registrationChannel\0"};
    struct hostent *server;

    char buffer[256];
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");
//    server = gethostbyname(argv[1]);
//    if (server == NULL) {
//        fprintf(stderr,"ERROR, no such host\n");
//        exit(0);
//    }
    //bzero((char *) &serv_addr, sizeof(serv_addr));
//    bcopy((char *)server->h_addr, 
//    (char *)&serv_addr.sin_addr.s_addr,
//    server->h_length);
//    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR connecting");
    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
        perror("ERROR writing to socket");
    //bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
        perror("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
    return 0;
    return 0;
}
