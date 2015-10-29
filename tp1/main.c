/* Non-Canonical Input Processing */

#include "include.h"
#include "interface.h"

/*  Usefull variables to controle the packages */
volatile int estado = 0, tentativaEnvio = 1, podeEnviar = TRUE;
struct termios oldtio;

/*
 * Struct that allows us saving all the information about the transmission
 */
typedef struct UserSettings {

	int fd;
	FILE* fp;
	char fileName[255];
	int filesize;
	int sender; // TRUE ou FALSE
	char port[20]; // Dispositivo /dev/ttySx
	int baudRate; // Velocidade de transmissão
	unsigned int timeout; // Valor do temporizador: 1 s
	unsigned int numTransmissions; // Número de tentativas em caso de falha
	unsigned int maxSize;

} Settings;

static Settings structDados;

unsigned char lastN = 255;
int duplicate = FALSE;

/*
 *	Function that clear all the screen
 */
void limparEcra() {
	unsigned int i;
	for (i = 0; i < 50; i++)
		printf("\n");
}


/*
 * The main function that runs all the program
 */
int main(int argc, char** argv) {

	signal(SIGALRM, atendeAlarme); /* Instalar alarme */
	setvbuf(stdout, NULL, _IONBF, 0); /* Desativar buffer do STDOUT */

	do {
<<<<<<< HEAD

		int choice = InitialMenu();
=======
		
		limparEcra();
		
		printf("-------------------------\n");
		printf("- SERIAL PORT RCOM 1415 -\n");
		printf("-------------------------\n");
		printf("- 1. Run   			    -\n");
		printf("- 2. Exit               -\n");
		printf("-------------------------\n");
		
		int choice;
		scanf("%d", &choice);
>>>>>>> origin/master

		switch (choice) {
		case 1:
			startstruct();
			structDados.fd = llopen();

			if (structDados.sender)
				prepareSender();
			else
				prepareReceiver();

			llclose(structDados.fd);
			break;
		case 2:
			printf("Exiting program! \n");
			break;
		default:
			continue;
		}

	} while (TRUE);

	return 0;
}

/*
 * Prepare the port to send the packages
 */
void prepareSender() {
	limparEcra();

	if ((structDados.fp = fopen(structDados.fileName, "rb")) != NULL) {
		fseek(structDados.fp, 0, SEEK_END);
		structDados.filesize = ftell(structDados.fp);
		rewind(structDados.fp);
	} else {
		printf("File not found!\n");
		exit(-1);
	}

	printf("Sending file...\n");

	llwrite();
}

/*
 * Prepare the port to receive the packages
 */
void prepareReceiver() {
	limparEcra();
	printf("Receiving file...\n");

	unsigned char* buf = NULL;
	llread(structDados.fd, buf);
}

/*
 * Get the information about the transmission
 */
void startstruct() {
	int choicePort = -1;

	do {

		char choice = SenderOrReceiver();

		if (choice == '1') {
			structDados.sender = TRUE;
			break;
		} else if (choice == '2') {
			structDados.sender = FALSE;
			break;
		} else
			continue;

	} while (TRUE);

	if (structDados.sender) {
		structDados.fileName = getFileName();
	}

	int port = getPort();
	snprintf(structDados.port, sizeof(structDados.port), "/dev/ttyS%d", port);

	do {

		int choice = getBaudRate();

		if (choice == 1) {
			structDados.baudRate = B9600;
			break;
		} else if (choice == 2) {
			structDados.baudRate = B19200;
			break;
		} else if (choice == 3) {
			structDados.baudRate = B38400;
			break;
		} else if (choice == 4) {
			structDados.baudRate = B57600;
			break;
		} else if (choice == 5) {
			structDados.baudRate = B115200;
			break;
		} else
			continue;

	} while (TRUE);

	structDados.timeout = getTimeOut();

	structDados.numTransmissions = getMaxTrans();

	if (structDados.sender) {
		structDados.maxSize = getMaxPackages();
		structDados.maxSize += 4;
	} else
		structDados.maxSize = 255;
}

/*
 * Open serial port device for reading 
 */
