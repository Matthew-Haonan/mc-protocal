/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/types.h>

#if defined(_WIN32)
# define OS_WIN32
/* ws2_32.dll has getaddrinfo and freeaddrinfo on Windows XP and later.
 * minwg32 headers check WINVER before allowing the use of these */
# ifndef WINVER
# define WINVER 0x0501
# endif
# include <ws2tcpip.h>
# define SHUT_RDWR 2
# define close closesocket
#else
# include <sys/socket.h>
# include <sys/ioctl.h>

#if defined(__OpenBSD__) || (defined(__FreeBSD__) && __FreeBSD__ < 5)
# define OS_BSD
# include <netinet/in_systm.h>
#endif

# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <poll.h>
# include <netdb.h>
#endif

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

#include "mc-private.h"

#include "mc-tcp.h"
#include "mc-tcp-private.h"

#ifdef OS_WIN32
static int _mc_tcp_init_win32(void)
{
    /* Initialise Windows Socket API */
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() returned error code %d\n",
                (unsigned int)GetLastError());
        errno = EIO;
        return -1;
    }
    return 0;
}
#endif

static int _mc_set_slave(mc_t *ctx, int slave)
{
    /* Broadcast address is 0 (MC_BROADCAST_ADDRESS) */
    if (slave >= 0 && slave <= 247) {
        ctx->slave = slave;
    } else if (slave == MC_TCP_SLAVE) {           /*孔--MC_TCP_SLAVE宏定义为0xFF*/
        /* The special value MC_TCP_SLAVE (0xFF) can be used in TCP mode to
         * restore the default value. */
        ctx->slave = slave;
    } else {
        errno = EINVAL;                      /*孔--errno.h里面定义了EINVAL，定义为22 */
        return -1;                           /*孔--英文为Invalid argument，无效参数 */
    }                                        /*孔--return -1表示如果参数无效，这个函数就返回-1 */

    return 0;                                /*孔--return 0表示如果参数有效，这个函数就返回-1 */
}

/* Builds a TCP request header */
//int _mc_tcp_build_request_basis(mc_t *ctx, int function,
//                                    uint32_t addr, int nb,
//                                    uint8_t *req)
int _mc_tcp_build_request_basis(mc_t *ctx, int function,    //增加形参指令和子指令
										int addr, int nb,
										uint8_t *req, int command, int subcommand)

{

    /* Extract from MC Messaging on TCP/IP Implementation Guide V1.0b
       (page 23/46):
       The transaction identifier is used to associate the future response
       with the request. So, at a time, on a TCP connection, this identifier
       must be unique. */
    //static uint16_t t_id = 0;

    /* Transaction ID */
    //if (t_id < UINT16_MAX)        /*孔--UINT16_MAX这又是在哪定义的？？？？？？？？？？？*/
    //    t_id++;
    //else
    //    t_id = 0;
    //req[0] = t_id >> 8;
    //req[1] = t_id & 0x00ff;

    /* Protocol Mc */
    //req[2] = 0;
    //req[3] = 0;                  /*孔--这段程序是什么意思？？？？？作用是什么？？？？？？？*/

    /* Length will be defined later by set_req_length_tcp at offsets 4
       and 5 */

    //req[6] = ctx->slave;          /*孔--req的1个元素为8位，slave为int，这样赋值把最后8个bit赋值*/
    //req[7] = function;
    //req[8] = addr >> 8;           /*孔--addr的高位*/
    //req[9] = addr & 0x00ff;       /*孔--addr的低位*/
    //req[10] = nb >> 8;            /*孔--nb的高位*/
    //req[11] = nb & 0x00ff;        /*孔--nb的低位*/

    //return _MC_TCP_PRESET_REQ_LENGTH;       /*孔--mc TCP当前请求预设长度，定义为12，为什么是12？？？？*/


	req[0] = 0x50;          
    req[1] = 0x00;          //孔--以上副帧头
    req[2] = 0x00;           
    req[3] = 0xff;       
    req[4] = 0xff;          
    req[5] = 0x03;       
	req[6] = 0x00;          //孔--以上访问路径
	/* Length will be defined later by set_req_length_tcp at offsets 7
       and 8 */
	
    
	req[9] = 0x10;        
	req[10] = 0x00;         //孔--以上监视定时器，0010H=16*0.25=4S
	req[11] = command & 0x00ff;        
	req[12] = command >> 8;       
	req[13] = subcommand & 0x00ff;        
	req[14] = subcommand >> 8;    //孔--以上指令子指令    
	req[15] = addr & 0x00ff;       
	req[16] = addr >> 8;
	req[17] = addr >> 16;         //孔--以上起始软元件编号
	req[18] = function;           //孔--软元件代码
	req[19] = nb & 0x00ff;
	req[20] = nb >> 8;            //孔--以上软元件点数

    return _MC_TCP_PRESET_REQ_LENGTH;       /*孔--mc TCP当前请求预设长度，定义为12，为什么是12？？？？*/
}

