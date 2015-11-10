//==============================================================================
// Name        : awget.h
// Author      : Nicholas
// Version     : 1
// Copyright   : None
// Description : v1: holds declarations for awget.c
//
//==============================================================================

#ifndef AWGET_H
#define AWGET_H

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int MAX_STONES = 20;
const int DEFAULT_PORT = 21789;
const int BUFFERSIZE = 1024;

typedef struct {
    char SSaddr[128];
    int SSport;
}StepStone;

#endif //AWGET_H
