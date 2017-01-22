#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <math.h>
#include <openssl/md5.h>

struct Message
{
    long int sec;
    long int nsec;
    char value;
    //struct timespec time;
};

struct Messages
{
    char group[25];
    struct Message messsages[150];
    int numberOfMessages;
};

extern void registerHandler(int signalNumber, void(*handler)(int, siginfo_t*, void*));

extern struct sockaddr_un createAbstractSockaddr(char name[]);

extern void convertFloatToTimeSpec(float time, struct timespec * ts);

extern void createTimerAndRegisterHandler(timer_t *timerId, void(*handler)(int, siginfo_t*, void*));

extern void setTimer(timer_t timerId,struct itimerspec *timeSpec);

extern int messageComp(const void* first, const void* second);

extern char* getMD5sum(char * string);
