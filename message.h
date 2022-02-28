#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct message
{
    unsigned short type;
    unsigned short cmd;
    unsigned short index;
    unsigned short total;
    unsigned int length;
    unsigned char payload[];
} TMessage;

TMessage* Message_New(unsigned short type, unsigned short cmd, unsigned short index, unsigned short total, const char* payload, unsigned int length);
int Message_Size(TMessage* m);
TMessage* Message_N2H(TMessage* m);
TMessage* Message_H2N(TMessage* m);

#endif