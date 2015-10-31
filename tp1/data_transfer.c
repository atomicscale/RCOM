#include "data_transfer.h"

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

unsigned char lastN = 255;


/*
 *	
 */
int linkwrite(unsigned char* data, Settings structDados, int datasize, int Ns, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar) {
	tentativaEnvio = 1;
	podeEnviar = TRUE;
	estado = 0;

	unsigned int aSubstituir = 0;
	unsigned char FRAME_I[6 + datasize];
	FRAME_I[0] = FLAG;
	FRAME_I[1] = A;
	FRAME_I[2] = Ns == 0 ? 0x00 : 0x20;    //
	FRAME_I[3] = FRAME_I[1] ^ FRAME_I[2];
	unsigned char BCC2 = 0x00;
	int i;
	for (i = 0; i < datasize; i++) {
		FRAME_I[4 + i] = data[i];
		BCC2 ^= data[i];
		if (FRAME_I[4 + i] == 0x7e || FRAME_I[4 + i] == 0x7d) aSubstituir++;
	}
	FRAME_I[4 + datasize] = BCC2;
	if (FRAME_I[4 + datasize] == 0x7e || FRAME_I[4 + datasize] == 0x7d) aSubstituir++;
	FRAME_I[5 + datasize] = FLAG;

	unsigned char FRAME_I_FINAL[6 + datasize + aSubstituir];
	int j;
	for (i = 0, j = 0; i < 6 + datasize + aSubstituir; i++, j++) {
		if (j < 4 || j == 6 + datasize - 1) FRAME_I_FINAL[i] = FRAME_I[j];
		else if (FRAME_I[j] == 0x7e || FRAME_I[j] == 0x7d) {
			FRAME_I_FINAL[i++] = 0x7d;
			FRAME_I_FINAL[i] = 0x20 ^ FRAME_I[j];
		}
		else FRAME_I_FINAL[i] = FRAME_I[j];
	}

	int OK = FALSE; // Recebeu RR corretamente?
	while (tentativaEnvio <= structDados.numTransmissions) {
		if (OK) break;
		else if (podeEnviar) {
			printf("Sending frame: ");
			for (i = 0; i < 6 + datasize + aSubstituir; i++)
				printf("%d ", FRAME_I_FINAL[i]);
			printf("\n");
			tcflush(structDados.fd, TCIFLUSH);
			write(structDados.fd, FRAME_I_FINAL, 6 + datasize + aSubstituir);
			alarm(structDados.timeout);
			podeEnviar = FALSE;
		}

		unsigned char c;
		int res = read(structDados.fd, &c, 1);

		if (res != 0) {
			alarm(0);

			switch (estado) {
			case 0:
				if (c == FLAG) {
					estado = 1;
					printf("Confirmation: Switching to state 1\n");
				}
				else printf("Confirmation: Remaining on state 0\n");
				break;
			case 1:
				if (c == A) {
					estado = 2;
					printf("Confirmation: Switching to state 2\n");
				}
				else if (c == FLAG)
					printf("Confirmation: Remaining on state 1\n");
				else {
					estado = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			case 2:
				if (c == (Ns == 0 ? 0x21 : 0x01)) { //
					estado = 3;
					printf("Confirmation: positive. Switching to state 3\n");
				}
				else if (c == 0x05 || c == 0x25) { //
					podeEnviar = TRUE;
					estado = 0;
					printf("Confirmation: negative. Resending...\n");
				}
				else if (c == FLAG) {
					estado = 1;
					printf("Confirmation: Switching to state 1\n");
				}
				else {
					estado = 0;
					printf("Confirmation: Switching 2 state 0\n");
				}
				break;
			case 3:
				if (c == (A ^ (Ns == 0 ? 0x21 : 0x01))) { //
					estado = 4;
					printf("Confirmation: Switching to state 4\n");
				}
				else if (c == FLAG) {
					estado = 1;
					printf("Confirmation: Switching to state 1\n");
				}
				else {
					estado = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			case 4:
				if (c == FLAG) {
					estado = 5;
					printf("Confirmation: Switching to state 5\n");
					OK = TRUE; // Sair do ciclo
				}
				else {
					estado = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			}
		}
	}

	return tentativaEnvio <= structDados.numTransmissions ? 0 : -1;
}



/*
 *	
 */
int linkread(unsigned char* dataPackage, Settings structDados, int duplicate) {
	unsigned char c;
	unsigned char buf[structDados.maxSize + 6];
	int estado = 0;
	unsigned char bcc2 = 0x00;
	int packagesLength = 0;
	int i = 0;
	unsigned char Controlo;
	unsigned char N;

	int wasREJsent = FALSE;

	while (estado != 7 && wasREJsent == FALSE) {
		read(structDados.fd, &c, 1);

		switch (estado) {
		case 0:
			if (c == FLAG)
				buf[estado++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 1:
			if (c == A)
				buf[estado++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 2:
			if (c == 0x00 || c == 0x20) //
				buf[estado++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 3:
			if (c == (buf[1] ^ buf[2]))
				buf[estado++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 4:
		{
			int megaEstado = 0;
			if (c == 0x00) {
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // C
				Controlo = c;

				read(structDados.fd, &c, 1);
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // N
				printf("N = %d\n", c);
				N = c + 1;

				read(structDados.fd, &c, 1);
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // L2      
				read(structDados.fd, &c, 1);
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // L1    
				packagesLength = 256 * dataPackage[2] + dataPackage[3];
				i = packagesLength;
				while (i > 0) {
					read(structDados.fd, &c, 1);
					if (c == 0x7d) {
						read(structDados.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
					i--;
				}
				estado = 5;
			}
			else if (c == 0x01) {
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(structDados.fd, &c, 1); // T1
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(structDados.fd, &c, 1); // L1
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado] = c;

				for (i = dataPackage[megaEstado++]; i > 0; i--) {
					read(structDados.fd, &c, 1); // V1
					if (c == 0x7d) {
						read(structDados.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
				}

				read(structDados.fd, &c, 1); // T2
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(structDados.fd, &c, 1); // L2
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado] = c;

				for (i = dataPackage[megaEstado++]; i > 0; i--) {
					read(structDados.fd, &c, 1); // V2
					if (c == 0x7d) {
						read(structDados.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
				}

				read(structDados.fd, &c, 1); // T3
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(structDados.fd, &c, 1); // L3
				if (c == 0x7d) {
					read(structDados.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado] = c;

				for (i = dataPackage[megaEstado++]; i > 0; i--) {
					read(structDados.fd, &c, 1); // V3
					if (c == 0x7d) {
						read(structDados.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
				}

				estado = 5;
			}
			else if (c == 0x02) {
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;
				estado = 5;
			}
			else
				wasREJsent = TRUE;
			break;
		}
		case 5:
			printf("bcc2 esperado: %d, obtido: %d\n", bcc2, c);
			if (c == 0x7d) {
				read(structDados.fd, &c, 1);
				c ^= 0x20;
				if (c == bcc2)
					estado = 6;
				else
					wasREJsent = TRUE;
			}
			else if (c == bcc2)
				estado = 6;
			else
				wasREJsent = TRUE;
			break;
		case 6:
			if (c == FLAG)
				estado = 7;
			else
				wasREJsent = TRUE;
			break;
		}
	}

	printf("WasRejSent: %d (bcc2 = %d)\n", wasREJsent, bcc2);
	if (wasREJsent) return -1;

	if (Controlo == 0x00) { // Se foi pacote de dados...
		if (N == lastN) duplicate = TRUE;
		lastN = N; 
	}

	return packagesLength;
}