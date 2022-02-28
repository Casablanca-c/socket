#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "msg_parser.h"


typedef struct msg_parser
{
    TMessage cache;     // 缓存已解析的消息头
    int header;         // 标识消息头是否解析成功， 1--成功
    int need;           // 标识还需要多少字节才能完成当前解析
    TMessage* msg;      // 解析中的协议消息（半成品）
}TMsgParser;

static void InitState(TMsgParser* p) 
{
    p->header = 0;
    p->need = sizeof(p->cache);

    free(p->msg);

    p->msg = NULL;
}

static int ToMidState(TMsgParser* p) 
{
    p->header = 1;
    p->need = p->cache.length;

    p->msg = malloc(sizeof(p->cache) + p->need);

    if( p->msg ) 
    {
        *p->msg = p->cache;  // 因为结构体定义本身就避开了深拷贝,所以这么赋值就是按bit拷贝内存
    }
    return !!p->msg;
}

static TMessage* ToLastState(TMsgParser* p) 
{
    TMessage* ret = NULL;
    if( p->header && !p->need ) 
    {
        ret = p->msg;

        p->msg = NULL;
    }

    return ret;
}



static int ToRecv(int fd, char* buf, int size)
{
    int i = 0;
    int retry = 0;

    while( i < size )
    {
        int len = read(fd, buf, size);

        if( len > 0 )
        {
            i += len;
        }
        else if( len < 0 )
        {
            break;
        }
        else
        {
            if( retry++ > 5)
            {
                break;
            }

            usleep(200 * 1000);
        }
    }

    return i;
}

MParser* MParser_New()
{
    MParser* ret = calloc(1, sizeof(TMsgParser));

    if( ret )
    {
        InitState(ret);
    }

    return ret;
}

TMessage* MParser_ReadMem(MParser* parser, unsigned char* mem, unsigned int length)
{
    TMessage* ret = NULL;
    TMsgParser* p = (TMsgParser*)parser;

    if( p && mem && length )
    {
        if( !p->header )
        {
            int len = (p->need < length) ? p->need : length;
            int offset = sizeof(p->cache) - p->need;

            memcpy( (char*)&p->cache + offset, mem, len);

            if( len == p->need )
            {
                Message_N2H(&p->cache);

                mem += p->need;
                length -= p->need;

                if( ToMidState(p) )
                {
                    ret = MParser_ReadMem(p, mem, length);
                }
            }
            else
            {
                p->need -= len;
            }
        }
        else
        {
            if( p->msg ) 
            {
                int len = (p->need < length) ? p->need : length;
                int offset = p->msg->length - p->need;

                memcpy(p->msg->payload + offset, mem, len);

                p->need -= len;
            }

            if( ret = ToLastState(p) )
            {
                InitState(p);
            }
        }
    }

    return ret;
   
}

TMessage* MParser_ReadFd(MParser* parser, int fd)
{
    TMessage* ret = NULL;
    TMsgParser* p = (TMsgParser*) parser;

    if( (fd != -1) && p )
    {
        if( !p->header )
        {
            int offset = sizeof(p->cache) - p->need;
            int len = ToRecv(fd, (char*)&p->cache + offset, p->need);

            if( len == p->need )
            {
                Message_N2H(&p->cache);

                if( ToMidState(p))
                {
                    ret = MParser_ReadFd(p, fd);
                }
                else
                {
                    InitState(p);
                }
            }
            else
            {
                p->need -= len;
            }
        }
        else
        {
            if( p->msg )
            {
                int offset = p->cache.length - p->need;
                int len = ToRecv(fd, p->msg->payload + offset, p->need);

                p->need -= len;
            }

            if( ret = ToLastState(p) )
            {
                InitState(p);
            }
        }
    }

    return ret;
}

void MParser_Reset(MParser* parser)
{
    TMsgParser* p = (TMsgParser*) parser;

    if( p ) 
    {
        InitState(p);
    }
}

void MParser_Del(MParser* parser)
{
    TMsgParser* p = (TMsgParser*) parser;

    if( p )
    {
        free(p->msg);
        free(p);
    }
}
