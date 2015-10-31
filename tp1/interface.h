/*
 *
 */

#include <stdio.h>

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

void limparEcra();

int InitialMenu();

int SenderOrReceiver();

char* getFileName();

int getPort();

int getBaudRate();

int getTimeOut();

int getMaxTrans();

int getMaxPackages();






#endif