int llopen() {
	struct termios newtio;

	/* Open serial port device for reading and writing and not as controlling tty
	 because we don't want to get killed if linenoise sends CTRL-C. */
	int fd = open(structDados.port, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror(structDados.port);
		exit(-1);
	}

	if (tcgetattr(fd, &oldtio) == -1) { // save current port settings
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = structDados.baudRate | CS8 | CLOCAL | CREAD;
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

	if (structDados.sender == TRUE)
		sendSET(fd);
	else if (structDados.sender == FALSE)
		sendUA(fd);

	return fd;
}

/*
 * Function that send the SET-TRAMA
 */
void sendSET(int fd) {
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

	while (tentativaEnvio <= structDados.numTransmissions && passed == FALSE) {
		if (podeEnviar) {
			podeEnviar = FALSE;
			alarm(structDados.timeout);
			int res = write(fd, SET, 5);

			if (res == 5)
				printf("Frame sent successfully\n");
			else {
				printf("Frame wasn't sent successfully\n");
				podeEnviar = TRUE;
			}

			alarm(structDados.timeout);

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
					} else
						printf("UA: Remaining on state 0\n");
					break;
				case 1:
					if (c == A) {
						estado = 2;
						printf("UA: Switching to state 2\n");
						buf[1] = c;
					} else if (c == FLAG) {
						estado = 1;
						printf("UA: Remaining on state 1\n");
						buf[0] = c;
					} else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 2:
					if (c == 0x03) { //
						estado = 3;
						printf("UA: Switching to state 3\n");
						buf[2] = c;
					} else if (c == FLAG) {
						estado = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					} else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 3:
					if (c == (A ^ 0x03)) {  //
						estado = 4;
						printf("UA: Switching to state 4\n");
						buf[3] = c;
					} else if (c == FLAG) {
						estado = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					} else {
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
					} else {
						estado = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				}
			}
			//sleep(1);
		}
	}
	if (tentativaEnvio > structDados.numTransmissions)
		exit(-1);
}

/*
 * Function that send the UA response
 */
void sendUA(int fd) {
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
			} else
				printf("SET: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {
				estado = 2;
				printf("SET: Switching to state 2\n");
				buf[1] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("SET: Remaining on state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x07) { //
				estado = 3;
				printf("SET: Switching to state 3\n");
				buf[2] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x07)) { //
				estado = 4;
				printf("SET: Switching to state 4\n");
				buf[3] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				estado = 5;
				printf("SET: Switching to state 5\n");
				buf[4] = c;
			} else {
				estado = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		}
	}

	write(fd, UA, 5);
}

/*
 *	
 */
void atendeAlarme() {
	printf("Bad response #%d from receiver!\n", tentativaEnvio);
	tentativaEnvio++;
	podeEnviar = TRUE;
	estado = 0;
}

/*
 *	
 */
int llwrite() {
	int Ns = 0;
	unsigned int i = 0;

	unsigned char CTRL_START[11 + strlen(structDados.fileName) + 1];
	CTRL_START[i++] = 0x01; // Indica início 
	CTRL_START[i++] = 0x00; // A enviar tamanho do ficheiro
	CTRL_START[i++] = 0x02; // 2 bytes
	CTRL_START[i++] = (unsigned char) ((structDados.filesize & 0xff00) >> 8); // Tamanho do ficheiro
	CTRL_START[i++] = (unsigned char) (structDados.filesize & 0xff);
	CTRL_START[i++] = 0x01; // A enviar nome do ficheiro
	CTRL_START[i++] = strlen(structDados.fileName) + 1; // strlen(structDados.fileName) + 1 bytes
	int j;
	for (j = 0; j < strlen(structDados.fileName) + 1; j++) {
		CTRL_START[i++] = structDados.fileName[j];
	}
	CTRL_START[i++] = 0x02; // A enviar comprimento máximo do campo de dados das tramas I
	CTRL_START[i++] = 0x02; // 2 bytes
	CTRL_START[i++] = (structDados.maxSize >> 8) & 0xff;
	CTRL_START[i++] = structDados.maxSize & 0xff;

	if (linkwrite(CTRL_START, 11 + strlen(structDados.fileName) + 1, Ns) < 0)
		return -1;
	Ns = (Ns + 1) % 2;

	int resRead;
	int numPack = 0;
	int numWritten = 0;

	do {
		unsigned char DATA_PACK[structDados.maxSize];
		DATA_PACK[0] = 0x00; // Indica dados //
		DATA_PACK[1] = numPack++ % 256; // Número de sequência
		resRead = fread(DATA_PACK + 4, sizeof(unsigned char),
				structDados.maxSize - 4, structDados.fp);
		DATA_PACK[2] = (resRead >> 8) & 0xff;
		DATA_PACK[3] = resRead & 0xff;

		if (linkwrite(DATA_PACK, 4 + resRead, Ns) < 0)
			return -1;
		Ns = (Ns + 1) % 2;
	} while (resRead);

	unsigned char CTRL_STOP[1];
	CTRL_STOP[0] = 0x02; // Indica paragem //
	if (linkwrite(CTRL_STOP, 1, Ns) < 0)
		return -1;
	Ns = (Ns + 1) % 2;

	fclose(structDados.fp);

	return numWritten; // Número de bytes escritos
}

