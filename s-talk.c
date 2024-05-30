#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "list.h"

#include "threads.h"

int main(int argc, char *argv[])
{
    if (argc > 4)
    {
        printf("System: Error! too many arguments.\n");
    }
    else if (argc < 4)
    {
        printf("System: Error! too few arguments.\n");
    }
    else
    {
        printf("SYSTEM: Welcome to s-talk chat bot by Tawheed Sarker Aakash and Md Ashraful Islam Bhuiyan.\nYou are connecting from port: %d to %s:%d\n", atoi(argv[1]), argv[2], atoi(argv[3]));
        Threads_init(argv[2], atoi(argv[3]), atoi(argv[1]));
        Sender_shutdown();
        Receiver_shutdown();
        printf("SYSTEM: Port: %d disconnected successfully.\n", atoi(argv[1]));
    }
    return 0;
}