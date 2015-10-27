#include "include.h"

#define _POSIX_SOURCE 	1 /* POSIX compliant source */
#define FALSE 			0
#define TRUE 			1
#define FLAG 			0x7e
#define A 				0x03
#define C 				(data.sender ? 0x03 : 0x07)

/* Some usefull variables */
static Data *data;

volatile int state = 0;
volatile int trySend = 1;
volatile int canSend = TRUE;

int duplicate = FALSE;

struct termios oldtio;

/* auxiliary function to clean screen */
void cleanScreen() {
	int i;
	for (i = 0; i < 50; i++)
		printf("\n");
}


int main(int argc, char** argv){

	signal(SIGALRM, callAlarm); /* Install the alarm */
	setvbuf(stdout,NULL,_IONBF,0); /* Disables the STDOUT */

	while(1) {

		cleanScreen();

		printf("-----------------------\n");
		printf("-  RCOM PROJECT 1516  -\n");
		printf("-----------------------\n");
		printf("- 1. Send / Receive   -\n");
		printf("- 2. Exit             -\n");
		printf("-----------------------\n");

		int choice = 0;

		scanf("%d",&choice);

		if(choice == 1){

			intializeTypeOfTransmission();
			data.fd = llopen();

			if(data.sender)
				initalizeSender();
			else
				initializeReceiver();

			llclose(data.fd);

			return 0;
		} else if(choice == 2){
			printf("Exiting program! \n");
			return 2;
		}
		  else
		    printf("Invalid option! Try Again!");

	} 

	return 0;
}

void initalizeSender(){

	cleanScreen();

	if((data.fp = fopen(data.fileName,"rb")) != NULL){
		fseek(data.fp, 0, SEEK_END); /* specifies that the offset provided is relative to the end of the file */
		data.filesize = ftell(data.fp); /* give the size of the file */
		rewind(data.fp); /* sets the file position to the beginning of the file */
	} else {
		printf("File not found! \n");
		exit(-1);
	}

	printf("Sending file . . . \n");

	llwrite();
}


void initializeReceiver(){

	cleanScreen();
	printf("Receiving file . . . \n");

	unsigned char* buf = NULL;
	llread(data.fp,buf);
}

void intializeTypeOfTransmission(){

	/* Get if we are receiving or sending */
	while(1) {

		cleanScreen();
		printf("What you want to do? \n");
		printf("	1. Send\n");
		printf("	2. Receive\n");
		char choice = getchar();

		if(choice == 1){
			data.sender = TRUE;
			break;
		} else if(choice == 2){
			data.sender = FALSE;
			break;
		} else 
			printf("Invalid input! Try again!");
	
	}

	/* If we are sending */
	if (data.sender) { 
		cleanScreen();
		printf("What's the name of the file you want to send?\n");
		getchar();
		gets(data.fileName);
	}

	/* Getting the port */
	cleanScreen();
	int choicePort = -1;
	printf("Which port will you use?\n");
	scanf("%d", &choicePort);
	snprintf(data.port, sizeof(data.port), "/dev/ttyS%d", choicePort);

	/* Getting the Baudrate */
	while(1) {
		
		cleanScreen();
		printf("Which BAUDRATE do you prefer?\n");
		printf("     1. B9600\n");
		printf("     2. B19200\n");
		printf("     3. B38400\n");
		printf("     4. B57600\n");
		printf("     5. B115200\n");
		
		int choice = scanf("%d", &choice);
		
		if (choice == 1) {
			data.baudRate = B9600;
			break;
		}
		else if (choice == 2) {
			data.baudRate = B19200;
			break;
		}
		else if (choice == 3) {
			data.baudRate = B38400;
			break;
		}
		else if (choice == 4) {
			data.baudRate = B57600;
			break;
		}
		else if (choice == 5) {
			data.baudRate = B115200;
			break;
		}
		else {
			printf("Invalid BAUDRATE! Try again!");
			continue;
		}

	}

	/* Getting TimeOut */
	cleanScreen();
	printf("TimeOut(Seconds): \n");
	scanf("%d", &data.timeout);

	/* Getting Attempts */
	cleanScreen();
	printf("Max Attempts(> 0): \n");
	scanf("%d", &data.maxTransmissions);

	/* Getting data bytes off each frame */
	if (data.sender) {
		cleanScreen();
		printf("How many data bytes should each frame contain?\n");
		scanf("%d", &data.maxSize);
		data.maxSize += 4;
	}
	else 
		data.maxSize = 255;
}

