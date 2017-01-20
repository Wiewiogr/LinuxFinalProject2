#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

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

    return 0;
}