/* Builds a TCP response header */
int _mc_tcp_build_response_basis(sft_t *sft, uint8_t *rsp)
{
    /* Extract from MC Messaging on TCP/IP Implementation
       Guide V1.0b (page 23/46):
       The transaction identifier is used to associate the future
       response with the request. */
    rsp[0] = sft->t_id >> 8;
    rsp[1] = sft->t_id & 0x00ff;

    /* Protocol Mc */
    rsp[2] = 0;
    rsp[3] = 0;

    /* Length will be set later by send_msg (4 and 5) */

    /* The slave ID is copied from the indication */
    rsp[6] = sft->slave;
    rsp[7] = sft->function;

    return _MC_TCP_PRESET_RSP_LENGTH;        /*孔--mc TCP当前回复预设长度，定义为8，为什么是8？？？？*/
}


int _mc_tcp_prepare_response_tid(const uint8_t *req, int *req_length)
{
    return (req[0] << 8) + req[1];      /*孔--这个函数是干什么用的？？？？？？？？？？？还有一个参数没用？？*/
}

int _mc_tcp_send_msg_pre(uint8_t *req, int req_length)
{
    /* Substract the header length to the message length */
    //int mbap_length = req_length - 6;

    //req[4] = mbap_length >> 8;
    //req[5] = mbap_length & 0x00FF;       
	int mbap_length = req_length - 9;
	
	req[7] = mbap_length & 0x00ff;
	req[8] = mbap_length >> 8;	   

    return req_length;
}
                                                 /*孔--下面ssize_t前面有定义就是int*/
ssize_t _mc_tcp_send(mc_t *ctx, const uint8_t *req, int req_length)
{
    /* MSG_NOSIGNAL
       Requests not to send SIGPIPE on errors on stream oriented
       sockets when the other end breaks the connection.  The EPIPE
       error is still returned. */
    return send(ctx->s, (const char*)req, req_length, MSG_NOSIGNAL);  //孔--返回发送数据的总数
}                       /*孔--MSG_NOSIGNAL宏定义为0*/

ssize_t _mc_tcp_recv(mc_t *ctx, uint8_t *rsp, int rsp_length) {
    return recv(ctx->s, (char *)rsp, rsp_length, 0);
}                       /*孔--recv这个函数在哪定义的？？？？？？？？？？？？？？，函数什么意思和作用？？？？？？？*/

int _mc_tcp_check_integrity(mc_t *ctx, uint8_t *msg, const int msg_length)
{
    return msg_length;
}

