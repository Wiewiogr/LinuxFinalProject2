#include "common.h"

void registerHandler(int signalNumber, void(*handler)(int, siginfo_t*, void*))
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signalNumber, &sa, NULL) == -1)
        perror("sigaction");
    printf("zarejestrowalem sygnal\n");
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

