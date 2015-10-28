/*
 * Some usefull variables for the assigment
 */
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define _POSIX_SOURCE 	1 	/* POSIX compliant source */
#define FALSE 			0   
#define TRUE 			1
#define FLAG 			0x7e
#define A 				0x03

void initializeSender();				/*  Initialize The Sender Port */

void initialzeReceiver();         		/*  Initialize The Receiver Port */

void initializeTypeOfTransmission();	/*  Initialize the type of transmission */

void callAlarm();						/*  Function to control the conection */

int llopen();							/*  Open The Serial Port */

void sendSET(int fd);					/* 	Function to send the SET-TRAMA */

void sendUA(int fd);					/* 	Function to send the UA-TRAMA */

int llwrite();							/* 	Writting to the serial Port*/

int linkwrite(unsigned char*, int, int); /* Auxiliary function to write to the port */

int llread();							/*  Reading from the serial port */

int linkread(unsigned char*);			/* 	Auxiliary function to read to the port*/

int llclose();							/* 	Closing the port */

void senderDISC(unsigned char*);        /* 	Send the DIS-TRAMA */

void receiverDISC(unsigned char*);		/* 	Receive the DIS-TRAMA */

