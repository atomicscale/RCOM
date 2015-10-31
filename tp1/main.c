#include "app.h"

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

static Settings* structDados; // Esta definida no app.h

volatile int estado = 0, tentativaEnvio = 1, podeEnviar = TRUE;

/*
 *	
 */
void atendeAlarme()  {
	printf("Bad response #%d from receiver!\n", tentativaEnvio);
	tentativaEnvio++;
	podeEnviar = TRUE;
	estado = 0;
}

/*
 *
 */
void limparEcra() {
	unsigned int i;
	for (i = 0; i < 50; i++)
		printf("\n");
};



/*
 * Prepare the port to send the packages
 */
void prepareSender() {
	limparEcra();

	if ((structDados->fp = fopen(structDados->fileName, "rb")) != NULL) {
		fseek(structDados->fp, 0, SEEK_END);
		structDados->filesize = ftell(structDados->fp);
		rewind(structDados->fp);
	}
	else {
		printf("File not found!\n");
		exit(-1);
	}

	printf("Sending file...\n");

	llwrite(structDados, estado, tentativaEnvio, podeEnviar);
}

/*
 * Prepare the port to receive the packages
 */
void prepareReceiver() {
	limparEcra();

	printf("Receiving file...\n");

	//unsigned char* buf = NULL;
	llread(structDados);
}

/*
 * Get the information about the transmission
 */
void startstruct() {

	int choicePort = -1;

	
	structDados = malloc(sizeof(Settings));
	structDados->fp = malloc(sizeof(FILE));
	structDados->fd =  0;
	structDados->fileName = malloc(255 * sizeof(char) + 1);
	structDados->filesize = 0;
	structDados->sender = 0;
	/* structDados->port = malloc(20 * sizeof(char) + 1); doesnt need */ 
	structDados->baudRate = 0;
	structDados->timeout = 0;
	structDados->numTransmissions = 0;
	structDados->maxSize = 0;

	do {
		limparEcra();
		printf(". Are you a sender or a receiver?\n");
		printf("    1. Sender \n");
		printf("    2. Receiver \n");
		char choice = getchar();

		if (choice == '1'){
			structDados->sender = TRUE;
			break;
		}
		else if (choice == '2'){
			structDados->sender = FALSE;
			break;
		}
		else 
			continue;

	} while (TRUE);

	
	if (structDados->sender == TRUE) {
		limparEcra();
		printf(". What's the name of the file you want to send?\n");
		getchar();
		gets(structDados->fileName);
	}



	limparEcra();
	printf(". Which port will you use?\n");
	scanf("%d", &choicePort);
	snprintf(structDados->port, sizeof(structDados->port), "/dev/ttyS%d", choicePort);
	sleep(1);

	do {
		limparEcra();
		printf(". Which BAUDRATE do you prefer?\n");
		printf("     1. B9600\n");
		printf("     2. B19200\n");
		printf("     3. B38400\n");
		printf("     4. B57600\n");
		printf("     5. B115200\n");
		int choice = scanf("%d", &choice);
		if (choice == 1) {
			structDados->baudRate = B9600;
			break;
		}
		else if (choice == 2) {
			structDados->baudRate = B19200;
			break;
		}
		else if (choice == 3) {
			structDados->baudRate = B38400;
			break;
		}
		else if (choice == 4) {
			structDados->baudRate = B57600;
			break;
		}
		else if (choice == 5) {
			structDados->baudRate = B115200;
			break;
		}
		else continue;
	} while (TRUE);

	limparEcra();
	printf(". How will your timeout be?\n");
	scanf("%d", &structDados->timeout);

	limparEcra();
	printf(". How many times will your program try to send a package?\n");
	scanf("%d", &structDados->numTransmissions);

	if (structDados->sender) {
		limparEcra();
		printf(". How many data bytes should each frame contain?\n");
		scanf("%d", &structDados->maxSize);
		structDados->maxSize += 4;
	}
	else structDados->maxSize = 255;
}


int main(int argc, char** argv) {

	signal(SIGALRM, atendeAlarme); /* Instalar alarme */
	setvbuf(stdout, NULL, _IONBF, 0); /* Desativar buffer do STDOUT */

	do {

		limparEcra();
		
		printf("-------------------------\n");
		printf("- SERIAL PORT RCOM 1516 -\n");
		printf("-------------------------\n");
		printf("- 1. Run   			    -\n");
		printf("- 2. Exit               -\n");
		printf("-------------------------\n");
		
		int choice;
		scanf("%d", &choice);

		switch (choice) {
		case 1:
			startstruct();
			structDados->fd = llopen(structDados, estado, tentativaEnvio, podeEnviar);

			if (structDados->sender)
				prepareSender();
			else
				prepareReceiver();

			llclose(structDados, estado, tentativaEnvio, podeEnviar);
			return 0;
		case 2:
			printf("Exiting program! \n");
			return 0;
		default:
			continue;
		}

	} while (TRUE);

	return 0;
}
