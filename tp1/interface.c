#include "interface.h"


void limparEcra() {
	unsigned int i;
	for (i = 0; i < 50; i++)
		printf("\n");
}

int InitialMenu() {

	limparEcra();

	printf("-------------------------\n");
	printf("- SERIAL PORT RCOM 1516 -\n");
	printf("-------------------------\n");
	printf("- 1. Run   			    -\n");
	printf("- 2. Exit               -\n");
	printf("-------------------------\n");

	int choice = scanf("%d",&choice);

	return choice;
}

int SenderOrReceiver() {

	limparEcra();

	limparEcra();
	printf("-> Are you a sender or a receiver?\n");
	printf("    1. Sender \n");
	printf("    2. Receiver \n");

	int choice = scanf("%d",&choice);

	return choice;

}

char* getFileName() {
	char name[20];

	printf("-> What's the name of the file you want to send?\n");
	getchar();
	gets(name);

	return name;
}

int getPort(){

	int choicePort;

	limparEcra();
	printf("-> Which port will you use?\n");
	scanf("%d", &choicePort);

	return choicePort;
}

int getBaudRate(){

	limparEcra();
	printf("-> Which BAUDRATE do you prefer?\n");
	printf("     1. B9600\n");
	printf("     2. B19200\n");
	printf("     3. B38400\n");
	printf("     4. B57600\n");
	printf("     5. B115200\n");
	int choice = scanf("%d", &choice);

	return choice;
}

int getTimeOut(){

	int timeOut;

	printf("# How will your timeout be?\n");
	scanf("%d", &timeOut);

	return timeOut;
}

int getMaxTrans(){

	limparEcra();

	printf("-> How many times will your program try to send a package?\n");
	int max = scanf("%d", &max);

	return max;
}

int getMaxPackages(){

	limparEcra();

	printf("-> How many data bytes should each frame contain?\n");
	int bytes = scanf("%d",&bytes);

	return bytes;

}