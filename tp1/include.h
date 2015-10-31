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

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03
	
void prepareSender();					/*  Initialize The Sender Port */

void prepareReceiver();					/*  Initialize The Receiver Port */

void startstruct();						/*  Initialize the type of transmission */

void atendeAlarme();					/*  Function to control the conection */

