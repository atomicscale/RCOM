#ifndef TERMINATION_H
#define TERMINATION_H

#include "app.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03

extern void senderDISC(unsigned char* DISC, Settings structDados, volatile int estado, volatile int tentativaEnvio, volatile int podeEnviar);
extern void receiverDISC(unsigned char* DISC, Settings structDados, volatile int estado);


#endif