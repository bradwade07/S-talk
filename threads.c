#include "threads.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFERSIZE 1024
#define LISTSIZE 50

static pthread_t Sender_sendThread;
static pthread_t Sender_keyboardThread;
static pthread_t Receiver_receiveThread;
static pthread_t Receiver_printThread;
static List *Sender_list;
static List *Receiver_list;
static pthread_cond_t Sender_pthreadSendCondVar = PTHREAD_COND_INITIALIZER;
static pthread_cond_t Sender_pthreadKeyboardCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t Sender_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t Receiver_pthreadReceiveCondVar = PTHREAD_COND_INITIALIZER;
static pthread_cond_t Receiver_pthreadPrintCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t Receiver_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
static char *Receiver_address;
static int Receiver_port;
static int Sender_port;

void *Sender_sendThreadFn(void *unused)
{

    // NOW CREATING THE SOCKET FOR THE RECIPIENT
    char msg[BUFFERSIZE];
    struct sockaddr_in sinRemote;
    memset(&sinRemote, 0, sizeof(sinRemote));
    sinRemote.sin_family = AF_INET;
    sinRemote.sin_port = htons(Receiver_port);
    // CREATE THE SOCKET FOR UDP (RECEIVER)
    int receiverSocketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
    if (receiverSocketDescriptor == -1)
    {
        perror("SYSTEM: Socket creation failed.");
        exit(EXIT_FAILURE);
    }
    // BIND THE SOCKET TO THE PORT (PORT) THAT WE SPECIFY
    while (1)
    {
        if (List_count(Sender_list) > 0)
        {
            if (inet_pton(AF_INET, Receiver_address, &sinRemote.sin_addr) < 0)
            {
                perror("SYSTEM: IP string to binary conversion failed.");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_lock(&Sender_pthreadMutex);
            {
                if (List_count(Sender_list) <= 0)
                {
                    pthread_cond_wait(&Sender_pthreadSendCondVar, &Sender_pthreadMutex);
                }
                strncpy(msg, (char *)List_trim(Sender_list), sizeof(msg) - 1);
                msg[sizeof(msg) - 1] = '\0';
            }
            pthread_mutex_unlock(&Sender_pthreadMutex);
            if (List_count(Sender_list) >= 0 && List_count(Sender_list) < LISTSIZE)
            {
                pthread_cond_signal(&Sender_pthreadKeyboardCondVar);
            }
            ssize_t bytesTx = sendto(receiverSocketDescriptor, msg, strlen(msg), 0, (const struct sockaddr *)&sinRemote, sizeof(sinRemote));
            if (bytesTx == -1)
            {
                perror("SYSTEM: Failed to send.");
                exit(EXIT_FAILURE);
            }
        }
    }
    close(receiverSocketDescriptor);
    return NULL;
}
void *Sender_keyboardThreadFn(void *unused)
{
    char buffer[BUFFERSIZE];
    while (1)
    {
        fgets(buffer, BUFFERSIZE, stdin);
        if (buffer[0] == '!' && (buffer[1] == '\n' || buffer[1] == '\0'))
        {
            pthread_cancel(Sender_sendThread);
            pthread_cancel(Sender_keyboardThread);
            pthread_cancel(Receiver_receiveThread);
            pthread_cancel(Receiver_printThread);
        }
        if (List_count(Sender_list) < LISTSIZE)
        {
            pthread_mutex_lock(&Sender_pthreadMutex);
            {
                if (List_count(Sender_list) >= LISTSIZE)
                {
                    pthread_cond_wait(&Sender_pthreadKeyboardCondVar, &Sender_pthreadMutex);
                }
                List_prepend(Sender_list, buffer);
            }
            pthread_mutex_unlock(&Sender_pthreadMutex);
            if (List_count(Sender_list) > 0 && List_count(Sender_list) < LISTSIZE)
            {
                pthread_cond_signal(&Sender_pthreadSendCondVar);
            }
        }
    }
    return NULL;
}
void *Receiver_receiveThreadFn(void *unused)
{
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    socklen_t sinLen = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(Sender_port);

    // CREATE THE SOCKET FOR UDP
    int senderSocketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
    if (senderSocketDescriptor == -1)
    {
        perror("SYSTEM: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // BIND THE SOCKET TO THE PORT (PORT) THAT WE SPECIFY
    bind(senderSocketDescriptor, (struct sockaddr *)&sin, sizeof(sin));
    char buffer[BUFFERSIZE];
    while (1)
    {
        int bytesRx = recvfrom(senderSocketDescriptor, buffer, BUFFERSIZE, 0, (struct sockaddr *)&sin, &sinLen);
        int terminateIdx = (bytesRx < BUFFERSIZE) ? bytesRx : BUFFERSIZE - 1;
        buffer[terminateIdx] = '\0';
        pthread_mutex_lock(&Receiver_pthreadMutex);
        {
            if (List_count(Receiver_list) >= LISTSIZE)
            {
                pthread_cond_wait(&Receiver_pthreadReceiveCondVar, &Receiver_pthreadMutex);
            }
            List_prepend(Receiver_list, buffer);
        }
        pthread_mutex_unlock(&Receiver_pthreadMutex);
        if (List_count(Receiver_list) > 0 && List_count(Receiver_list) < LISTSIZE)
        {
            pthread_cond_signal(&Receiver_pthreadPrintCondVar);
        }
    }
    close(senderSocketDescriptor);
    return NULL;
}
void *Receiver_printThreadFn(void *unused)
{
    char msg[BUFFERSIZE + 8]; // 5 for port, 2 for : and space and 1 for '\0' = +8
    while (1)
    {
        pthread_mutex_lock(&Receiver_pthreadMutex);
        {
            if (List_count(Receiver_list) <= 0)
            {
                pthread_cond_wait(&Receiver_pthreadPrintCondVar, &Receiver_pthreadMutex);
            }
            sprintf(msg, "%d: ", Receiver_port);
            strncat(msg, (char *)List_trim(Receiver_list), sizeof(msg) - 1);
        }
        pthread_mutex_unlock(&Receiver_pthreadMutex);
        if (List_count(Receiver_list) >= 0 && List_count(Receiver_list) < LISTSIZE)
        {
            pthread_cond_signal(&Receiver_pthreadReceiveCondVar);
        }
        msg[sizeof(msg) - 1] = '\0';
        fputs(msg, stdout);
    }
    return NULL;
}

// Inits
void Sender_init(char *addressR, int portR, int portS)
{
    Sender_list = List_create();
    Receiver_address = addressR;
    Receiver_port = portR;
    Sender_port = portS;
    pthread_create(&Sender_keyboardThread, NULL, Sender_keyboardThreadFn, NULL);
    pthread_create(&Sender_sendThread, NULL, Sender_sendThreadFn, NULL);
}
void Receiver_init(int portS)
{
    Sender_port = portS;
    Receiver_list = List_create();
    pthread_create(&Receiver_receiveThread, NULL, Receiver_receiveThreadFn, NULL);
    pthread_create(&Receiver_printThread, NULL, Receiver_printThreadFn, NULL);
}

// Shutdowns
void Sender_shutdown()
{
    pthread_join(Sender_keyboardThread, NULL);
    pthread_join(Sender_sendThread, NULL);
}
void Receiver_shutdown()
{
    pthread_join(Receiver_receiveThread, NULL);
    pthread_join(Receiver_printThread, NULL);
}
void Threads_init(char *addressR, int portR, int portS)
{
    Sender_init(addressR, portR, portS);
    Receiver_init(portS);
}