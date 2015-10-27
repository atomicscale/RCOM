#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define _POSIX_SOURCE 	1 /* POSIX compliant source */
#define FALSE 			0
#define TRUE 			1
#define FLAG 			0x7e
#define A 				0x03


void initalizeSender();
void initializeReceiver();
void intializeTypeOfTransmission();
void callAlarm();
int llopen();
void sendSET(int fd);
void sendUA(int fd);
int llwrite();
int linkwrite(unsigned char*, int, int);
int llread();
int linkread(unsigned char*);
int llclose();
void senderDISC(unsigned char*);
void receiverDISC(unsigned char*);

