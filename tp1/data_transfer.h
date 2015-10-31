#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include "app.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03

extern int linkwrite(unsigned char* data, Settings* structDados, int datasize, int Ns, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar);
extern int linkread(unsigned char* dataPackage, Settings* structDados, int duplicate);

#endif 