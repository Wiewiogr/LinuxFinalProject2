#include "common.h"

void registerHandler(int signalNumber, void(*handler)(int, siginfo_t*, void*))
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signalNumber, &sa, NULL) == -1)
        perror("sigaction");
}


extern struct sockaddr_un createAbstractSockaddr(char name[])
{
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path+1,name,strlen(name));
    return addr;
}


void convertFloatToTimeSpec(float time, struct timespec * ts)
{
    ts->tv_sec = floor(time);
    ts->tv_nsec = (time - floor(time))*1000000000;
}


void createTimerAndRegisterHandler(timer_t *timerId, void(*handler)(int, siginfo_t*, void*))
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1)
        perror("sigaction");

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = timerId;
    if (timer_create(CLOCK_REALTIME, &sev, timerId) == -1)
        perror("timer_create");
}

void setTimer(timer_t timerId,struct itimerspec *timeSpec)
{
    if (timer_settime(timerId, 0, timeSpec, NULL) == -1)
        perror("timer_settime");
}

extern int messageComp(const void* first, const void* second)
{
    struct Message f = *((struct Message*)first);
    struct Message s = *((struct Message*)second);
    if(f.sec > s.sec) return 1;
    else if(f.sec < s.sec) return -1;
    if(f.nsec > s.nsec) return 1;
    else if(f.nsec < s.nsec) return -1;
    return 0;
}

extern char* getMD5sum(char * string)
{
    char result[MD5_DIGEST_LENGTH];
    char *res = MD5(string, strlen(string), result);
    char *readableSum = malloc(70);
    memset(readableSum,0,70);
    for(int n=0; n<MD5_DIGEST_LENGTH; n++)
        sprintf(readableSum,"%s%02x",readableSum,  result[n]);
    return readableSum;
}

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
    pollFds[index].events = POLLIN | POLLRDHUP;
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
        if(strncmp(msgs[i].group,groupId, strlen(msgs[i].group)-1) == 0)
        {
            return i;
        }
    }
    return -1;
}

void removeFromPollFd(struct pollfd * pollfds, int fd, int size)
{
    int target = 0;
    for(int i = 1; i < size; i++)
    {
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
}

void removeFromMessages(struct Messages * msgs, int index, int size)
{
    msgs[index].numberOfMessages = 0;
    for(int i = index; i < size-1; i++)
    {
        msgs[i] = msgs[i+1];
    }
}