int _mc_tcp_pre_check_confirmation(mc_t *ctx, const uint8_t *req,
                                       const uint8_t *rsp, int rsp_length)
{
    /* Check TID */
    //if (req[0] != rsp[0] || req[1] != rsp[1]) {
    //    if (ctx->debug) {
    //        fprintf(stderr, "Invalid TID received 0x%X (not 0x%X)\n",
    //                (rsp[0] << 8) + rsp[1], (req[0] << 8) + req[1]);
    //    }               /*孔--以上定义打印调试信息，后期详细研究？？？？？？？？？？？？？？*/
    //    errno = EMBBADDATA;
    //    return -1;
    //} else {
    //    return 0;
    //}

    int i , err = 0;             //孔--只是检验报文头对不对
	for (i = 0; i < ctx->backend->header_length - 4; i++ ){
		
		if (req[i] != rsp[i]) {
            if (ctx->debug) {
                fprintf(stderr, "Received wrong rsp[%d]\n", i);        
            }              
            errno = EMBBADDATA;
            err = -1;
        } else 
			{
             err = 0;
		    }
    
    }
	return err;
}

static int _mc_tcp_set_ipv4_options(int s)
{
    int rc;
    int option;

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                    (const void *)&option, sizeof(int));
    if (rc == -1) {        /*孔--setsockopt这个函数是什么作用什么意思？？？？？？？？？？？*/
        return -1;
    }

#ifndef OS_WIN32
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
    /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    rc = setsockopt(s, IPPROTO_IP, IP_TOS,
                    (const void *)&option, sizeof(int));
    if (rc == -1) {
        return -1;
    }
#endif

    return 0;
}

/* Establishes a mc TCP connection with a Mc server. */
static int _mc_tcp_connect(mc_t *ctx)    
{
    int rc;
    struct sockaddr_in addr;                        /*孔--sockaddr_in是一个结构体，已经包含了定义它的头文件*/
    mc_tcp_t *ctx_tcp = ctx->backend_data;

#ifdef OS_WIN32
    if (_mc_tcp_init_win32() == -1) {
        return -1;
    }
#endif

    ctx->s = socket(PF_INET, SOCK_STREAM, 0);
    if (ctx->s == -1) {
        return -1;
    }

    rc = _mc_tcp_set_ipv4_options(ctx->s);
    if (rc == -1) {
        close(ctx->s);
        return -1;
    }

    if (ctx->debug) {
        printf("Connecting to %s\n", ctx_tcp->ip);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ctx_tcp->port);
    addr.sin_addr.s_addr = inet_addr(ctx_tcp->ip);
    rc = connect(ctx->s, (struct sockaddr *)&addr,
                 sizeof(struct sockaddr_in));
    if (rc == -1) {
        close(ctx->s);
        return -1;
    }

    return 0;
}

/* Establishes a mc TCP PI connection with a Mc server. */
static int _mc_tcp_pi_connect(mc_t *ctx)
{
    int rc;
    struct addrinfo *ai_list;
    struct addrinfo *ai_ptr;
    struct addrinfo ai_hints;
    mc_tcp_pi_t *ctx_tcp_pi = ctx->backend_data;

#ifdef OS_WIN32
    if (_mc_tcp_init_win32() == -1) {
        return -1;
    }
#endif

    memset(&ai_hints, 0, sizeof(ai_hints));
#ifdef AI_ADDRCONFIG
    ai_hints.ai_flags |= AI_ADDRCONFIG;
#endif
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_addr = NULL;
    ai_hints.ai_canonname = NULL;
    ai_hints.ai_next = NULL;

    ai_list = NULL;
    rc = getaddrinfo(ctx_tcp_pi->node, ctx_tcp_pi->service,
                     &ai_hints, &ai_list);
    if (rc != 0)
        return rc;

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        int s;

        s = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (s < 0)
            continue;

        if (ai_ptr->ai_family == AF_INET)
            _mc_tcp_set_ipv4_options(s);

        rc = connect(s, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (rc != 0) {
            close(s);
            continue;
        }

        ctx->s = s;
        break;
    }

    freeaddrinfo(ai_list);

    if (ctx->s < 0) {
        return -1;
    }

    return 0;
}

/* Closes the network connection and socket in TCP mode */
void _mc_tcp_close(mc_t *ctx)     
{
    shutdown(ctx->s, SHUT_RDWR);     /*孔--SHUT_RDWR定义为2，找时间研究shutdown和close（closesocket）？？？？？？？*/
    close(ctx->s);
}

