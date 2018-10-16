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

#ifndef _MC_H_
#define _MC_H_

/* Add this for macros that defined unix flavor */
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#ifndef _MSC_VER
#include <stdint.h>
#include <sys/time.h>
#else
#include "stdint.h"
#include <time.h>
#endif

#include "mc-version.h"

#ifdef  __cplusplus                            /*孔--如果定义了_cplusplus,说明这是C++程序，因为这个变量是C++宏自定义的。*/
# define MC_BEGIN_DECLS  extern "C" {      /*孔--如果是C++编译程序，则被花括号括起来的部分按照C语言进行编译*/
# define MC_END_DECLS    }                 /*孔--若不是C++，则花括号的地方变成空*/
#else 
# define MC_BEGIN_DECLS
# define MC_END_DECLS
#endif

MC_BEGIN_DECLS

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef ON
#define ON 1
#endif

#define MC_BROADCAST_ADDRESS    0

/* Mc_Application_Protocol_V1_1b.pdf (chapter 6 section 1 page 12)
 * Quantity of Coils to read (2 bytes): 1 to 2000 (0x7D0)
 * (chapter 6 section 11 page 29)
 * Quantity of Coils to write (2 bytes): 1 to 1968 (0x7B0)
 */
#define MC_MAX_READ_BITS              2000
#define MC_MAX_WRITE_BITS             1968

/* Mc_Application_Protocol_V1_1b.pdf (chapter 6 section 3 page 15)
 * Quantity of Registers to read (2 bytes): 1 to 125 (0x7D)
 * (chapter 6 section 12 page 31)
 * Quantity of Registers to write (2 bytes) 1 to 123 (0x7B)
 * (chapter 6 section 17 page 38)
 * Quantity of Registers to write in R/W registers (2 bytes) 1 to 121 (0x79)
 */
#define MC_MAX_READ_REGISTERS          125
#define MC_MAX_WRITE_REGISTERS         123
#define MC_MAX_RW_WRITE_REGISTERS      121

/* Random number to avoid errno conflicts */
#define MC_ENOBASE 112345678                 /*孔--随机数，但为什么是112345678？？？*/ 

/* Protocol exceptions */
enum {
    MC_EXCEPTION_ILLEGAL_FUNCTION = 0x01,    /*孔--枚举类型默认以后从第一个值依次递增1*/ 
    MC_EXCEPTION_ILLEGAL_DATA_ADDRESS,       /*孔--所以这个值为0x02*/ 
    MC_EXCEPTION_ILLEGAL_DATA_VALUE,
    MC_EXCEPTION_SLAVE_OR_SERVER_FAILURE,
    MC_EXCEPTION_ACKNOWLEDGE,
    MC_EXCEPTION_SLAVE_OR_SERVER_BUSY,
    MC_EXCEPTION_NEGATIVE_ACKNOWLEDGE,
    MC_EXCEPTION_MEMORY_PARITY,
    MC_EXCEPTION_NOT_DEFINED,
    MC_EXCEPTION_GATEWAY_PATH,
    MC_EXCEPTION_GATEWAY_TARGET,
    MC_EXCEPTION_MAX
};

#define EMBXILFUN  (MC_ENOBASE + MC_EXCEPTION_ILLEGAL_FUNCTION)
#define EMBXILADD  (MC_ENOBASE + MC_EXCEPTION_ILLEGAL_DATA_ADDRESS)
#define EMBXILVAL  (MC_ENOBASE + MC_EXCEPTION_ILLEGAL_DATA_VALUE)
#define EMBXSFAIL  (MC_ENOBASE + MC_EXCEPTION_SLAVE_OR_SERVER_FAILURE)
#define EMBXACK    (MC_ENOBASE + MC_EXCEPTION_ACKNOWLEDGE)
#define EMBXSBUSY  (MC_ENOBASE + MC_EXCEPTION_SLAVE_OR_SERVER_BUSY)
#define EMBXNACK   (MC_ENOBASE + MC_EXCEPTION_NEGATIVE_ACKNOWLEDGE)
#define EMBXMEMPAR (MC_ENOBASE + MC_EXCEPTION_MEMORY_PARITY)
#define EMBXGPATH  (MC_ENOBASE + MC_EXCEPTION_GATEWAY_PATH)
#define EMBXGTAR   (MC_ENOBASE + MC_EXCEPTION_GATEWAY_TARGET)

/* Native libmc error codes */
#define EMBBADCRC  (EMBXGTAR + 1)
#define EMBBADDATA (EMBXGTAR + 2)
#define EMBBADEXC  (EMBXGTAR + 3)
#define EMBUNKEXC  (EMBXGTAR + 4)
#define EMBMDATA   (EMBXGTAR + 5)

extern const unsigned int libmc_version_major;
extern const unsigned int libmc_version_minor;
extern const unsigned int libmc_version_micro;

typedef struct _mc mc_t;