/*
 *	
 */
int linkwrite(unsigned char* data, int datasize, int Ns) {
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
		if (FRAME_I[4 + i] == 0x7e || FRAME_I[4 + i] == 0x7d)
			aSubstituir++;
	}
	FRAME_I[4 + datasize] = BCC2;
	if (FRAME_I[4 + datasize] == 0x7e || FRAME_I[4 + datasize] == 0x7d)
		aSubstituir++;
	FRAME_I[5 + datasize] = FLAG;

	unsigned char FRAME_I_FINAL[6 + datasize + aSubstituir];
	int j;
	for (i = 0, j = 0; i < 6 + datasize + aSubstituir; i++, j++) {
		if (j < 4 || j == 6 + datasize - 1)
			FRAME_I_FINAL[i] = FRAME_I[j];
		else if (FRAME_I[j] == 0x7e || FRAME_I[j] == 0x7d) {
			FRAME_I_FINAL[i++] = 0x7d;
			FRAME_I_FINAL[i] = 0x20 ^ FRAME_I[j];
		} else
			FRAME_I_FINAL[i] = FRAME_I[j];
	}

	int OK = FALSE; // Recebeu RR corretamente?
	while (tentativaEnvio <= structDados.numTransmissions) {
		if (OK)
			break;
		else if (podeEnviar) {
			printf("Sending frame: ");
			for (i = 0; i < 6 + datasize + aSubstituir; i++)
				printf("%d ", FRAME_I_FINAL[i]);
			printf("\n");
			//tcflush(structDados.fd, TCIFLUSH);
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
				} else
					printf("Confirmation: Remaining on state 0\n");
				break;
			case 1:
				if (c == A) {
					estado = 2;
					printf("Confirmation: Switching to state 2\n");
				} else if (c == FLAG)
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
				} else if (c == 0x05 || c == 0x25) { //
					podeEnviar = TRUE;
					estado = 0;
					printf("Confirmation: negative. Resending...\n");
				} else if (c == FLAG) {
					estado = 1;
					printf("Confirmation: Switching to state 1\n");
				} else {
					estado = 0;
					printf("Confirmation: Switching 2 state 0\n");
				}
				break;
			case 3:
				if (c == (A ^ (Ns == 0 ? 0x21 : 0x01))) { //
					estado = 4;
					printf("Confirmation: Switching to state 4\n");
				} else if (c == FLAG) {
					estado = 1;
					printf("Confirmation: Switching to state 1\n");
				} else {
					estado = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			case 4:
				if (c == FLAG) {
					estado = 5;
					printf("Confirmation: Switching to state 5\n");
					OK = TRUE; // Sair do ciclo
				} else {
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
int llread() {
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
		unsigned char dataPackage[structDados.maxSize];

		int packageSize = linkread(dataPackage);

		if (packageSize == -1) {
			REJ[2] = (Nr == 0) ? 0x05 : 0x25;
			REJ[3] = REJ[1] ^ REJ[2];
			tcflush(structDados.fd, TCIFLUSH);
			write(structDados.fd, REJ, 5);
		} else if (dataPackage[0] == 0x00) { //
			int posPackage = 0;
			if (duplicate)
				printf("Duplicate frame!\n");

			while (posPackage < packageSize && !duplicate) {
				fwrite(dataPackage + 4 + posPackage, sizeof(unsigned char), 1,
						structDados.fp);
				posPackage++;
				numread++;
			}

			if (!duplicate)
				Nr = (Nr + 1) % 2;

			duplicate = FALSE;
			RR[2] = (Nr == 0) ? 0x01 : 0x21; //
			RR[3] = RR[1] ^ RR[2];
			write(structDados.fd, RR, 5);

		} else if (dataPackage[0] == 0x01) { //
			unsigned int i = 1;
			unsigned int j;

			if (dataPackage[i++] == 0x00 && dataPackage[i++] == 0x02) {
				structDados.filesize = dataPackage[i++] << 8;
				structDados.filesize += dataPackage[i++];
			}

			if (dataPackage[i++] == 0x01) {
				unsigned int size = dataPackage[i++];
				for (j = 0; j < size; j++) {
					structDados.fileName[j] = dataPackage[i++];
					printf("%c", structDados.fileName[j]);
				}
				printf("\n");
			}

			structDados.fp = fopen(structDados.fileName, "wb");

			if (dataPackage[i++] == 0x02 && dataPackage[i++] == 0x02) {
				structDados.maxSize = dataPackage[i++] << 8;
				structDados.maxSize += dataPackage[i++];
			}

			Nr = (Nr + 1) % 2;
			RR[2] = (Nr == 0) ? 0x01 : 0x21; //
			RR[3] = RR[1] ^ RR[2];
			write(structDados.fd, RR, 5);

		} else if (dataPackage[0] == 0x02) { //

			Nr = (Nr + 1) % 2;
			RR[2] = (Nr == 0) ? 0x01 : 0x21; //
			RR[3] = RR[1] ^ RR[2];
			write(structDados.fd, RR, 5);
			break;

		}

	} while (TRUE);

	fclose(structDados.fp);

	FILE* filetest = fopen(structDados.fileName, "rb");
	fseek(filetest, 0, SEEK_END);
	if ((structDados.filesize = ftell(structDados.fp)))
		printf("\nFile sizes match!\n\n");
	else
		printf("\nFile sizes don't match!\n\n");

	return numread;
}

/*
 *	
 */
int linkread(unsigned char* dataPackage) {
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
		case 4: {
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
				N = c + 20;

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
			} else if (c == 0x01) {
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
			} else if (c == 0x02) {
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;
				estado = 5;
			} else
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
			} else if (c == bcc2)
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
	if (wasREJsent)
		return -1;

	if (Controlo == 0x00) { // Se foi pacote de dados...
		if (N == lastN)
			duplicate = TRUE;
		lastN = N;
	}

	return packagesLength;
}

