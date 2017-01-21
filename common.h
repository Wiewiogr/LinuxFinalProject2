#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

extern void registerHandler(int signalNumber, void(*handler)(int, siginfo_t*, void*));

extern char* createAbstractName(char name[]);
