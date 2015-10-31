#ifndef APP_H
#define APP_H


#include <stdio.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03

/*
 * Struct that allows us saving all the information about the transmission
 */
typedef struct{
    

    int fd;
    FILE* fp;
    char* fileName;
    int filesize;
    int sender; // TRUE ou FALSE
    char port[20]; // Dispositivo /dev/ttySx
    int baudRate; // Velocidade de transmissão
    unsigned int timeout; // Valor do temporizador: 1 s
    unsigned int numTransmissions; // Número de tentativas em caso de falha
    unsigned int maxSize;
    
} Settings;


extern int llopen(Settings* structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar);
extern int llwrite(Settings* structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar);
extern int llread(Settings* structDados);
extern int llclose(Settings* structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar);

#endif 