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


/*
 *	
 */
void senderDISC(unsigned char* DISC, Settings structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar) {
	tentativaEnvio = 1;
	podeEnviar = TRUE;
	estado = 0;

	unsigned char c;
	unsigned char buf[5];
	(void) buf;

	while (tentativaEnvio <= structDados.numTransmissions) {
		if (podeEnviar) {
			podeEnviar = FALSE;
			alarm(structDados.timeout);
			int res = write(structDados.fd, DISC, 5);

			if (res == 5) printf("Sent DISC\n");
			else {
				printf("Unable to send DISC\n");
				podeEnviar = TRUE;
			}

			alarm(structDados.timeout);

			while (estado != 5) {
				if (podeEnviar) {
					alarm(0);
					break;
				}

				read(structDados.fd, &c, 1);

				switch (estado) {
				case 0:
					if (c == FLAG) {
						estado = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					}
					else printf("DISC: Remaining on state 0\n");
					break;
				case 1:
					if (c == A) {
						estado = 2;
						printf("DISC: Switching to state 2\n");
						buf[1] = c;
					}
					else if (c == FLAG) {
						estado = 1;
						printf("DISC: Remaining on state 1\n");
						buf[0] = c;
					}
					else {
						estado = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 2:
					if (c == 0x0B) {
						estado = 3;
						printf("DISC: Switching to state 3\n");
						buf[2] = c;
					}
					else if (c == FLAG) {
						estado = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						estado = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 3:
					if (c == (A ^ 0x0B)) {
						estado = 4;
						printf("DISC: Switching to state 4\n");
						buf[3] = c;
					}
					else if (c == FLAG) {
						estado = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						estado = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 4:
					if (c == FLAG) {
						alarm(0);
						estado = 5;
						printf("DISC: Switching to state 5\n");
						buf[4] = c;
						tentativaEnvio = structDados.numTransmissions + 1; // Obrigar a sair do ciclo
					}
					else {
						estado = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				}
			}
		}
	}

	unsigned char UA[5]; // Trama UA (unnumbered acknowledgement)
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = 0x03; //
	UA[3] = A ^ 0x03; //
	UA[4] = FLAG;

	write(structDados.fd, UA, 5);
	sleep(1);
}


/*
 *	
 */
void receiverDISC(unsigned char* DISC, Settings structDados, volatile int estado) {
	unsigned char c;
	unsigned char buf[5];
	(void) buf;

	while (estado != 5) {
		read(structDados.fd, &c, 1);

		switch (estado) {
		case 0:
			if (c == FLAG) {
				estado = 1;
				printf("DISC: Switching to state 1\n");
				buf[0] = c;
			}
			else printf("DISC: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {

				estado = 2;
				printf("DISC: Switching to state 2\n");
				buf[1] = c;
			}
			else if (c == FLAG) {
				estado = 1;
				printf("DISC: Remaining on state 1\n");
				buf[0] = c;
			}
			else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x0B) {
				estado = 3;
				printf("DISC: Switching to state 3\n");
				buf[2] = c;
			}
			else if (c == FLAG) {
				estado = 1;
				printf("DISC: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x0B)) {
				estado = 4;
				printf("DISC: Switching to state 4\n");
				buf[3] = c;
			}
			else if (c == FLAG) {
				estado = 1;
				printf("  DISC: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				estado = 5;
				printf("DISC: Switching to state 5\n");
				buf[4] = c;
			}
			else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		}
	}

	write(structDados.fd, DISC, 5);

	estado = 0;

	while (estado != 5) {
		read(structDados.fd, &c, 1);

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
			if (c == (A ^ 0x03)) { //
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
				estado = 5;
				printf("UA: Switching to state 5\n");
				buf[4] = c;
			}
			else {
				estado = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		}
	}
}