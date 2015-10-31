#ifndef ESTABLISHMENT_H
#define ESTABLISHMENT_H

#include "app.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03

extern void sendSET(int fd, Settings* structDados, int estado, int tentativaEnvio, int podeEnviar);
extern void sendUA(int fd, Settings* structDados, int estado);

#endif 