#include <stdio.h>

/*
 * Transmission Information
 */
typedef struct DataSettings {
	int fd;
	FILE* fp;
	char fileName[255];
	int filesize;
	int sender; 	// TRUE ou FALSE
	char port[20]; 	// Device /dev/ttySx
	int baudRate; 	// Transmission Speed
	unsigned int timeout; 	// Timer: 1s
	unsigned int maxTransmissions; // Maximum of failed attempts
	unsigned int maxSize; // Size of each frame
};