typedef struct {
    int nb_bits;
    int nb_input_bits;
    int nb_input_registers;
    int nb_registers;
    uint8_t *tab_bits;
    uint8_t *tab_input_bits;
    uint16_t *tab_input_registers;
    uint16_t *tab_registers;
} mc_mapping_t;         /* 孔--定义结构体mc_mapping_t，包含以上元素*/

typedef enum
{
    MC_ERROR_RECOVERY_NONE          = 0,
    MC_ERROR_RECOVERY_LINK          = (1<<1),      /*孔--1左移1位是2，实际上就是*2，比*2快*/
    MC_ERROR_RECOVERY_PROTOCOL      = (1<<2),      /*孔--1左移2位是4*/
} mc_error_recovery_mode;          /* 孔--重命名一个enum为mc_error_recovery_mode，包含以上元素*/

int mc_set_slave(mc_t* ctx, int slave);
int mc_set_error_recovery(mc_t *ctx, mc_error_recovery_mode error_recovery);
void mc_set_socket(mc_t *ctx, int socket);
int mc_get_socket(mc_t *ctx);

void mc_get_response_timeout(mc_t *ctx, struct timeval *timeout);
void mc_set_response_timeout(mc_t *ctx, const struct timeval *timeout);

void mc_get_byte_timeout(mc_t *ctx, struct timeval *timeout);
void mc_set_byte_timeout(mc_t *ctx, const struct timeval *timeout);

int mc_get_header_length(mc_t *ctx);

int mc_connect(mc_t *ctx);
void mc_close(mc_t *ctx);

void mc_free(mc_t *ctx);

int mc_flush(mc_t *ctx);
void mc_set_debug(mc_t *ctx, int boolean);

const char *mc_strerror(int errnum);            /*本函数为返回指针值的函数*/

int mc_read_bits(mc_t *ctx, int addr, int nb, uint8_t *dest);
int mc_read_input_bits(mc_t *ctx, int addr, int nb, uint8_t *dest);
int mc_read_registers(mc_t *ctx, int addr, int nb, uint16_t *dest);
int mc_read_input_registers(mc_t *ctx, int addr, int nb, uint16_t *dest);
int mc_write_bit(mc_t *ctx, int coil_addr, int status);
int mc_write_register(mc_t *ctx, int reg_addr, int value);
int mc_write_bits(mc_t *ctx, int addr, int nb, const uint8_t *data);
int mc_write_registers(mc_t *ctx, int addr, int nb, const uint16_t *data);
int mc_write_and_read_registers(mc_t *ctx, int write_addr, int write_nb,
                                    const uint16_t *src, int read_addr, int read_nb,
                                    uint16_t *dest);
int mc_report_slave_id(mc_t *ctx, uint8_t *dest);

mc_mapping_t* mc_mapping_new(int nb_coil_status, int nb_input_status,
                                     int nb_holding_registers, int nb_input_registers);
                                                          /*孔--本函数为返回指针值的函数，指针类型为结构体*/
void mc_mapping_free(mc_mapping_t *mb_mapping);

int mc_send_raw_request(mc_t *ctx, uint8_t *raw_req, int raw_req_length);

int mc_receive(mc_t *ctx, uint8_t *req);
int mc_receive_from(mc_t *ctx, int sockfd, uint8_t *req);

int mc_receive_confirmation(mc_t *ctx, uint8_t *rsp);

int mc_reply(mc_t *ctx, const uint8_t *req,
                 int req_length, mc_mapping_t *mb_mapping);
int mc_reply_exception(mc_t *ctx, const uint8_t *req,
                           unsigned int exception_code);

/**
 * UTILS FUNCTIONS
 **/

#define MC_GET_HIGH_BYTE(data) (((data) >> 8) & 0xFF)   /*孔--data右移8位，然后按位与，得到高字节*/
#define MC_GET_LOW_BYTE(data) ((data) & 0xFF)            /*孔--data按位与，得到低字节*/
#define MC_GET_INT32_FROM_INT16(tab_int16, index) ((tab_int16[(index)] << 16) + tab_int16[(index) + 1])
#define MC_GET_INT16_FROM_INT8(tab_int8, index) ((tab_int8[(index)] << 8) + tab_int8[(index) + 1])
#define MC_SET_INT16_TO_INT8(tab_int8, index, value) \
    do { \
        tab_int8[(index)] = (value) >> 8;  \
        tab_int8[(index) + 1] = (value) & 0xFF; \
    } while (0)
                                                             /*孔--上边这三个函数不知道干什么的？？？？*/
void mc_set_bits_from_byte(uint8_t *dest, int index, const uint8_t value);
void mc_set_bits_from_bytes(uint8_t *dest, int index, unsigned int nb_bits,
                                const uint8_t *tab_byte);
uint8_t mc_get_byte_from_bits(const uint8_t *src, int index, unsigned int nb_bits);
float mc_get_float(const uint16_t *src);
void mc_set_float(float f, uint16_t *dest);

#include "mc-tcp.h"                                      /*孔--跑这才包含这两个头文件*/
#include "mc-rtu.h"

MC_END_DECLS

#endif  /* _MC_H_ */
