#include <unistd.h>
#include <stdio.h>
#include "common.h"

void timerHandler(int sig, siginfo_t *si, void *uc)
{
    printf("dostalem sygnal!!!\n");
    exit(1);
}

int main()
{
    registerHandler(SIGALRM,timerHandler);
    printf("my group : %d\n", getpgrp());
    while(1)
        pause();
    return 0;
}
