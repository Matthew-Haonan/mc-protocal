/*
 * Copyright © 2010-2012 Stéphane Raimbault <stephane.raimbault@gmail.com>
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

#ifndef _MC_PRIVATE_H_
#define _MC_PRIVATE_H_

#ifndef _MSC_VER
# include <stdint.h>
# include <sys/time.h>
#else
# include "stdint.h"
# include <time.h>
typedef int ssize_t;
#endif
#include <sys/types.h>
#include <config.h>

#include "mc.h"

MC_BEGIN_DECLS

/* It's not really the minimal length (the real one is report slave ID
 * in RTU (4 bytes)) but it's a convenient size to use in RTU or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RTU (1) + function (1) + address (2) + number (2) + CRC (2)
 */
//#define _MIN_REQ_LENGTH 12
#define _MIN_REQ_LENGTH 21


#define _REPORT_SLAVE_ID 180

#define _MC_EXCEPTION_RSP_LENGTH 5

/* Timeouts in microsecond (0.5 s) */
#define _RESPONSE_TIMEOUT    500000
#define _BYTE_TIMEOUT        500000

/* Function codes */
#define  _READ_D      0xA8
#define  _READ_M      0x90
#define  _READ_X      0x9C
#define  _READ_Y      0x9D

#define  _WRITE_D	  0xA8
#define  _WRITE_M     0x90
#define  _WRITE_X     0x9C
#define  _WRITE_Y     0x9D


#define  _COMMAND_READ          0x0401
#define  _COMMAND_WRITE         0x1401
#define  _SUBCOMMAND_WORDS      0x0000
#define  _SUBCOMMAND_BITS       0x0001




#define _FC_READ_COILS                0x01
#define _FC_READ_DISCRETE_INPUTS      0x02
#define _FC_READ_HOLDING_REGISTERS    0x03
#define _FC_READ_INPUT_REGISTERS      0x04
#define _FC_WRITE_SINGLE_COIL         0x05
#define _FC_WRITE_SINGLE_REGISTER     0x06
#define _FC_READ_EXCEPTION_STATUS     0x07
#define _FC_WRITE_MULTIPLE_COILS      0x0F
#define _FC_WRITE_MULTIPLE_REGISTERS  0x10
#define _FC_REPORT_SLAVE_ID           0x11
#define _FC_WRITE_AND_READ_REGISTERS  0x17

typedef enum {
    _MC_BACKEND_TYPE_RTU=0,
    _MC_BACKEND_TYPE_TCP
} mc_bakend_type_t;

/* This structure reduces the number of params in functions and so
 * optimizes the speed of execution (~ 37%). */
typedef struct _sft {
    int slave;
    int function;
    int t_id;
} sft_t;

typedef struct _mc_backend {
    unsigned int backend_type;
    unsigned int header_length;
    unsigned int checksum_length;
    unsigned int max_adu_length;
    int (*set_slave) (mc_t *ctx, int slave);                          /*本结构体以下定义均为指向函数的指针*/                       
    int (*build_request_basis) (mc_t *ctx, int function, int addr,
                                int nb, uint8_t *req, int command, int subcommand);   //孔--已经增加指令子指令
    int (*build_response_basis) (sft_t *sft, uint8_t *rsp);
    int (*prepare_response_tid) (const uint8_t *req, int *req_length);
    int (*send_msg_pre) (uint8_t *req, int req_length);
    ssize_t (*send) (mc_t *ctx, const uint8_t *req, int req_length);
    ssize_t (*recv) (mc_t *ctx, uint8_t *rsp, int rsp_length);
    int (*check_integrity) (mc_t *ctx, uint8_t *msg,
                            const int msg_length);
    int (*pre_check_confirmation) (mc_t *ctx, const uint8_t *req,
                                   const uint8_t *rsp, int rsp_length);
    int (*connect) (mc_t *ctx);
    void (*close) (mc_t *ctx);
    int (*flush) (mc_t *ctx);
    int (*select) (mc_t *ctx, fd_set *rfds, struct timeval *tv, int msg_length);
    int (*filter_request) (mc_t *ctx, int slave);
} mc_backend_t;

struct _mc {
    /* Slave address */
    int slave;
    /* Socket or file descriptor */
    int s;
    int debug;
    int error_recovery;
    struct timeval response_timeout;     /*response_timeout为结构体timeval类型变量*/      
    struct timeval byte_timeout;         /*这个结构体定义在time.h中，有两个成员tv_sec和tv_usec*/      
    const mc_backend_t *backend;
    void *backend_data;
};

void _mc_init_common(mc_t *ctx);
void _error_print(mc_t *ctx, const char *context);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t dest_size);
#endif

MC_END_DECLS

#endif  /* _MC_PRIVATE_H_ */
