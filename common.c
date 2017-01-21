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

extern char* createAbstractName(char name[])
{
    char* newName = (char*)malloc(50);
    sprintf(newName,"\0%s",name);
    return newName;
}
