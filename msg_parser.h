#ifndef MSG_PARSER_H
#define MSG_PARSER_H

#include "message.h"

typedef void MParser;

MParser* MParser_New();
TMessage* MParser_ReadMem(MParser* parser, unsigned char* mem, unsigned int length);
TMessage* MParser_ReadFd(MParser* parser, int fd);
void MParser_Reset(MParser* parser);
void MParser_Del(MParser* parser);

#endif