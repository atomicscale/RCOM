#include "app.h"
#include "establishment.h"
#include "data_transfer.h"
#include "termination.h"

#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

struct termios oldtio;
int duplicate = FALSE;


/*
 * Open serial port device for reading 
 */
int llopen(Settings* structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar) {
	struct termios newtio;

	/* Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C. */
	int fd = open(structDados->port, O_RDWR | O_NOCTTY);
	if (fd < 0) { perror(structDados->port); exit(-1); }

	if (tcgetattr(fd, &oldtio) == -1) { // save current port settings
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = structDados->baudRate | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 10; // inter-character timer
	newtio.c_cc[VMIN] = 0; // blocking read until x chars received

	/* VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) próximo(s) caracter(es) */

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	printf("New termios structure set\n");

	if (structDados->sender == TRUE)
		sendSET(fd, structDados, estado, tentativaEnvio, podeEnviar);
	else if (structDados->sender == FALSE)
		sendUA(fd, structDados, estado);

	return fd;
}


/*
 *	
 */
int llwrite(Settings* structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar) {
	int Ns = 0;
	unsigned int i = 0;

	unsigned char CTRL_START[11 + strlen(structDados->fileName) + 1];
	CTRL_START[i++] = 0x01; // Indica início 
	CTRL_START[i++] = 0x00; // A enviar tamanho do ficheiro
	CTRL_START[i++] = 0x02; // 2 bytes
	CTRL_START[i++] = (unsigned char)((structDados->filesize & 0xff00) >> 8); // Tamanho do ficheiro
	CTRL_START[i++] = (unsigned char)(structDados->filesize & 0xff);
	CTRL_START[i++] = 0x01; // A enviar nome do ficheiro
	CTRL_START[i++] = strlen(structDados->fileName) + 1; // strlen(structDados.fileName) + 1 bytes
	int j;
	for (j = 0; j < strlen(structDados->fileName) + 1; j++) {
		CTRL_START[i++] = structDados->fileName[j];
	}
	CTRL_START[i++] = 0x02; // A enviar comprimento máximo do campo de dados das tramas I
	CTRL_START[i++] = 0x02; // 2 bytes
	CTRL_START[i++] = (structDados->maxSize >> 8) & 0xff;
	CTRL_START[i++] = structDados->maxSize & 0xff;

	if (linkwrite(CTRL_START,structDados, 11 + strlen(structDados->fileName) + 1, Ns, estado, tentativaEnvio, podeEnviar) < 0) return -1;
	Ns = (Ns + 1) % 2;

	int resRead;
	int numPack = 0;
	int numWritten = 0;

	do {
		unsigned char DATA_PACK[structDados->maxSize];
		DATA_PACK[0] = 0x00; // Indica dados //
		DATA_PACK[1] = numPack++ % 256; // Número de sequência
		resRead = fread(DATA_PACK + 4, sizeof(unsigned char), structDados->maxSize - 4, structDados->fp);
		DATA_PACK[2] = (resRead >> 8) & 0xff;
		DATA_PACK[3] = resRead & 0xff;

		if (linkwrite(DATA_PACK,structDados, 4 + resRead, Ns, estado, tentativaEnvio, podeEnviar) < 0) return -1;
		Ns = (Ns + 1) % 2;
	} while (resRead);

	unsigned char CTRL_STOP[1];
	CTRL_STOP[0] = 0x02; // Indica paragem //
	if (linkwrite(CTRL_STOP,structDados, 1, Ns, estado, tentativaEnvio, podeEnviar) < 0) return -1;
	Ns = (Ns + 1) % 2;

	fclose(structDados->fp);

	return numWritten; // Número de bytes escritos
}


/*
 *	
 */
int llread(Settings* structDados) {
	int Nr = 0;
	int numread = 0;
	unsigned char RR[5];
	RR[0] = FLAG;
	RR[1] = A;
	RR[4] = FLAG;

	unsigned char REJ[5];
	REJ[0] = FLAG;
	REJ[1] = A;
	REJ[4] = FLAG;

	do {
		unsigned char dataPackage[structDados->maxSize];

		int packageSize = linkread(dataPackage,structDados, duplicate);

		if (packageSize == -1) {
			REJ[2] = (Nr == 0) ? 0x05 : 0x25;
			REJ[3] = REJ[1] ^ REJ[2];
			tcflush(structDados->fd, TCIFLUSH);
			write(structDados->fd, REJ, 5);
		}
		else if (dataPackage[0] == DATA) { //
			int posPackage = 0;
			if (duplicate) printf("Duplicate frame!\n");
			while (posPackage < packageSize && !duplicate) {
				fwrite(dataPackage + 4 + posPackage, sizeof(unsigned char), 1, structDados->fp);
				posPackage++;
				numread++;
			}
			if (!duplicate) Nr = (Nr + 1) % 2;
			duplicate = FALSE;
			RR[2] = (Nr == 0) ? 0x01 : 0x21;//
			RR[3] = RR[1] ^ RR[2];
			write(structDados->fd, RR, 5);
		}
		else if (dataPackage[0] == START) { //
			unsigned int i = 1;
			unsigned int j;

			if (dataPackage[i++] == 0x00 && dataPackage[i++] == 0x02) {
				structDados->filesize = dataPackage[i++] << 8;
				structDados->filesize += dataPackage[i++];
			}

			if (dataPackage[i++] == 0x01) {
				unsigned int size = dataPackage[i++];
				for (j = 0; j < size; j++) {
					structDados->fileName[j] = dataPackage[i++];
					printf("%c", structDados->fileName[j]);
				}
				printf("\n");
			}

			structDados->fp = fopen(structDados->fileName, "wb");

			if (dataPackage[i++] == 0x02 && dataPackage[i++] == 0x02) {
				structDados->maxSize = dataPackage[i++] << 8;
				structDados->maxSize += dataPackage[i++];
			}

			Nr = (Nr + 1) % 2;
			RR[2] = (Nr == 0) ? 0x01 : 0x21;//
			RR[3] = RR[1] ^ RR[2];
			write(structDados->fd, RR, 5);
		}
		else if (dataPackage[0] == END) { //
			Nr = (Nr + 1) % 2;
			RR[2] = (Nr == 0) ? 0x01 : 0x21;//
			RR[3] = RR[1] ^ RR[2];
			write(structDados->fd, RR, 5);
			break;
		}
	} while (TRUE);

	fclose(structDados->fp);
	
	FILE* filetest = fopen(structDados->fileName, "rb");
	fseek(filetest, 0, SEEK_END);
	if ((structDados->filesize = ftell(structDados->fp)))
		printf("\nFile sizes match!\n\n");
	else
		printf("\nFile sizes don't match!\n\n");

	return numread;
}

/*
 *	
 */
int llclose(Settings* structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar) {
	tentativaEnvio = 1;
	estado = 0;

	unsigned char DISC[5]; // Trama DISC (disconnect)
	DISC[0] = FLAG;
	DISC[1] = A;
	DISC[2] = 0x0B;
	DISC[3] = A ^ 0x0B;
	DISC[4] = FLAG;

	if (structDados->sender == TRUE)
		senderDISC(DISC, structDados, estado, tentativaEnvio, podeEnviar);
	else if (structDados->sender == FALSE)
		receiverDISC(DISC, structDados, estado);

	if (tcsetattr(structDados->fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	printf("Closing port...\n");
	close(structDados->fd);

	return 0;
}
