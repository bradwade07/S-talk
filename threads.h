#ifndef _THREADS_H_
#define _THREADS_H_

void *Sender_sendThreadFn(void *unused);
void *Sender_keyboardThreadFn(void *unused);
void Sender_init(char *addressR, int portR, int portS);
void Sender_shutdown();
void *Receiver_receiveThreadFn(void *unused);
void *Receiver_printThreadFn(void *unused);
void Receiver_init(int portS);
void Receiver_shutdown();
void Threads_init(char *addressR, int portR, int portS);
#endif