int _mc_tcp_flush(mc_t *ctx)
{
    int rc;
    int rc_sum = 0;

    do {
        /* Extract the garbage from the socket */
        char devnull[MC_TCP_MAX_ADU_LENGTH];
#ifndef OS_WIN32
        rc = recv(ctx->s, devnull, MC_TCP_MAX_ADU_LENGTH, MSG_DONTWAIT);
#else
        /* On Win32, it's a bit more complicated to not wait */
        fd_set rfds;
        struct timeval tv;

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(ctx->s, &rfds);
        rc = select(ctx->s+1, &rfds, NULL, NULL, &tv);
        if (rc == -1) {
            return -1;
        }

        if (rc == 1) {
            /* There is data to flush */
            rc = recv(ctx->s, devnull, MC_TCP_MAX_ADU_LENGTH, 0);
        }
#endif
        if (rc > 0) {
            rc_sum += rc;
        }
    } while (rc == MC_TCP_MAX_ADU_LENGTH);

    return rc_sum;
}

/* Listens for any request from one or many mc masters in TCP */
int mc_tcp_listen(mc_t *ctx, int nb_connection)
{
    int new_socket;
    int yes;
    struct sockaddr_in addr;
    mc_tcp_t *ctx_tcp = ctx->backend_data;

#ifdef OS_WIN32
    if (_mc_tcp_init_win32() == -1) {
        return -1;
    }
#endif

    new_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket == -1) {
        return -1;
    }

    yes = 1;
    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR,
                   (char *) &yes, sizeof(yes)) == -1) {
        close(new_socket);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    /* If the mc port is < to 1024, we need the setuid root. */
    addr.sin_port = htons(ctx_tcp->port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(new_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(new_socket);
        return -1;
    }

    if (listen(new_socket, nb_connection) == -1) {
        close(new_socket);
        return -1;
    }

    return new_socket;
}

int mc_tcp_pi_listen(mc_t *ctx, int nb_connection)
{
    int rc;
    struct addrinfo *ai_list;
    struct addrinfo *ai_ptr;
    struct addrinfo ai_hints;
    const char *node;
    const char *service;
    int new_socket;
    mc_tcp_pi_t *ctx_tcp_pi = ctx->backend_data;

    if (ctx_tcp_pi->node[0] == 0)
        node = NULL; /* == any */
    else
        node = ctx_tcp_pi->node;

    if (ctx_tcp_pi->service[0] == 0)
        service = "502";
    else
        service = ctx_tcp_pi->service;

    memset(&ai_hints, 0, sizeof (ai_hints));
    ai_hints.ai_flags |= AI_PASSIVE;
#ifdef AI_ADDRCONFIG
    ai_hints.ai_flags |= AI_ADDRCONFIG;
#endif
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_addr = NULL;
    ai_hints.ai_canonname = NULL;
    ai_hints.ai_next = NULL;

    ai_list = NULL;
    rc = getaddrinfo(node, service, &ai_hints, &ai_list);
    if (rc != 0)
        return -1;

    new_socket = -1;
    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        int s;

        s = socket(ai_ptr->ai_family, ai_ptr->ai_socktype,
                            ai_ptr->ai_protocol);
        if (s < 0) {
            if (ctx->debug) {
                perror("socket");
            }
            continue;
        } else {
            int yes = 1;
            rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                            (void *) &yes, sizeof (yes));
            if (rc != 0) {
                close(s);
                if (ctx->debug) {
                    perror("setsockopt");
                }
                continue;
            }
        }

        rc = bind(s, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (rc != 0) {
            close(s);
            if (ctx->debug) {
                perror("bind");
            }
            continue;
        }

        rc = listen(s, nb_connection);
        if (rc != 0) {
            close(s);
            if (ctx->debug) {
                perror("listen");
            }
            continue;
        }

        new_socket = s;
        break;
    }
    freeaddrinfo(ai_list);

    if (new_socket < 0) {
        return -1;
    }

    return new_socket;
}

