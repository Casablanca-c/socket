#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <malloc.h>
#include "tcp_client.h"
#include "tcp_server.h"

#define FD_SIZE 1024

typedef struct tcp_server
{
    int fd;
    int valid;
    Listener cb;
    TcpClient* client[FD_SIZE];
} TServer;

TcpServer* TcpServer_New()
{
    TServer* ret = malloc(sizeof(TServer));

    if( ret )
    {
        int i = 0;

        ret->fd = -1;
        ret->valid = 0;
        ret->cb = NULL;

        for(int i=0; i<FD_SIZE; i++)
        {
            ret->client[i] = NULL;
        }
    }

    return ret;
}

int TcpServer_Start(TcpServer* server, int port, int max)
{
    TServer* s = (TServer*)server;

    if( s && !s->valid )
    {
        struct sockaddr_in saddr = {0};

        s->fd = socket(PF_INET, SOCK_STREAM, 0);    
        s->valid = (s->fd != -1);

        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        saddr.sin_port = htons(port);

        s->valid = s->valid && (bind(s->fd, (struct sockaddr*)&saddr, sizeof(saddr)) != -1 );
        s->valid = s->valid && (listen(s->fd, max) != -1);
    }

    return s->valid;
}

void TcpServer_Stop(TcpServer* server)
{
    TServer* s = (TServer*)server;

    if( s )
    {
        int i = 0;

        s->valid = 0;

        close(s->fd);

        for(i=0; i<FD_SIZE; i++)
        {
            TcpClient_Del(s->client[i]);
            s->client[i] = NULL;
        }
    }
}

void TcpServer_SetListener(TcpServer* server, Listener listener)
{
    TServer* s = (TServer*)server;

    if( s )
    {
        s->cb = listener;
    }
}

int TcpServer_IsValid(TcpServer* server)
{
    return server ? ((TServer*)server)->valid : 0;
}

static int SelectHandler(TServer* s, fd_set* rset, fd_set* reads, int num, int max)
{
    int ret = max;
    int i = 0;

    for(i=0; i<=max; i++)
    {
        if( FD_ISSET(i, rset) )
        {
            int index = i;
            int event = -1;

            if( index == s->fd )
            {
                struct sockaddr_in caddr = {0};
                socklen_t asize = sizeof(caddr);

                index = accept(s->fd, (struct sockaddr*)&caddr, &asize);

                if( index > -1 )
                {
                    FD_SET(index, reads);

                    ret = (index > max) ? index : max;

                    s->client[index] = TcpClient_From(index);

                    event = EVT_CONN;
                }
            }
            else
            {
                event = EVT_DATA;
            }

            if( s->cb )
            {
                if( TcpClient_IsValid(s->client[index]) )
                {
                    s->cb(s->client[index], event);
                }
                else
                {
                    if( s->client[index] )
                    {
                        s->cb(s->client[index], EVT_CLOSE);
                    }

                    TcpClient_Del(s->client[index]);

                    s->client[index] = NULL;

                    FD_CLR(index, reads);
                }
            }
        }
    }

    return ret;
}

void TcpServer_DoWork(TcpServer* server)
{
    TServer* s = (TServer*)server;

    if( s && s->valid )
    {
        int max = 0;
        int num = 0;
        fd_set reads = {0};
        fd_set rset = {0};
        struct timeval timeout = {0};

        FD_ZERO(&reads);
        FD_SET(s->fd, &reads);

        max = s->fd;

        while( s->valid )
        {
            rset = reads;

            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;

            num = select(max+1, &rset, 0, 0, &timeout);

            if( num > 0 )
            {
                max = SelectHandler(s, &rset, &reads, num, max);
            }
        }
    }
} 

void TcpServer_Del(TcpServer* server)
{
    TcpServer_Stop(server);
    free(server);
}