/*
 *	
 */
int llclose() {
	tentativaEnvio = 1;
	estado = 0;

	unsigned char DISC[5]; // Trama DISC (disconnect)
	DISC[0] = FLAG;
	DISC[1] = A;
	DISC[2] = 0x0B;
	DISC[3] = A ^ 0x0B;
	DISC[4] = FLAG;

	if (structDados.sender == TRUE)
		senderDISC(DISC);
	else if (structDados.sender == FALSE)
		receiverDISC(DISC);

	if (tcsetattr(structDados.fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	printf("Closing port...\n");
	close(structDados.fd);

	return 0;
}

/*
<<<<<<< HEAD
 *	Function that clear all the screen
 */
void limparEcra() {
	unsigned int i;
	for (i = 0; i < 50; i++)
		printf("\n");
}

/*
=======
>>>>>>> origin/master
 *	
 */
void senderDISC(unsigned char* DISC) {
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

			if (res == 5)
				printf("Sent DISC\n");
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
					} else
						printf("DISC: Remaining on state 0\n");
					break;
				case 1:
					if (c == A) {
						estado = 2;
						printf("DISC: Switching to state 2\n");
						buf[1] = c;
					} else if (c == FLAG) {
						estado = 1;
						printf("DISC: Remaining on state 1\n");
						buf[0] = c;
					} else {
						estado = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 2:
					if (c == 0x0B) {
						estado = 3;
						printf("DISC: Switching to state 3\n");
						buf[2] = c;
					} else if (c == FLAG) {
						estado = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					} else {
						estado = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 3:
					if (c == (A ^ 0x0B)) {
						estado = 4;
						printf("DISC: Switching to state 4\n");
						buf[3] = c;
					} else if (c == FLAG) {
						estado = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					} else {
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
					} else {
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
void receiverDISC(unsigned char* DISC) {
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
			} else
				printf("DISC: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {

				estado = 2;
				printf("DISC: Switching to state 2\n");
				buf[1] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("DISC: Remaining on state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x0B) {
				estado = 3;
				printf("DISC: Switching to state 3\n");
				buf[2] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("DISC: Switching to state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x0B)) {
				estado = 4;
				printf("DISC: Switching to state 4\n");
				buf[3] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("  DISC: Switching to state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				estado = 5;
				printf("DISC: Switching to state 5\n");
				buf[4] = c;
			} else {
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
			} else
				printf("UA: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {

				estado = 2;
				printf("UA: Switching to state 2\n");
				buf[1] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("UA: Remaining on state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x03) { //
				estado = 3;
				printf("UA: Switching to state 3\n");
				buf[2] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("UA: Switching to state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x03)) { //
				estado = 4;
				printf("UA: Switching to state 4\n");
				buf[3] = c;
			} else if (c == FLAG) {
				estado = 1;
				printf("UA: Switching to state 1\n");
				buf[0] = c;
			} else {
				estado = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				estado = 5;
				printf("UA: Switching to state 5\n");
				buf[4] = c;
			} else {
				estado = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		}
	}
}