/* On success, the function return a non-negative integer that is a descriptor
   for the accepted socket. On error, -1 is returned, and errno is set
   appropriately. */
int mc_tcp_accept(mc_t *ctx, int *socket)
{
    struct sockaddr_in addr;
    socklen_t addrlen;

    addrlen = sizeof(addr);
    ctx->s = accept(*socket, (struct sockaddr *)&addr, &addrlen);
    if (ctx->s == -1) {
        close(*socket);
        *socket = 0;
        return -1;
    }

    if (ctx->debug) {
        printf("The client connection from %s is accepted\n",
               inet_ntoa(addr.sin_addr));
    }

    return ctx->s;
}

int mc_tcp_pi_accept(mc_t *ctx, int *socket)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;

    addrlen = sizeof(addr);
    ctx->s = accept(*socket, (void *)&addr, &addrlen);
    if (ctx->s == -1) {
        close(*socket);
        *socket = 0;
    }

    if (ctx->debug) {
        printf("The client connection is accepted.\n");
    }

    return ctx->s;
}

int _mc_tcp_select(mc_t *ctx, fd_set *rfds, struct timeval *tv, int length_to_read)
{
    int s_rc;
    while ((s_rc = select(ctx->s+1, rfds, NULL, NULL, tv)) == -1) {
        if (errno == EINTR) {
            if (ctx->debug) {
                fprintf(stderr, "A non blocked signal was caught\n");
            }
            /* Necessary after an error */
            FD_ZERO(rfds);
            FD_SET(ctx->s, rfds);
        } else {
            return -1;
        }
    }

    if (s_rc == 0) {
        errno = ETIMEDOUT;
        return -1;
    }

    return s_rc;
}

int _mc_tcp_filter_request(mc_t *ctx, int slave)
{
    return 0;
}

const mc_backend_t _mc_tcp_backend = {
    _MC_BACKEND_TYPE_TCP,           /*孔--宏定义为1*/
    _MC_TCP_HEADER_LENGTH,          /*孔--宏定义为7*/
    _MC_TCP_CHECKSUM_LENGTH,        /*孔--宏定义为0*/
    MC_TCP_MAX_ADU_LENGTH,          /*孔--宏定义为260*/
    _mc_set_slave,
    _mc_tcp_build_request_basis,
    _mc_tcp_build_response_basis,
    _mc_tcp_prepare_response_tid,
    _mc_tcp_send_msg_pre,
    _mc_tcp_send,
    _mc_tcp_recv,
    _mc_tcp_check_integrity,
    _mc_tcp_pre_check_confirmation,
    _mc_tcp_connect,
    _mc_tcp_close,
    _mc_tcp_flush,
    _mc_tcp_select,
    _mc_tcp_filter_request
};


const mc_backend_t _mc_tcp_pi_backend = {
    _MC_BACKEND_TYPE_TCP,
    _MC_TCP_HEADER_LENGTH,          
    _MC_TCP_CHECKSUM_LENGTH,
    MC_TCP_MAX_ADU_LENGTH,
    _mc_set_slave,
    _mc_tcp_build_request_basis,
    _mc_tcp_build_response_basis,
    _mc_tcp_prepare_response_tid,
    _mc_tcp_send_msg_pre,
    _mc_tcp_send,
    _mc_tcp_recv,
    _mc_tcp_check_integrity,
    _mc_tcp_pre_check_confirmation,
    _mc_tcp_pi_connect,
    _mc_tcp_close,
    _mc_tcp_flush,
    _mc_tcp_select,
    _mc_tcp_filter_request
};

