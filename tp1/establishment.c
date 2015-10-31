#include "establishment.h"

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

/*
 * Function that send the SET-TRAMA
 */
void sendSET(int fd, Settings* structDados, int estado, int tentativaEnvio, int podeEnviar) {
	unsigned char SET[5]; // Trama SET
	SET[0] = FLAG;
	SET[1] = A;
	SET[2] = 0x07; //
	SET[3] = SET[1] ^ SET[2];
	SET[4] = FLAG;

	unsigned char c;
	unsigned char buf[5];
	(void) buf;
	int passed = FALSE;

	while (tentativaEnvio <= structDados->numTransmissions && passed == FALSE) {
		if (podeEnviar) {
			podeEnviar = FALSE;
			alarm(structDados->timeout);
			int res = write(fd, SET, 5);

			if (res == 5) printf("Frame sent successfully\n");
			else {
				printf("Frame wasn't sent successfully\n");
				podeEnviar = TRUE;
			}

			alarm(structDados->timeout);

			while (estado != 5) {
				if (podeEnviar) {
					alarm(0);
					break;
				}

				read(fd, &c, 1);

				switch (estado) {
				case 0:
					if (c == FLAG) {
						estado = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					}
					else printf("UA: Remaining on state 0\n");
					break;
				case 1:
					if (c == A) {
						estado = 2;
						printf("UA: Switching to state 2\n");
						buf[1] = c;
					}
					else if (c == FLAG) {
						estado = 1;
						printf("UA: Remaining on state 1\n");
						buf[0] = c;
					}
					else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 2:
					if (c == 0x03) { //
						estado = 3;
						printf("UA: Switching to state 3\n");
						buf[2] = c;
					}
					else if (c == FLAG) {
						estado = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 3:
					if (c == (A ^ 0x03)) {  //
						estado = 4;
						printf("UA: Switching to state 4\n");
						buf[3] = c;
					}
					else if (c == FLAG) {
						estado = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 4:
					if (c == FLAG) {
						alarm(0);
						estado = 5;
						printf("UA: Switching to state 5\n");
						buf[4] = c;
						passed = TRUE;
					}
					else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				}
			}
			
		}
	}
	if (tentativaEnvio > structDados->numTransmissions) exit(-1);
}


/*
 * Function that send the UA response
 */
void sendUA(int fd, Settings* structDados, int estado) {
	unsigned char UA[5]; // Trama UA (unnumbered acknowledgement)
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = 0x03; //
	UA[3] = A ^ 0x03; //
	UA[4] = FLAG;

	unsigned char c;
	unsigned char buf[5];
	(void) buf;

	while (estado != 5) {
		read(fd, &c, 1);

		switch (estado) {
		case 0:
			if (c == FLAG) {
				estado = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			}
			else printf("SET: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {
				estado = 2;
				printf("SET: Switching to state 2\n");
				buf[1] = c;
			}
			else if (c == FLAG) {
				estado = 1;
				printf("SET: Remaining on state 1\n");
				buf[0] = c;
			}
			else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x07) { //
				estado = 3;
				printf("SET: Switching to state 3\n");
				buf[2] = c;
			}
			else if (c == FLAG) {
				estado = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x07)) { //
				estado = 4;
				printf("SET: Switching to state 4\n");
				buf[3] = c;
			}
			else if (c == FLAG) {
				estado = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				estado = 5;
				printf("SET: Switching to state 5\n");
				buf[4] = c;
			}
			else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		}
	}

	write(fd, UA, 5);
}