int llopen() {
	
	struct termios newtio;

	/* Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C. */
	int fd = open(data.port, O_RDWR | O_NOCTTY);
	
	if (fd < 0) { 
		perror("Open serial port device for reading and writing\n"); 
		exit(-1); 
	} 

	printf("Sucess, open serial port for reading and writing\n");

	/* Saving current port settings */
	if (tcgetattr(fd, &oldtio) == -1) { 
		perror("Saving current port settings!\n");
		return -1;
	}

	bzero(&newtio, sizeof(newtio)); 
	newtio.c_cflag = data.baudRate | CS8 | CLOCAL | CREAD;
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

	if (data.sender == TRUE) {
		printf("Sending SET! \n");
		sendSET(fd);
	} else {
		printf("Sending UA! \n");
		sendUA(fd);
	}

	return fd;
}

void sendSET(int fd) {
	
	/* Trama SET */
	unsigned char SET[5]; 
	SET[0] = FLAG;
	SET[1] = A;
	SET[2] = 0x07;
	SET[3] = SET[1] ^ SET[2];
	SET[4] = FLAG;

	unsigned char c;
	unsigned char buf[5];
	int passed = FALSE;

	while (trySend <= data.maxTransmissions && passed == FALSE) {
		
	   if (canSend) {
			
		canSend = FALSE;
		alarm(data.timeout); /* generate a SIGALRM signal for the process after the number of time-out seconds */
		int frameSent = write(fd, SET, 5);

		if (frameSent == 5) {
			printf("Frame sent successfully\n");
		} else {
			printf("Frame wasn't sent successfully\n");
			canSend = TRUE;
		}

		alarm(data.timeout); /* generate a SIGALRM signal for the process after the number of time-out seconds */

		/* State MAchine SET */
		while (state != 5) {
				
			if (canSend) {
				alarm(0); /* Cancels last alarm call */
				break;
			}

			read(fd, &c, 1);

			switch (state) {
				case 0:
					if (c == FLAG) {
						state = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					}
					else printf("UA: Remaining on state 0\n");
					break;
				case 1:
					if (c == A) {
						state = 2;
						printf("UA: Switching to state 2\n");
						buf[1] = c;
					}
					else if (c == FLAG) {
						state = 1;
						printf("UA: Remaining on state 1\n");
						buf[0] = c;
					}
					else {
						state = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 2:
					if (c == 0x03) {
						state = 3;
						printf("UA: Switching to state 3\n");
						buf[2] = c;
					}
					else if (c == FLAG) {
						state = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						state = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 3:
					if (c == (A ^ 0x03)) {
						state = 4;
						printf("UA: Switching to state 4\n");
						buf[3] = c;
					}
					else if (c == FLAG) {
						state = 1;
						printf("UA: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						state = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				case 4:
					if (c == FLAG) {
						alarm(0);
						state = 5;
						printf("UA: Switching to state 5\n");
						buf[4] = c;
						passed = TRUE;
					}
					else {
						state = 0;
						printf("UA: Switching to state 0\n");
					}
					break;
				}
			}
			
		}
	}
	
	if (trySend > data.maxTransmissions) {

		printf("-> Max Transmissions reached\n");
		exit(-1);

	} else 
		printf("-> Sucess sending SET\n");
	
}

void sendUA(int fd) {
	
 	/* Trama UA (unnumbered acknowledgement) */
	unsigned char UA[5]; 
	UA[0] = FLAG;
	UA[1] = A;
	UA[2] = 0x03;
	UA[3] = A ^ UA[2];
	UA[4] = FLAG;

	unsigned char c;
	unsigned char buf[5];

	while (state != 5) {
	  
	  read(fd, &c, 1);

	  /* State machine  UA */
	  switch (state) {
		case 0:
			if (c == FLAG) {
			   state = 1;
			   printf("SET: Switching to state 1\n");
			   buf[0] = c;
			}
			else printf("SET: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {
				state = 2;
				printf("SET: Switching to state 2\n");
				buf[1] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("SET: Remaining on state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x07) {
				state = 3;
				printf("SET: Switching to state 3\n");
				buf[2] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x07)) {
				state = 4;
				printf("SET: Switching to state 4\n");
				buf[3] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("SET: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				state = 5;
				printf("SET: Switching to state 5\n");
				buf[4] = c;
			}
			else {
				state = 0;
				printf("SET: Switching to state 0\n");
			}
			break;
		}
	}

	write(fd, UA, 5);

	printf("-> Sucess Sending UA \n");
}

void callAlarm()  {
	printf("Bad response %d from receiver!\n", trySend);
	trySend++;
	canSend = TRUE;
	state = 0;
}


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

	while (TRUE) {
		
		unsigned char dataPackage[data.maxSize];

		int packageSize = linkread(dataPackage);

		if (packageSize == -1) {

			REJ[2] = (Nr == 0) ? 0x05 : 0x25;
			REJ[3] = REJ[1] ^ REJ[2];
			tcflush(data.fd, TCIFLUSH);
			write(data.fd, REJ, 5);

		}   else if (dataPackage[0] == 0x00) {
			
			int posPackage = 0;
			
			if (duplicate) 
				printf("Duplicate frame!\n");

			while (posPackage < packageSize && !duplicate) {
				fwrite(dataPackage + 4 + posPackage, sizeof(unsigned char), 1, data.fp);
				posPackage++;
				numread++;
			}

			if (!duplicate)
				Nr = (Nr + 1) % 2;

			duplicate = FALSE;
			RR[2] = (Nr == 0) ? 0x01 : 0x21;
			RR[3] = RR[1] ^ RR[2];
			write(data.fd, RR, 5);

		}	else if (dataPackage[0] == 0x01) {
			
			unsigned int i = 1;
			unsigned int j;

			if (dataPackage[i++] == 0x00 && dataPackage[i++] == 0x02) {
				data.filesize = dataPackage[i++] << 8;
				data.filesize += dataPackage[i++];
			}

			if (dataPackage[i++] == 0x01) {
				unsigned int size = dataPackage[i++];
				for (j = 0; j < size; j++) {
					data.fileName[j] = dataPackage[i++];
					printf("%c", data.fileName[j]);
				}
				printf("\n");
			}

			data.fp = fopen(data.fileName, "wb");

			if (dataPackage[i++] == 0x02 && dataPackage[i++] == 0x02) {
				data.maxSize = dataPackage[i++] << 8;
				data.maxSize += dataPackage[i++];
			}

			Nr = (Nr + 1) % 2;
			RR[2] = (Nr == 0) ? 0x01 : 0x21;
			RR[3] = RR[1] ^ RR[2];
			write(data.fd, RR, 5);

		}	else if (dataPackage[0] == 0x02) {
			Nr = (Nr + 1) % 2;
			RR[2] = (Nr == 0) ? 0x01 : 0x21;
			RR[3] = RR[1] ^ RR[2];
			write(data.fd, RR, 5);
			break;
		}
	} 

	fclose(data.fp);
	
	FILE* filetest = fopen(data.fileName, "rb");
	fseek(filetest, 0, SEEK_END);
	
	if ((data.filesize = ftell(data.fp)))
		printf("\nFile sizes match!\n\n");
	else
		printf("\nFile sizes don't match!\n\n");

	return numread;
}

int llwrite() {
	
	int Ns = 0;
	unsigned int i = 0;

	unsigned char CTRL_START[11 + strlen(data.fileName) + 1];
	CTRL_START[i++] = 0x01; // Indica início
	CTRL_START[i++] = 0x00; // A enviar tamanho do ficheiro
	CTRL_START[i++] = 0x02; // 2 bytes
	CTRL_START[i++] = (unsigned char)((data.filesize & 0xff00) >> 8); // Tamanho do ficheiro
	CTRL_START[i++] = (unsigned char)(data.filesize & 0xff);
	CTRL_START[i++] = 0x01; // A enviar nome do ficheiro
	CTRL_START[i++] = strlen(data.fileName) + 1; 

	unsigned int j;

	for (j = 0; j < strlen(data.fileName) + 1; j++) {
		CTRL_START[i++] = data.fileName[j];
	}
	CTRL_START[i++] = 0x02; // A enviar comprimento máximo do campo de dados das tramas I
	CTRL_START[i++] = 0x02; // 2 bytes
	CTRL_START[i++] = (data.maxSize >> 8) & 0xff;
	CTRL_START[i++] = data.maxSize & 0xff;

	if (linkwrite(CTRL_START, 11 + strlen(data.fileName) + 1, Ns) < 0) 
		return -1;

	Ns = (Ns + 1) % 2;

	int resRead;
	int numPack = 0;
	int numWritten = 0;

	do {
		unsigned char DATA_PACK[data.maxSize];
		DATA_PACK[0] = 0x00; // Indica dados
		DATA_PACK[1] = numPack++ % 256; // Número de sequência
		resRead = fread(DATA_PACK + 4, sizeof(unsigned char), data.maxSize - 4, data.fp);
		DATA_PACK[2] = (resRead >> 8) & 0xff;
		DATA_PACK[3] = resRead & 0xff;

		if (linkwrite(DATA_PACK, 4 + resRead, Ns) < 0) return -1;
		Ns = (Ns + 1) % 2;
	} while (resRead);

	unsigned char CTRL_STOP[1];
	CTRL_STOP[0] = 0x02; // Indica paragem
	
	if (linkwrite(CTRL_STOP, 1, Ns) < 0) 
	    return -1;

	Ns = (Ns + 1) % 2;

	fclose(data.fp);

	return numWritten; // Número de bytes escritos
}


int linkwrite(unsigned char* data, int datasize, int Ns) {
	trySend = 1;
	canSend = TRUE;
	state = 0;

	unsigned int aSubstituir = 0;
	unsigned char FRAME_I[6 + datasize];
	FRAME_I[0] = FLAG;
	FRAME_I[1] = A;
	FRAME_I[2] = Ns == 0 ? 0x00 : 0x20;
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
		if (j < 4 || j == 6 + datasize - 1) FRAME_I_FINAL[i] = FRAME_I[j];
		else if (FRAME_I[j] == 0x7e || FRAME_I[j] == 0x7d) {
			FRAME_I_FINAL[i++] = 0x7d;
			FRAME_I_FINAL[i] = 0x20 ^ FRAME_I[j];
		}
		else FRAME_I_FINAL[i] = FRAME_I[j];
	}

	int OK = FALSE; // Recebeu RR corretamente?
	while (trySend <= data.maxTransmissions) {
		if (OK) break;
		else if (canSend) {
			printf("Sending frame: ");
			for (i = 0; i < 6 + datasize + aSubstituir; i++)
				printf("%d ", FRAME_I_FINAL[i]);
			printf("\n");
			write(data.fd, FRAME_I_FINAL, 6 + datasize + aSubstituir);
			alarm(data.timeout);
			canSend = FALSE;
		}

		unsigned char c;
		int res = read(data.fd, &c, 1);

		if (res != 0) {
			alarm(0);

			switch (state) {
			case 0:
				if (c == FLAG) {
					state = 1;
					printf("Confirmation: Switching to state 1\n");
				}
				else printf("Confirmation: Remaining on state 0\n");
				break;
			case 1:
				if (c == A) {
					state = 2;
					printf("Confirmation: Switching to state 2\n");
				}
				else if (c == FLAG)
					printf("Confirmation: Remaining on state 1\n");
				else {
					state = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			case 2:
				if (c == (Ns == 0 ? 0x21 : 0x01)) {
					state = 3;
					printf("Confirmation: positive. Switching to state 3\n");
				}
				else if (c == 0x05 || c == 0x25) {
					canSend = TRUE;
					state = 0;
					printf("Confirmation: negative. Resending...\n");
				}
				else if (c == FLAG) {
					state = 1;
					printf("Confirmation: Switching to state 1\n");
				}
				else {
					state = 0;
					printf("Confirmation: Switching 2 state 0\n");
				}
				break;
			case 3:
				if (c == (A ^ (Ns == 0 ? 0x21 : 0x01))) {
					state = 4;
					printf("Confirmation: Switching to state 4\n");
				}
				else if (c == FLAG) {
					state = 1;
					printf("Confirmation: Switching to state 1\n");
				}
				else {
					state = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			case 4:
				if (c == FLAG) {
					state = 5;
					printf("Confirmation: Switching to state 5\n");
					OK = TRUE; // Sair do ciclo
				}
				else {
					state = 0;
					printf("Confirmation: Switching to state 0\n");
				}
				break;
			}
		}
	}

	return trySend <= data.maxTransmissions ? 0 : -1;
}

int linkread(unsigned char* dataPackage) {
	
	unsigned char c;
	unsigned char buf[data.maxSize + 6];
	int state = 0;
	unsigned char bcc2 = 0x00;
	int packagesLength = 0;
	int i = 0;
	unsigned char Controlo;
	unsigned char N;
	int wasREJsent = FALSE;


	while (state != 7 && wasREJsent == FALSE) {
		read(data.fd, &c, 1);

		switch (state) {
		case 0:
			if (c == FLAG)
				buf[state++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 1:
			if (c == A)
				buf[state++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 2:
			if (c == 0x00 || c == 0x20)
				buf[state++] = c;
			else
				wasREJsent = TRUE;
			break;
		case 3:
			if (c == (buf[1] ^ buf[2]))
				buf[state++] = c;
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

				read(data.fd, &c, 1);
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // N
				printf("N = %d\n", c);
				N = c;

				read(data.fd, &c, 1);
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // L2      
				read(data.fd, &c, 1);
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c; // L1    
				packagesLength = 256 * dataPackage[2] + dataPackage[3];
				i = packagesLength;
				while (i > 0) {
					read(data.fd, &c, 1);
					if (c == 0x7d) {
						read(data.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
					i--;
				}
				state = 5;
			}
			else if (c == 0x01) {
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(data.fd, &c, 1); // T1
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(data.fd, &c, 1); // L1
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado] = c;

				for (i = dataPackage[megaEstado++]; i > 0; i--) {
					read(data.fd, &c, 1); // V1
					if (c == 0x7d) {
						read(data.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
				}

				read(data.fd, &c, 1); // T2
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(data.fd, &c, 1); // L2
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado] = c;

				for (i = dataPackage[megaEstado++]; i > 0; i--) {
					read(data.fd, &c, 1); // V2
					if (c == 0x7d) {
						read(data.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
				}

				read(data.fd, &c, 1); // T3
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;

				read(data.fd, &c, 1); // L3
				if (c == 0x7d) {
					read(data.fd, &c, 1);
					c ^= 0x20;
				}
				bcc2 ^= c;
				dataPackage[megaEstado] = c;

				for (i = dataPackage[megaEstado++]; i > 0; i--) {
					read(data.fd, &c, 1); // V3
					if (c == 0x7d) {
						read(data.fd, &c, 1);
						c ^= 0x20;
					}
					bcc2 ^= c;
					dataPackage[megaEstado++] = c;
				}

				state = 5;
			}
			else if (c == 0x02) {
				bcc2 ^= c;
				dataPackage[megaEstado++] = c;
				state = 5;
			}
			else
				wasREJsent = TRUE;
			break;
		}
		case 5:
			printf("bcc2 esperado: %d, obtido: %d\n", bcc2, c);
			if (c == 0x7d) {
				read(data.fd, &c, 1);
				c ^= 0x20;
				if (c == bcc2)
					state = 6;
				else
					wasREJsent = TRUE;
			}
			else if (c == bcc2)
				state = 6;
			else
				wasREJsent = TRUE;
			break;
		case 6:
			if (c == FLAG)
				state = 7;
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


int llclose() {
	trySend = 1;
	state = 0;

	unsigned char DISC[5]; // Trama DISC (disconnect)
	DISC[0] = FLAG;
	DISC[1] = A;
	DISC[2] = 0x0B;
	DISC[3] = A ^ 0x0B;
	DISC[4] = FLAG;

	if (data.sender == TRUE)
		senderDISC(DISC);
	else if (data.sender == FALSE)
		receiverDISC(DISC);

	if (tcsetattr(data.fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	printf("-> Closing port...\n");
	close(data.fd);

	return 0;
}

void senderDISC(unsigned char* DISC) {
	trySend = 1;
	canSend = TRUE;
	state = 0;

	unsigned char c;
	unsigned char buf[5]; 

	while (trySend <= data.numTransmissions) {
		if (canSend) {
			canSend = FALSE;
			alarm(data.timeout);
			int res = write(data.fd, DISC, 5);

			if (res == 5) printf("Sent DISC\n");
			else {
				printf("Unable to send DISC\n");
				canSend = TRUE;
			}

			alarm(data.timeout);

			while (state != 5) {
				if (canSend) {
					alarm(0);
					break;
				}

				read(data.fd, &c, 1);

				switch (state) {
				case 0:
					if (c == FLAG) {
						state = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					}
					else printf("DISC: Remaining on state 0\n");
					break;
				case 1:
					if (c == A) {
						state = 2;
						printf("DISC: Switching to state 2\n");
						buf[1] = c;
					}
					else if (c == FLAG) {
						state = 1;
						printf("DISC: Remaining on state 1\n");
						buf[0] = c;
					}
					else {
						state = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 2:
					if (c == 0x0B) {
						state = 3;
						printf("DISC: Switching to state 3\n");
						buf[2] = c;
					}
					else if (c == FLAG) {
						state = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						state = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 3:
					if (c == (A ^ 0x0B)) {
						state = 4;
						printf("DISC: Switching to state 4\n");
						buf[3] = c;
					}
					else if (c == FLAG) {
						state = 1;
						printf("DISC: Switching to state 1\n");
						buf[0] = c;
					}
					else {
						state = 0;
						printf("DISC: Switching to state 0\n");
					}
					break;
				case 4:
					if (c == FLAG) {
						alarm(0);
						state = 5;
						printf("DISC: Switching to state 5\n");
						buf[4] = c;
						trySend = data.numTransmissions + 1; // Obrigar a sair do ciclo
					}
					else {
						state = 0;
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
	UA[2] = 0x03;
	UA[3] = A ^ UA[2];
	UA[4] = FLAG;

	write(data.fd, UA, 5);
	sleep(1);
}

void receiverDISC(unsigned char* DISC) {
	unsigned char c;
	unsigned char buf[5];

	while (state != 5) {
		read(data.fd, &c, 1);

		switch (state) {
		case 0:
			if (c == FLAG) {
				state = 1;
				printf("DISC: Switching to state 1\n");
				buf[0] = c;
			}
			else printf("DISC: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {

				state = 2;
				printf("DISC: Switching to state 2\n");
				buf[1] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("DISC: Remaining on state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x0B) {
				state = 3;
				printf("DISC: Switching to state 3\n");
				buf[2] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("DISC: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x0B)) {
				state = 4;
				printf("DISC: Switching to state 4\n");
				buf[3] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("  DISC: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				state = 5;
				printf("DISC: Switching to state 5\n");
				buf[4] = c;
			}
			else {
				state = 0;
				printf("DISC: Switching to state 0\n");
			}
			break;
		}
	}

	write(data.fd, DISC, 5); 

	state = 0;

	while (state != 5) {
		read(data.fd, &c, 1);

		switch (state) {
		case 0:
			if (c == FLAG) {
				state = 1;
				printf("UA: Switching to state 1\n");
				buf[0] = c;
			}
			else printf("UA: Remaining on state 0\n");
			break;
		case 1:
			if (c == A) {

				state = 2;
				printf("UA: Switching to state 2\n");
				buf[1] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("UA: Remaining on state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		case 2:
			if (c == 0x03) {
				state = 3;
				printf("UA: Switching to state 3\n");
				buf[2] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("UA: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		case 3:
			if (c == (A ^ 0x03)) {
				state = 4;
				printf("UA: Switching to state 4\n");
				buf[3] = c;
			}
			else if (c == FLAG) {
				state = 1;
				printf("UA: Switching to state 1\n");
				buf[0] = c;
			}
			else {
				state = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		case 4:
			if (c == FLAG) {
				state = 5;
				printf("UA: Switching to state 5\n");
				buf[4] = c;
			}
			else {
				state = 0;
				printf("UA: Switching to state 0\n");
			}
			break;
		}
	}
}