mc_t* mc_new_tcp(const char *ip, int port)
{
    mc_t *ctx;                       /*孔--这个结构体内容很多*/
    mc_tcp_t *ctx_tcp;               /*孔--这个结构体只有port和address*/
    size_t dest_size;                    /*孔--size_t 32位系统为无符号int*/
    size_t ret_size;

#if defined(OS_BSD)                      /*孔--BSD系统，暂不考虑*/
    /* MSG_NOSIGNAL is unsupported on *BSD so we install an ignore
       handler for SIGPIPE. */
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        /* The debug flag can't be set here... */
        fprintf(stderr, "Coud not install SIGPIPE handler.\n");
        return NULL;
    }
#endif

    ctx = (mc_t *) malloc(sizeof(mc_t));     /*孔--开辟mc_t长度的动态内存区，将地址转换*/
    _mc_init_common(ctx);                        /*孔--成mc_t*型，放在ctx里，并给这个地址里的*/
                                                     /*孔--元素进行赋值*/
    /* Could be changed after to reach a remote serial Mc device */
    ctx->slave = MC_TCP_SLAVE;           /*孔--MC_TCP_SLAVE宏定义为0xFF，覆盖初值*/

    ctx->backend = &(_mc_tcp_backend);   /*孔--用上面定义的_mc_tcp_backend为开辟的空间的元素赋值*/

    ctx->backend_data = (mc_tcp_t *) malloc(sizeof(mc_tcp_t));  
                                                     /*孔--开辟mc_tcp_t长度的动态内存区，将地址转换*/
	                                                 /*孔--成mc_tcp_t*型，放在ctx->backend_data里*/
                                                     /*孔--为何上面是用的&，而下面是一个指针？因为上面是*/
	                                                 /*孔--一个实实在在的结构体，下面只是一个类型，没有实例*/
													 
    ctx_tcp = (mc_tcp_t *)ctx->backend_data;     /*孔--‘->’的优先级高于*，所以是将上面的指针转换成*/
                                                     /*孔--mc_tcp_t*类型，并赋值*/
    dest_size = sizeof(char) * 16;                   /*孔--16个字符的长度*/
    ret_size = strlcpy(ctx_tcp->ip, ip, dest_size);  /*孔--指针赋值给数组，复制16个字符，返回值ret_size为*/
	                                                 /*孔--ip字符串的数量*/
    if (ret_size == 0) {                             /*孔--ip长度为0*/
        fprintf(stderr, "The IP string is empty\n");
        mc_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    if (ret_size >= dest_size) {                     /*孔--为什么是大于等于？？？？？？？？？？？？？？？？*/
        fprintf(stderr, "The IP string has been truncated\n");
        mc_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    ctx_tcp->port = port;

    return ctx;
}


mc_t* mc_new_tcp_pi(const char *node, const char *service)
{
    mc_t *ctx;
    mc_tcp_pi_t *ctx_tcp_pi;
    size_t dest_size;
    size_t ret_size;

    ctx = (mc_t *) malloc(sizeof(mc_t));
    _mc_init_common(ctx);

    /* Could be changed after to reach a remote serial Mc device */
    ctx->slave = MC_TCP_SLAVE;

    ctx->backend = &(_mc_tcp_pi_backend);

    ctx->backend_data = (mc_tcp_pi_t *) malloc(sizeof(mc_tcp_pi_t));
    ctx_tcp_pi = (mc_tcp_pi_t *)ctx->backend_data;

    dest_size = sizeof(char) * _MC_TCP_PI_NODE_LENGTH;
    ret_size = strlcpy(ctx_tcp_pi->node, node, dest_size);
    if (ret_size == 0) {
        fprintf(stderr, "The node string is empty\n");
        mc_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    if (ret_size >= dest_size) {
        fprintf(stderr, "The node string has been truncated\n");
        mc_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    dest_size = sizeof(char) * _MC_TCP_PI_SERVICE_LENGTH;
    ret_size = strlcpy(ctx_tcp_pi->service, service, dest_size);
    if (ret_size == 0) {
        fprintf(stderr, "The service string is empty\n");
        mc_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    if (ret_size >= dest_size) {
        fprintf(stderr, "The service string has been truncated\n");
        mc_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    return ctx;
}
