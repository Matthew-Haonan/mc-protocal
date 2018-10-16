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
 *
 *
 * This library implements the Mc protocol.
 * http://libmc.org/
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include <config.h>

#include "mc.h"
#include "mc-private.h"         /*孔-- .h里面包含了mc-version.h,这里面包含了mc-private.h  */

/* Internal use */
#define MSG_LENGTH_UNDEFINED -1

/* Exported version */
const unsigned int libmc_version_major = LIBMC_VERSION_MAJOR;    /*孔-- 是否这个值现在为（3）？？？ */
const unsigned int libmc_version_minor = LIBMC_VERSION_MINOR;
const unsigned int libmc_version_micro = LIBMC_VERSION_MICRO;

/* Max between RTU and TCP max adu length (so TCP) */
#define MAX_MESSAGE_LENGTH 260

/* 3 steps are used to parse the query */
typedef enum {
    //_STEP_FUNCTION,                                //孔--注释掉了，MC没有功能码
    _STEP_META,
    _STEP_DATA
} _step_t;                                            /* 孔--重命名一个enum为_step_t，包含以上元素*/


const char *mc_strerror(int errnum) {             /* 孔--定义函数内容，声明是在mc.h里。errnum是形参*/
    switch (errnum) {                                 /* 孔--本函数为返回指针值的函数*/
    case EMBXILFUN:                                   /* 孔--虽然没有break语句，但一旦符合条件后不会顺序执行*/
        return "Illegal function";                    /* 孔--因为return后就结束了*/
    case EMBXILADD:                                   /* 孔--前面这些数都是112345678+一个数*/
        return "Illegal data address";                /* 孔--如果前面这些都不满足，则返回strerror(errnum)*/
    case EMBXILVAL:                                   /* 孔--这个函数是<errno.h>里面的，会根据errno这个系统变量值返回*/
        return "Illegal data value";                  /* 孔--对应的字符串，后面这个函数调用的时候实参就是errno*/
    case EMBXSFAIL:                                   /* 孔--如果程序到了default，errno为默认值0，则是success*/
        return "Slave device or server failure";
    case EMBXACK:
        return "Acknowledge";
    case EMBXSBUSY:
        return "Slave device or server is busy";
    case EMBXNACK:
        return "Negative acknowledge";
    case EMBXMEMPAR:
        return "Memory parity error";
    case EMBXGPATH:
        return "Gateway path unavailable";
    case EMBXGTAR:
        return "Target device failed to respond";
    case EMBBADCRC:
        return "Invalid CRC";
    case EMBBADDATA:
        return "Invalid data";
    case EMBBADEXC:
        return "Invalid exception code";
    case EMBMDATA:
        return "Too many data";
    default:                                          /* 孔--为什么这个指针函数的返回值是字符串呢？？？？？？？*/
        return strerror(errnum);                      /* 孔--strerror这特么又是在哪定义的？？？？前面已解决*/                     
    }
}

void _error_print(mc_t *ctx, const char *context)
{
    if (ctx->debug) {                                            /*孔--如果这个结构体的debug为1，则有以下输出*/
        fprintf(stderr, "ERROR %s", mc_strerror(errno));
        if (context != NULL) {
            fprintf(stderr, ": %s\n", context);                  /*孔--\n为换行*/
        } else {
            fprintf(stderr, "\n");
        }
    }
}

int _sleep_and_flush(mc_t *ctx)
{
#ifdef _WIN32
    /* usleep doesn't exist on Windows */
    Sleep((ctx->response_timeout.tv_sec * 1000) +           /*孔--这个.tv_sec又特么哪来的？？？？？*/
          (ctx->response_timeout.tv_usec / 1000));          /*孔--已解决，声明处备注了。WIN32转换成了ms*/ 
#else
    /* usleep source code */
    struct timespec request, remaining;
    request.tv_sec = ctx->response_timeout.tv_sec;
    request.tv_nsec = ((long int)ctx->response_timeout.tv_usec % 1000000)
        * 1000;
    while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
        request = remaining;
#endif
    return mc_flush(ctx);
}

int mc_flush(mc_t *ctx)
{
    int rc = ctx->backend->flush(ctx);
    if (rc != -1 && ctx->debug) {
        printf("%d bytes flushed\n", rc);
    }
    return rc;
}

/* Computes the length of the expected response */
static unsigned int compute_response_length_from_request(mc_t *ctx, uint8_t *req)
{
    //int length;
    //const int offset = ctx->backend->header_length;

    //switch (req[offset]) {                              /*孔--req[offset]是什么意思？？？？req是数组？？？？*/
    //case _FC_READ_COILS:                                /*孔--以上已解决，req[offset]等价于*（req+offset），后期补课！！*/
    //case _FC_READ_DISCRETE_INPUTS: {
    //    /* Header + nb values (code from write_bits) */
    //    int nb = (req[offset + 3] << 8) | req[offset + 4];
    //    length = 2 + (nb / 8) + ((nb % 8) ? 1 : 0);
    //}
    //    break;
    //case _FC_WRITE_AND_READ_REGISTERS:
    //case _FC_READ_HOLDING_REGISTERS:
    //case _FC_READ_INPUT_REGISTERS:
    //    /* Header + 2 * nb values */
    //    length = 2 + 2 * (req[offset + 3] << 8 | req[offset + 4]);
    //    break;
    //case _FC_READ_EXCEPTION_STATUS:
    //    length = 3;
    //    break;
    //case _FC_REPORT_SLAVE_ID:
    //    /* The response is device specific (the header provides the
    //       length) */
    //    return MSG_LENGTH_UNDEFINED;
    //default:
    //    length = 5;
    //}

    //return offset + length + ctx->backend->checksum_length;      /*孔--预期的返回报文长度*/


	int length, nb;
    const int offset = ctx->backend->header_length;
	const int command = req[offset + 1] << 8 | 
		                req[offset];
	const int subcommand = req[offset + 3] << 8 | 
		                   req[offset + 2];
    if (command == 0x0401)
		{
		if (subcommand ==0x0000)
			{
			length = ((req[offset + 9] << 8) | req[offset + 8]) * 2;
			}
		else
			{
			 nb = ((req[offset + 9] << 8) | req[offset + 8]);
			length = (nb / 2) + ((nb % 2) ? 1 : 0);
			}
    	}
	else
		{
		length = 0;
		}
	
	return offset + length + ctx->backend->checksum_length;

}

/* Sends a request/response */
static int send_msg(mc_t *ctx, uint8_t *msg, int msg_length)      
{
    int rc;
    int i;

    msg_length = ctx->backend->send_msg_pre(msg, msg_length);   

    if (ctx->debug) {
        for (i = 0; i < msg_length; i++)
            printf("[%.2X]", msg[i]);     //*孔--输出2位16进制数据
        printf("\n");
    }

    /* In recovery mode, the write command will be issued until to be
       successful! Disabled by default. */
    do {
        rc = ctx->backend->send(ctx, msg, msg_length);    //孔--返回发送数据的总数
        if (rc == -1) {                                   //*孔--如果发送失败，这个错误信息就算不debug也打印
            _error_print(ctx, NULL);
            if (ctx->error_recovery & MC_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;    //*孔--error_recovery初始化定义为0，后面不懂？？？？？？？？
                
                if ((errno == EBADF || errno == ECONNRESET || errno == EPIPE)) {
                    mc_close(ctx);
                    mc_connect(ctx);
                } else {
                    _sleep_and_flush(ctx);
                }
                errno = saved_errno;
            }
        }
    } while ((ctx->error_recovery & MC_ERROR_RECOVERY_LINK) &&
             rc == -1);

    if (rc > 0 && rc != msg_length) {
        errno = EMBBADDATA;
        return -1;
    }

    return rc;
}

int mc_send_raw_request(mc_t *ctx, uint8_t *raw_req, int raw_req_length)
{
    sft_t sft;
    uint8_t req[MAX_MESSAGE_LENGTH];
    int req_length;

    if (raw_req_length < 2) {
        /* The raw request must contain function and slave at least */
        errno = EINVAL;
        return -1;
    }

    sft.slave = raw_req[0];
    sft.function = raw_req[1];
    /* The t_id is left to zero */
    sft.t_id = 0;
    /* This response function only set the header so it's convenient here */
    req_length = ctx->backend->build_response_basis(&sft, req);

    if (raw_req_length > 2) {
        /* Copy data after function code */
        memcpy(req + req_length, raw_req + 2, raw_req_length - 2);
        req_length += raw_req_length - 2;
    }

    return send_msg(ctx, req, req_length);
}

/*
    ---------- Request     Indication ----------
    | Client | ---------------------->| Server |
    ---------- Confirmation  Response ----------
*/

typedef enum {
    /* Request message on the server side */
    MSG_INDICATION,
    /* Request message on the client side */
    MSG_CONFIRMATION
} msg_type_t;

/* Computes the length to read after the function received */
//已删除

/* Computes the length to read after the meta information (address, count, etc) */
static int compute_data_length_after_meta(mc_t *ctx, uint8_t *msg,
                                          msg_type_t msg_type)
{
    //int function = msg[ctx->backend->header_length];
    
	int end_code = (msg[ctx->backend->header_length - 1] << 8) | 
	               (msg[ctx->backend->header_length - 2]);             //结束码定义
    int length = 0;

    if (msg_type == MSG_INDICATION) {
        //switch (function) {
        //case _FC_WRITE_MULTIPLE_COILS:
        //case _FC_WRITE_MULTIPLE_REGISTERS:
        //    length = msg[ctx->backend->header_length + 5];
        //    break;
        //case _FC_WRITE_AND_READ_REGISTERS:
        //    length = msg[ctx->backend->header_length + 9];
        //    break;
        //default:
        //    length = 0;
        //}
    } else {
        /* MSG_CONFIRMATION */
        //if (function <= _FC_READ_INPUT_REGISTERS ||
        //    function == _FC_REPORT_SLAVE_ID ||
        //    function == _FC_WRITE_AND_READ_REGISTERS) {      //只有这几种功能码后面的一位是数据长度，如果不是这
        //    length = msg[ctx->backend->header_length + 1];   //几种功能码，后面的也不需要读了，长度就是0
        if (end_code == 0 ) {      
            length = ((msg[ctx->backend->header_length - 3] << 8) | 
	               msg[ctx->backend->header_length - 4]) - 2;     //响应数据长度-2，也即从结束码后面的长度 
        } else {                                             
            length = 0;
        }
    }

    length += ctx->backend->checksum_length;      //貌似是给有校验字节的报文预留的

    return length;
}


/* Waits a response from a mc server or a request from a mc client.
   This function blocks if there is no replies (3 timeouts).

   The function shall return the number of received characters and the received
   message in an array of uint8_t if successful. Otherwise it shall return -1
   and errno is set to one of the values defined below:
   - ECONNRESET
   - EMBBADDATA
   - EMBUNKEXC
   - ETIMEDOUT
   - read() or recv() error codes
*/

static int receive_msg(mc_t *ctx, uint8_t *msg, msg_type_t msg_type)
{
    int rc;
    fd_set rfds;
    struct timeval tv;
    struct timeval *p_tv;
    int length_to_read;
    int msg_length = 0;
    _step_t step;

    if (ctx->debug) {
        if (msg_type == MSG_INDICATION) {
            printf("Waiting for a indication...\n");
        } else {
            printf("Waiting for a confirmation...\n");
        }
    }

    /* Add a file descriptor to the set */
    FD_ZERO(&rfds);                   //孔--这两个函数是什么东西？？？？？？？
    FD_SET(ctx->s, &rfds);

    /* We need to analyse the message step by step.  At the first step, we want
     * to reach the function code because all packets contain this
     * information. */
    //step = _STEP_FUNCTION;
	step = _STEP_META;               //孔--第一步就是META
    //length_to_read = ctx->backend->header_length + 1;
	length_to_read = ctx->backend->header_length ;

    if (msg_type == MSG_INDICATION) {
        /* Wait for a message, we don't know when the message will be
         * received */
        p_tv = NULL;
    } else {
        tv.tv_sec = ctx->response_timeout.tv_sec;       //孔--初始化时定义为0
        tv.tv_usec = ctx->response_timeout.tv_usec;     //孔--初始化时定义为0.5s
        p_tv = &tv;
    }

    while (length_to_read != 0) {
        rc = ctx->backend->select(ctx, &rfds, p_tv, length_to_read);
        if (rc == -1) {
            _error_print(ctx, "select");
            if (ctx->error_recovery & MC_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if (errno == ETIMEDOUT) {
                    _sleep_and_flush(ctx);
                } else if (errno == EBADF) {
                    mc_close(ctx);
                    mc_connect(ctx);
                }
                errno = saved_errno;
            }
            return -1;
        }

        rc = ctx->backend->recv(ctx, msg + msg_length, length_to_read);//recv函数返回实际接受的数量，正常也就是
        if (rc == 0) {                                                 //length_to_read
            errno = ECONNRESET;
            rc = -1;
        }

        if (rc == -1) {
            _error_print(ctx, "read");
            if ((ctx->error_recovery & MC_ERROR_RECOVERY_LINK) &&
                (errno == ECONNRESET || errno == ECONNREFUSED ||
                 errno == EBADF)) {
                int saved_errno = errno;
                mc_close(ctx);
                mc_connect(ctx);
                /* Could be removed by previous calls */
                errno = saved_errno;
            }
            return -1;
        }

        /* Display the hex code of each character received */
        if (ctx->debug) {
            int i;
            for (i=0; i < rc; i++)
                printf("<%.2X>", msg[msg_length + i]);
        }

        /* Sums bytes received */
        msg_length += rc;   //我理解这实际上是相对初始地址的偏移量
        /* Computes remaining bytes */
        length_to_read -= rc;   //因为整个while循环是用length_to_read不等于0来循环的，所以要有这个语句

        if (length_to_read == 0) {
            switch (step) {
            //case _STEP_FUNCTION:
                /* Function code position */
            //    length_to_read = compute_meta_length_after_function(
            //        msg[ctx->backend->header_length],
            //        msg_type);
            //    if (length_to_read != 0) {
            //        step = _STEP_META;
            //        break;
            //    } /* else switches straight to the next step */
            case _STEP_META:
                length_to_read = compute_data_length_after_meta(
                    ctx, msg, msg_type);
                if ((msg_length + length_to_read) > ctx->backend->max_adu_length) {
					                         //如果已经读到的和要读的和大于260，报错，这个和实际的报文长
                    errno = EMBBADDATA;
                    _error_print(ctx, "too many data");
                    return -1;
                }
                step = _STEP_DATA;
                break;
            default:
                break;
            }
        }

        if (length_to_read > 0 && ctx->byte_timeout.tv_sec != -1) {         //这一段又没有读懂？？？？？？？
            /* If there is no character in the buffer, the allowed timeout
               interval between two consecutive bytes is defined by
               byte_timeout */
            tv.tv_sec = ctx->byte_timeout.tv_sec;
            tv.tv_usec = ctx->byte_timeout.tv_usec;
            p_tv = &tv;
        }
    }

    if (ctx->debug)
        printf("\n");

    return ctx->backend->check_integrity(ctx, msg, msg_length);
}

/* Receive the request from a mc master */
int mc_receive(mc_t *ctx, uint8_t *req)
{
    return receive_msg(ctx, req, MSG_INDICATION);
}

/* Receives the confirmation.

   The function shall store the read response in rsp and return the number of
   values (bits or words). Otherwise, its shall return -1 and errno is set.

   The function doesn't check the confirmation is the expected response to the
   initial request.
*/
int mc_receive_confirmation(mc_t *ctx, uint8_t *rsp)
{
    return receive_msg(ctx, rsp, MSG_CONFIRMATION);
}

static int check_confirmation(mc_t *ctx, uint8_t *req,
                              uint8_t *rsp, int rsp_length)
{
    int rc;
    int rsp_length_computed;
    const int offset = ctx->backend->header_length;

    if (ctx->backend->pre_check_confirmation) {
        rc = ctx->backend->pre_check_confirmation(ctx, req, rsp, rsp_length);//孔--这个函数只验证报文头
        if (rc == -1) {
            if (ctx->error_recovery & MC_ERROR_RECOVERY_PROTOCOL) { //孔--错误恢复模式并且确实错误了
                _sleep_and_flush(ctx);
            }
            return -1;        //孔--如果报文头就不对，这个程序段就直接返回-1，结束了
        }
    }

    rsp_length_computed = compute_response_length_from_request(ctx, req);
	                                                    //孔--按照请求报文计算返回总长度
										
    /* Check length */
    //if (rsp_length == rsp_length_computed ||              //孔--1.实际长度和计算的长度相等
    //    rsp_length_computed == MSG_LENGTH_UNDEFINED) {
    //    int req_nb_value;
    //    int rsp_nb_value;
    //    const int function = rsp[offset];

    //    /* Check function code */
    //    if (function != req[offset]) {                    //孔--2.功能码相等
    //        if (ctx->debug) {
    //            fprintf(stderr,
    //                    "Received function not corresponding to the request (%d != %d)\n",
    //                    function, req[offset]);
    //        }
    //        if (ctx->error_recovery & MC_ERROR_RECOVERY_PROTOCOL) {
     //           _sleep_and_flush(ctx);
    //        }
    //        errno = EMBBADDATA;
    //        return -1;                                   //孔--功能码不等就结束
    //    }

    //    /* Check the number of values is corresponding to the request */
    //    switch (function) {
    //    case _FC_READ_COILS:                              //孔--3.核对两个报文的内容里面的字节数量
    //    case _FC_READ_DISCRETE_INPUTS:                    //部分是否相等
    //        /* Read functions, 8 values in a byte (nb
    //         * of values in the request and byte count in
    //         * the response. */
    //        req_nb_value = (req[offset + 3] << 8) + req[offset + 4];
    //        req_nb_value = (req_nb_value / 8) + ((req_nb_value % 8) ? 1 : 0);
    //        rsp_nb_value = rsp[offset + 1];
    //        break;
    //    case _FC_WRITE_AND_READ_REGISTERS:
    //    case _FC_READ_HOLDING_REGISTERS:
    //    case _FC_READ_INPUT_REGISTERS:
    //        /* Read functions 1 value = 2 bytes */
    //        req_nb_value = (req[offset + 3] << 8) + req[offset + 4];
    //        rsp_nb_value = (rsp[offset + 1] / 2);
    //        break;
    //    case _FC_WRITE_MULTIPLE_COILS:
    //    case _FC_WRITE_MULTIPLE_REGISTERS:
    //        /* N Write functions */
    //        req_nb_value = (req[offset + 3] << 8) + req[offset + 4];
    //        rsp_nb_value = (rsp[offset + 3] << 8) | rsp[offset + 4];
    //        break;
    //    case _FC_REPORT_SLAVE_ID:
    //        /* Report slave ID (bytes received) */
    //        req_nb_value = rsp_nb_value = rsp[offset + 1];
    //        break;
    //    default:
    //        /* 1 Write functions & others */
    //        req_nb_value = rsp_nb_value = 1;
    //    }



		 /* Check length */
    if (rsp_length == rsp_length_computed ||              //孔--1.实际长度和计算的长度相等
        rsp_length_computed == MSG_LENGTH_UNDEFINED) 
        {
        int req_nb_value;                   //请求-有效数据的个数，按位的单位是字节，字的是字
        int rsp_nb_value;                   //返回-有效数据的个数，按位的单位是字节，字的是字
        
		int nb;
        const int offset = ctx->backend->header_length;    //灏：MC的TCP报头为11字节
	    const int command = req[offset + 1] << 8 | 
		                    req[offset];
	    const int subcommand = req[offset + 3] << 8 | 
		                       req[offset + 2];
    if (command == 0x0401) //灏：读操作
		{
		if (subcommand ==0x0000)            //孔--读字
			{
			req_nb_value = ((req[offset + 9] << 8) | req[offset + 8]);   //字
			nb = rsp[offset - 3] << 8 | rsp[offset - 4];				//灏：以下nb均为字节数
			rsp_nb_value = (nb - 2) / 2;
			}
		else
			{
			nb = ((req[offset + 9] << 8) | req[offset + 8]);        //字节
			req_nb_value = (nb / 2) + ((nb % 2) ? 1 : 0);
			nb = rsp[offset - 3] << 8 | rsp[offset - 4];
			rsp_nb_value = nb - 2;
			}
    	}
	else     //灏：写操作
		{
		req_nb_value= 0;
		rsp_nb_value=0;
		}



        if (req_nb_value == rsp_nb_value) {
            rc = rsp_nb_value;                 
        } else {
            if (ctx->debug) {
                fprintf(stderr,
                        "Quantity not corresponding to the request (%d != %d)\n",
                        rsp_nb_value, req_nb_value);
            }

            if (ctx->error_recovery & MC_ERROR_RECOVERY_PROTOCOL) {
                _sleep_and_flush(ctx);
            }

            errno = EMBBADDATA;
            rc = -1;
        }
    } else if (rsp_length == (offset + 2 + ctx->backend->checksum_length) &&
               req[offset] == (rsp[offset] - 0x80)) {    //另外一个功能码情况
        /* EXCEPTION CODE RECEIVED */

        int exception_code = rsp[offset + 1];
        if (exception_code < MC_EXCEPTION_MAX) {
            errno = MC_ENOBASE + exception_code;
        } else {
            errno = EMBBADEXC;
        }
        _error_print(ctx, NULL);
        rc = -1;
    } else {
        if (ctx->debug) {
            fprintf(stderr,
                    "Message length not corresponding to the computed length (%d != %d)\n",
                    rsp_length, rsp_length_computed);
        }
        if (ctx->error_recovery & MC_ERROR_RECOVERY_PROTOCOL) {
            _sleep_and_flush(ctx);
        }
        errno = EMBBADDATA;
        rc = -1;
    }
	return rc;
}
	
	static int response_io_status(int address, int nb,
								  uint8_t *tab_io_status,
								  uint8_t *rsp, int offset)
	{
		int shift = 0;
		int byte = 0;
		int i;
	
		for (i = address; i < address+nb; i++) {
			byte |= tab_io_status[i] << shift;
        if (shift == 7) {
            /* Byte is full */
            rsp[offset++] = byte;
            byte = shift = 0;
        } else {
            shift++;
        }
    }

    if (shift != 0)
        rsp[offset++] = byte;

    return offset;
}

/* Build the exception response */
static int response_exception(mc_t *ctx, sft_t *sft,
                              int exception_code, uint8_t *rsp)
{
    int rsp_length;

    sft->function = sft->function + 0x80;
    rsp_length = ctx->backend->build_response_basis(sft, rsp);

    /* Positive exception code */
    rsp[rsp_length++] = exception_code;

    return rsp_length;
}

/* Send a response to the received request.
   Analyses the request and constructs a response.

   If an error occurs, this function construct the response
   accordingly.
*/
int mc_reply(mc_t *ctx, const uint8_t *req,
                 int req_length, mc_mapping_t *mb_mapping)
{
    int offset = ctx->backend->header_length;
    int slave = req[offset - 1];
    int function = req[offset];
    uint16_t address = (req[offset + 1] << 8) + req[offset + 2];
    uint8_t rsp[MAX_MESSAGE_LENGTH];
    int rsp_length = 0;
    sft_t sft;

    if (ctx->backend->filter_request(ctx, slave) == 1) {
        /* Filtered */
        return 0;
    }

    sft.slave = slave;
    sft.function = function;
    sft.t_id = ctx->backend->prepare_response_tid(req, &req_length);

    switch (function) {
    case _FC_READ_COILS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MC_MAX_READ_BITS < nb) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal nb of values %d in read_bits (max %d)\n",
                        nb, MC_MAX_READ_BITS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_bits) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in read_bits\n",
                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = (nb / 8) + ((nb % 8) ? 1 : 0);
            rsp_length = response_io_status(address, nb,
                                            mb_mapping->tab_bits,
                                            rsp, rsp_length);
        }
    }
        break;
    case _FC_READ_DISCRETE_INPUTS: {
        /* Similar to coil status (but too many arguments to use a
         * function) */
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MC_MAX_READ_BITS < nb) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal nb of values %d in read_input_bits (max %d)\n",
                        nb, MC_MAX_READ_BITS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_input_bits) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in read_input_bits\n",
                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = (nb / 8) + ((nb % 8) ? 1 : 0);
            rsp_length = response_io_status(address, nb,
                                            mb_mapping->tab_input_bits,
                                            rsp, rsp_length);
        }
    }
        break;
    case _FC_READ_HOLDING_REGISTERS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MC_MAX_READ_REGISTERS < nb) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal nb of values %d in read_holding_registers (max %d)\n",
                        nb, MC_MAX_READ_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_registers) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in read_registers\n",
                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int i;

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = nb << 1;
            for (i = address; i < address + nb; i++) {
                rsp[rsp_length++] = mb_mapping->tab_registers[i] >> 8;
                rsp[rsp_length++] = mb_mapping->tab_registers[i] & 0xFF;
            }
        }
    }
        break;
    case _FC_READ_INPUT_REGISTERS: {
        /* Similar to holding registers (but too many arguments to use a
         * function) */
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MC_MAX_READ_REGISTERS < nb) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal number of values %d in read_input_registers (max %d)\n",
                        nb, MC_MAX_READ_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_input_registers) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in read_input_registers\n",
                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int i;

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = nb << 1;
            for (i = address; i < address + nb; i++) {
                rsp[rsp_length++] = mb_mapping->tab_input_registers[i] >> 8;
                rsp[rsp_length++] = mb_mapping->tab_input_registers[i] & 0xFF;
            }
        }
    }
        break;
    case _FC_WRITE_SINGLE_COIL:
        if (address >= mb_mapping->nb_bits) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in write_bit\n",
                        address);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int data = (req[offset + 3] << 8) + req[offset + 4];

            if (data == 0xFF00 || data == 0x0) {
                mb_mapping->tab_bits[address] = (data) ? ON : OFF;
                memcpy(rsp, req, req_length);
                rsp_length = req_length;
            } else {
                if (ctx->debug) {
                    fprintf(stderr,
                            "Illegal data value %0X in write_bit request at address %0X\n",
                            data, address);
                }
                rsp_length = response_exception(
                    ctx, &sft,
                    MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
            }
        }
        break;
    case _FC_WRITE_SINGLE_REGISTER:
        if (address >= mb_mapping->nb_registers) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in write_register\n",
                        address);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int data = (req[offset + 3] << 8) + req[offset + 4];

            mb_mapping->tab_registers[address] = data;
            memcpy(rsp, req, req_length);
            rsp_length = req_length;
        }
        break;
    case _FC_WRITE_MULTIPLE_COILS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MC_MAX_WRITE_BITS < nb) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal number of values %d in write_bits (max %d)\n",
                        nb, MC_MAX_WRITE_BITS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_bits) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in write_bits\n",
                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            /* 6 = byte count */
            mc_set_bits_from_bytes(mb_mapping->tab_bits, address, nb, &req[offset + 6]);

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            /* 4 to copy the bit address (2) and the quantity of bits */
            memcpy(rsp + rsp_length, req + rsp_length, 4);
            rsp_length += 4;
        }
    }
        break;
    case _FC_WRITE_MULTIPLE_REGISTERS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];

        if (nb < 1 || MC_MAX_WRITE_REGISTERS < nb) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal number of values %d in write_registers (max %d)\n",
                        nb, MC_MAX_WRITE_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_registers) {
            if (ctx->debug) {
                fprintf(stderr, "Illegal data address %0X in write_registers\n",
                        address + nb);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int i, j;
            for (i = address, j = 6; i < address + nb; i++, j += 2) {
                /* 6 and 7 = first value */
                mb_mapping->tab_registers[i] =
                    (req[offset + j] << 8) + req[offset + j + 1];
            }

            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            /* 4 to copy the address (2) and the no. of registers */
            memcpy(rsp + rsp_length, req + rsp_length, 4);
            rsp_length += 4;
        }
    }
        break;
    case _FC_REPORT_SLAVE_ID: {
        int str_len;
        int byte_count_pos;

        rsp_length = ctx->backend->build_response_basis(&sft, rsp);
        /* Skip byte count for now */
        byte_count_pos = rsp_length++;
        rsp[rsp_length++] = _REPORT_SLAVE_ID;
        /* Run indicator status to ON */
        rsp[rsp_length++] = 0xFF;
        /* LMB + length of LIBMC_VERSION_STRING */
        str_len = 3 + strlen(LIBMC_VERSION_STRING);
        memcpy(rsp + rsp_length, "LMB" LIBMC_VERSION_STRING, str_len);
        rsp_length += str_len;
        rsp[byte_count_pos] = rsp_length - byte_count_pos - 1;
    }
        break;
    case _FC_READ_EXCEPTION_STATUS:
        if (ctx->debug) {
            fprintf(stderr, "FIXME Not implemented\n");
        }
        errno = ENOPROTOOPT;
        return -1;
        break;

    case _FC_WRITE_AND_READ_REGISTERS: {
        int nb = (req[offset + 3] << 8) + req[offset + 4];
        uint16_t address_write = (req[offset + 5] << 8) + req[offset + 6];
        int nb_write = (req[offset + 7] << 8) + req[offset + 8];
        int nb_write_bytes = req[offset + 9];

        if (nb_write < 1 || MC_MAX_RW_WRITE_REGISTERS < nb_write ||
            nb < 1 || MC_MAX_READ_REGISTERS < nb ||
            nb_write_bytes != nb_write * 2) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal nb of values (W%d, R%d) in write_and_read_registers (max W%d, R%d)\n",
                        nb_write, nb,
                        MC_MAX_RW_WRITE_REGISTERS, MC_MAX_READ_REGISTERS);
            }
            rsp_length = response_exception(
                ctx, &sft,
                MC_EXCEPTION_ILLEGAL_DATA_VALUE, rsp);
        } else if ((address + nb) > mb_mapping->nb_registers ||
                   (address_write + nb_write) > mb_mapping->nb_registers) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Illegal data read address %0X or write address %0X write_and_read_registers\n",
                        address + nb, address_write + nb_write);
            }
            rsp_length = response_exception(ctx, &sft,
                                            MC_EXCEPTION_ILLEGAL_DATA_ADDRESS, rsp);
        } else {
            int i, j;
            rsp_length = ctx->backend->build_response_basis(&sft, rsp);
            rsp[rsp_length++] = nb << 1;

            /* Write first.
               10 and 11 are the offset of the first values to write */
            for (i = address_write, j = 10; i < address_write + nb_write; i++, j += 2) {
                mb_mapping->tab_registers[i] =
                    (req[offset + j] << 8) + req[offset + j + 1];
            }

            /* and read the data for the response */
            for (i = address; i < address + nb; i++) {
                rsp[rsp_length++] = mb_mapping->tab_registers[i] >> 8;
                rsp[rsp_length++] = mb_mapping->tab_registers[i] & 0xFF;
            }
        }
    }
        break;

    default:
        rsp_length = response_exception(ctx, &sft,
                                        MC_EXCEPTION_ILLEGAL_FUNCTION,
                                        rsp);
        break;
    }

    return send_msg(ctx, rsp, rsp_length);
}

int mc_reply_exception(mc_t *ctx, const uint8_t *req,
                           unsigned int exception_code)
{
    int offset = ctx->backend->header_length;
    int slave = req[offset - 1];
    int function = req[offset];
    uint8_t rsp[MAX_MESSAGE_LENGTH];
    int rsp_length;
    int dummy_length = 99;
    sft_t sft;

    if (ctx->backend->filter_request(ctx, slave) == 1) {
        /* Filtered */
        return 0;
    }

    sft.slave = slave;
    sft.function = function + 0x80;;
    sft.t_id = ctx->backend->prepare_response_tid(req, &dummy_length);
    rsp_length = ctx->backend->build_response_basis(&sft, rsp);

    /* Positive exception code */
    if (exception_code < MC_EXCEPTION_MAX) {
        rsp[rsp_length++] = exception_code;
        return send_msg(ctx, rsp, rsp_length);
    } else {
        errno = EINVAL;
        return -1;
    }
}

/* Reads IO status */
static int read_io_status(mc_t *ctx, int function,
                          int addr, int nb, uint8_t *dest)
{
    int rc;
    int req_length;

    uint8_t req[_MIN_REQ_LENGTH];
    uint8_t rsp[MAX_MESSAGE_LENGTH];

    req_length = ctx->backend->build_request_basis(ctx, function, addr, nb, req, _COMMAND_READ, _SUBCOMMAND_BITS);

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        int i, temp, bit;
        int pos = 0;
        int offset;
        int offset_end;

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
        if (rc == -1)
            return -1;

        //offset = ctx->backend->header_length + 2;
        //offset_end = offset + rc;
        //for (i = offset; i < offset_end; i++) {
            /* Shift reg hi_byte to temp */
        //    temp = rsp[i];

        //    for (bit = 0x01; (bit & 0xff) && (pos < nb);) {  //灏：每次把一位读到一个dest字节里，整8位的部分通过(bit & 0xff)跳出循环，剩余的部分通过(pos < nb)跳出循环
        //        dest[pos++] = (temp & bit) ? TRUE : FALSE;
        //        bit = bit << 1;                     
        //    }

        //}

		offset = ctx->backend->header_length;
        offset_end = offset + rc;
        for (i = offset; i < offset_end; i++) {
            /* Shift reg hi_byte to temp */
            temp = ((rsp[i] << 4) & 0xf0) | ((rsp[i] >> 4) & 0x0f);  //灏：字节数据的高四位和低四位互换

            for (bit = 0x01; (bit & 0xff) && (pos < nb);) {
                dest[pos++] = (temp & bit) ? TRUE : FALSE;
                bit = bit << 4;           //灏：每次左移4位，每8位执行两次
            }

        }
    }

    return rc;
}

/* Use binary mode to read the boolean status of bits(M registers) and sets the array elements
   in the destination to TRUE or FALSE (single bits). */
int mc_read_bits_bm(mc_t *ctx, int addr, int nb, uint8_t *dest)
{
    int rc;

    if (nb > MC_MAX_READ_BITS) {      //灏：mc协议中一次最大采集位数为2000
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Too many bits requested (%d > %d)\n",
                    nb, MC_MAX_READ_BITS);
        }
        errno = EMBMDATA;
        return -1;
    }

    rc = read_io_status(ctx, _READ_M, addr, nb, dest); //灏：funciton改为MC的读位

    if (rc == -1)
        return -1;
    else
        return nb;
}


/* Same as mc_read_bits but reads the remote device input table */
int mc_read_input_bits(mc_t *ctx, int addr, int nb, uint8_t *dest)
{
    int rc;

    if (nb > MC_MAX_READ_BITS) {
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Too many discrete inputs requested (%d > %d)\n",
                    nb, MC_MAX_READ_BITS);
        }
        errno = EMBMDATA;
        return -1;
    }

    rc = read_io_status(ctx, _FC_READ_DISCRETE_INPUTS, addr, nb, dest);

    if (rc == -1)
        return -1;
    else
        return nb;
}

/* Reads the data from a remove device and put that data into an array */
static int read_registers(mc_t *ctx, int function, int addr, int nb,
                          uint16_t *dest)
{

	int rc;
    int req_length;
    uint8_t req[_MIN_REQ_LENGTH];        //孔--最小请求长度定义为12，req[12]，已经改为21
    uint8_t rsp[MAX_MESSAGE_LENGTH];     //孔--最大消息长度定义为260，rsp[260],这只是中间存放的数组，最终响应
                                         //孔--在dest里面
    if (nb > MC_MAX_READ_REGISTERS) {   /*孔--最大读寄存器长度定义为125*/
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Too many registers requested (%d > %d)\n",
                    nb, MC_MAX_READ_REGISTERS);
        }
        errno = EMBMDATA;
        return -1;
    }

    req_length = ctx->backend->build_request_basis(ctx, function, addr, nb, req, _COMMAND_READ, _SUBCOMMAND_WORDS);

    rc = send_msg(ctx, req, req_length);        //孔--返回发送数据的总数
    if (rc > 0) {                               //孔--如果发送的数据大于0                       
        int offset;
        int i;

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);       //这个rc是实际的字的个数
        if (rc == -1)
            return -1;

        offset = ctx->backend->header_length;

        for (i = 0; i < rc; i++) {                             //孔--字节组合成字
            /* shift reg hi_byte to temp OR with lo_byte */
            dest[i] = (rsp[offset + 1 + (i << 1)] << 8) |
                rsp[offset + (i << 1)];
        }
    }

    return rc;
}

/* Uses binary mode to read the D registers of remote device and put the data into an
   array */
int mc_read_registers_bd(mc_t *ctx, int addr, int nb, uint16_t *dest)
{
    int status;

    if (nb > MC_MAX_READ_REGISTERS) {                /* 孔--如果数量大于最大数量，且调试，则打印输出*/
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Too many registers requested (%d > %d)\n",
                    nb, MC_MAX_READ_REGISTERS);
        }
        errno = EMBMDATA;
        return -1;
    }

    /*status = read_registers(ctx, _FC_READ_HOLDING_REGISTERS,
                            addr, nb, dest);*/
	status = read_registers(ctx, _READ_D, addr, nb, dest);
    return status;
}

/* Reads the input registers of remote device and put the data into an array */
int mc_read_input_registers(mc_t *ctx, int addr, int nb,
                                uint16_t *dest)
{
    int status;

    if (nb > MC_MAX_READ_REGISTERS) {
        fprintf(stderr,
                "ERROR Too many input registers requested (%d > %d)\n",
                nb, MC_MAX_READ_REGISTERS);
        errno = EMBMDATA;
        return -1;
    }

    status = read_registers(ctx, _FC_READ_INPUT_REGISTERS,
                            addr, nb, dest);

    return status;
}

/* Write a value to the specified register of the remote device.
   Used by write_bit and write_register */
static int write_single(mc_t *ctx, int function, int addr, int value)
{
    int rc;
    int req_length;
    uint8_t req[_MIN_REQ_LENGTH];

    req_length = ctx->backend->build_request_basis(ctx, function, addr, value, req, _COMMAND_WRITE, _SUBCOMMAND_WORDS);

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        /* Used by write_bit and write_register */
        uint8_t rsp[MAX_MESSAGE_LENGTH];

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
    }

    return rc;
}

/* Turns ON or OFF a single bit of the remote device */
int mc_write_bit(mc_t *ctx, int addr, int status)
{
    return write_single(ctx, _FC_WRITE_SINGLE_COIL, addr,
                        status ? 0xFF00 : 0);
}

/* Writes a value in one register of the remote device */
int mc_write_register(mc_t *ctx, int addr, int value)
{
    return write_single(ctx, _FC_WRITE_SINGLE_REGISTER, addr, value);
}

/* Uses binary mdoe to write the bits to M registers of the array in the remote device */
int mc_write_bits_bm(mc_t *ctx, int addr, int nb, const uint8_t *src)
{
    int rc;
    int i;
    int byte_count;        //孔--此变量原先的意思是mc报文里的一个字节内容，MC可理解为
    int req_length;        //孔--后面要写的数字解的数量，不在报文内
    int bit_check = 0;
    int pos = 0;

    uint8_t req[MAX_MESSAGE_LENGTH];

    if (nb > MC_MAX_WRITE_BITS) {
        if (ctx->debug) {
            fprintf(stderr, "ERROR Writing too many bits (%d > %d)\n",
                    nb, MC_MAX_WRITE_BITS);
        }
        errno = EMBMDATA;
        return -1;
    }

    req_length = ctx->backend->build_request_basis(ctx,
                                                   _WRITE_M,
                                                   addr, nb, req, _COMMAND_WRITE, _SUBCOMMAND_BITS);
	                                               //孔--按实际更改功能码、指令、子指令
    //byte_count = (nb / 8) + ((nb % 8) ? 1 : 0);
	byte_count = (nb / 2) + ((nb % 2) ? 1 : 0);    //孔--8改为2，一个字节代表两个bits
    //req[req_length++] = byte_count;              //注释掉

    for (i = 0; i < byte_count; i++) {
        int bit;

        bit = 0x01;
        req[req_length] = 0;

        while ((bit & 0xFF) && (bit_check++ < nb)) {   //孔--src[]的内容为1 1 0 1 0这种形式，不需要调用者
                                                       //孔--自己把n个0或1组合，直接罗列
            if (src[pos++])
                req[req_length] |= bit;
            else
                req[req_length] &=~ bit;  //孔--（~ bit）为按位取反，优先级高
                                          //孔--src=1，就把对应的位置1，所以是或，别的位是和0或，不变
                                          //孔--src=0，就把对应的位置0，所以和取反后的与，别的位是和1与，不变
                                          //孔--好精妙啊
            //bit = bit << 1;
			bit = bit << 4;               //孔--左移四位
        }
		req[req_length] = req[req_length] >> 4 | req[req_length] << 4;   //孔--新增高低位交换
        req_length++;
    }

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        uint8_t rsp[MAX_MESSAGE_LENGTH];

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
    }


    return rc;
}

/*Uses binary mode to write the values from the array to the D registers of the remote device */
int mc_write_registers_bd(mc_t *ctx, int addr, int nb, const uint16_t *src)
{
    int rc;
    int i;
    int req_length;
    //int byte_count;
    uint8_t req[MAX_MESSAGE_LENGTH];

    if (nb > MC_MAX_WRITE_REGISTERS) {
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Trying to write to too many registers (%d > %d)\n",
                    nb, MC_MAX_WRITE_REGISTERS);
        }
        errno = EMBMDATA;
        return -1;
    }

    req_length = ctx->backend->build_request_basis(ctx,
                                                   _WRITE_D,
                                                   addr, nb, req, _COMMAND_WRITE, _SUBCOMMAND_WORDS);
    //byte_count = nb * 2;
    //req[req_length++] = byte_count;      /*灏：注释掉是因为MC的报文中没有“字节个数”这一项。*/

    for (i = 0; i < nb; i++) {
     //   req[req_length++] = src[i] >> 8;
     //   req[req_length++] = src[i] & 0x00FF;
     req[req_length++] = src[i] & 0x00FF;
	 req[req_length++] = src[i] >> 8;      /* MC报文中先低位，后高位 */
    }

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        uint8_t rsp[MAX_MESSAGE_LENGTH];

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
    }

    return rc;
}

/* Write multiple registers from src array to remote device and read multiple
   registers from remote device to dest array. */
int mc_write_and_read_registers(mc_t *ctx,
                                    int write_addr, int write_nb, const uint16_t *src,
                                    int read_addr, int read_nb, uint16_t *dest)

{
    int rc;
    int req_length;
    int i;
    int byte_count;
    uint8_t req[MAX_MESSAGE_LENGTH];
    uint8_t rsp[MAX_MESSAGE_LENGTH];

    if (write_nb > MC_MAX_RW_WRITE_REGISTERS) {
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Too many registers to write (%d > %d)\n",
                    write_nb, MC_MAX_RW_WRITE_REGISTERS);
        }
        errno = EMBMDATA;
        return -1;
    }

    if (read_nb > MC_MAX_READ_REGISTERS) {
        if (ctx->debug) {
            fprintf(stderr,
                    "ERROR Too many registers requested (%d > %d)\n",
                    read_nb, MC_MAX_READ_REGISTERS);
        }
        errno = EMBMDATA;
        return -1;
    }
    req_length = ctx->backend->build_request_basis(ctx,
                                                   _FC_WRITE_AND_READ_REGISTERS,
                                                   read_addr, read_nb, req, _COMMAND_READ, _SUBCOMMAND_WORDS);

    req[req_length++] = write_addr >> 8;
    req[req_length++] = write_addr & 0x00ff;
    req[req_length++] = write_nb >> 8;
    req[req_length++] = write_nb & 0x00ff;
    byte_count = write_nb * 2;
    req[req_length++] = byte_count;

    for (i = 0; i < write_nb; i++) {
        req[req_length++] = src[i] >> 8;
        req[req_length++] = src[i] & 0x00FF;
    }

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        int offset;

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
        if (rc == -1)
            return -1;

        offset = ctx->backend->header_length;

        /* If rc is negative, the loop is jumped ! */
        for (i = 0; i < rc; i++) {
            /* shift reg hi_byte to temp OR with lo_byte */
            dest[i] = (rsp[offset + 2 + (i << 1)] << 8) |
                rsp[offset + 3 + (i << 1)];
        }
    }

    return rc;
}

/* Send a request to get the slave ID of the device (only available in serial
   communication). */
int mc_report_slave_id(mc_t *ctx, uint8_t *dest)
{
    int rc;
    int req_length;
    uint8_t req[_MIN_REQ_LENGTH];

    req_length = ctx->backend->build_request_basis(ctx, _FC_REPORT_SLAVE_ID,
                                                   0, 0, req, _COMMAND_READ, _SUBCOMMAND_WORDS);

    /* HACKISH, addr and count are not used */
    req_length -= 4;

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        int i;
        int offset;
        uint8_t rsp[MAX_MESSAGE_LENGTH];

        rc = receive_msg(ctx, rsp, MSG_CONFIRMATION);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
        if (rc == -1)
            return -1;

        offset = ctx->backend->header_length + 2;

        /* Byte count, slave id, run indicator status,
           additional data */
        for (i=0; i < rc; i++) {
            dest[i] = rsp[offset + i];
        }
    }

    return rc;
}

void _mc_init_common(mc_t *ctx)
{
    /* Slave and socket are initialized to -1 */
    ctx->slave = -1;
    ctx->s = -1;

    ctx->debug = FALSE;
    ctx->error_recovery = MC_ERROR_RECOVERY_NONE;    /*孔--默认定义为0*/

    ctx->response_timeout.tv_sec = 0;
    ctx->response_timeout.tv_usec = _RESPONSE_TIMEOUT;   /*孔--默认定义为500000，就是0.5S*/

    ctx->byte_timeout.tv_sec = 0;
    ctx->byte_timeout.tv_usec = _BYTE_TIMEOUT;           /*孔--默认定义为500000，就是0.5S*/
}

/* Define the slave number */
int mc_set_slave(mc_t *ctx, int slave)
{
    return ctx->backend->set_slave(ctx, slave);
}

int mc_set_error_recovery(mc_t *ctx,
                              mc_error_recovery_mode error_recovery)
{
    /* The type of mc_error_recovery_mode is unsigned enum */
    ctx->error_recovery = (uint8_t) error_recovery;
    return 0;
}

void mc_set_socket(mc_t *ctx, int socket)
{
    ctx->s = socket;
}

int mc_get_socket(mc_t *ctx)
{
    return ctx->s;
}

/* Get the timeout interval used to wait for a response */
void mc_get_response_timeout(mc_t *ctx, struct timeval *timeout)
{
    *timeout = ctx->response_timeout;
}

void mc_set_response_timeout(mc_t *ctx, const struct timeval *timeout)
{
    ctx->response_timeout = *timeout;
}

/* Get the timeout interval between two consecutive bytes of a message */
void mc_get_byte_timeout(mc_t *ctx, struct timeval *timeout)
{
    *timeout = ctx->byte_timeout;
}

void mc_set_byte_timeout(mc_t *ctx, const struct timeval *timeout)
{
    ctx->byte_timeout = *timeout;
}

int mc_get_header_length(mc_t *ctx)
{
    return ctx->backend->header_length;
}

int mc_connect(mc_t *ctx)
{
    return ctx->backend->connect(ctx);
}

void mc_close(mc_t *ctx)
{
    if (ctx == NULL)
        return;

    ctx->backend->close(ctx);
}

void mc_free(mc_t *ctx)
{
    if (ctx == NULL)
        return;

    free(ctx->backend_data);
    free(ctx);
}

void mc_set_debug(mc_t *ctx, int boolean)
{
    ctx->debug = boolean;
}

/* Allocates 4 arrays to store bits, input bits, registers and inputs
   registers. The pointers are stored in mc_mapping structure.

   The mc_mapping_new() function shall return the new allocated structure if
   successful. Otherwise it shall return NULL and set errno to ENOMEM. */
mc_mapping_t* mc_mapping_new(int nb_bits, int nb_input_bits,
                                     int nb_registers, int nb_input_registers)
{
    mc_mapping_t *mb_mapping;

    mb_mapping = (mc_mapping_t *)malloc(sizeof(mc_mapping_t));
    if (mb_mapping == NULL) {
        return NULL;
    }

    /* 0X */
    mb_mapping->nb_bits = nb_bits;
    if (nb_bits == 0) {
        mb_mapping->tab_bits = NULL;
    } else {
        /* Negative number raises a POSIX error */
        mb_mapping->tab_bits =
            (uint8_t *) malloc(nb_bits * sizeof(uint8_t));
        if (mb_mapping->tab_bits == NULL) {
            free(mb_mapping);
            return NULL;
        }
        memset(mb_mapping->tab_bits, 0, nb_bits * sizeof(uint8_t));
    }

    /* 1X */
    mb_mapping->nb_input_bits = nb_input_bits;
    if (nb_input_bits == 0) {
        mb_mapping->tab_input_bits = NULL;
    } else {
        mb_mapping->tab_input_bits =
            (uint8_t *) malloc(nb_input_bits * sizeof(uint8_t));
        if (mb_mapping->tab_input_bits == NULL) {
            free(mb_mapping->tab_bits);
            free(mb_mapping);
            return NULL;
        }
        memset(mb_mapping->tab_input_bits, 0, nb_input_bits * sizeof(uint8_t));
    }

    /* 4X */
    mb_mapping->nb_registers = nb_registers;
    if (nb_registers == 0) {
        mb_mapping->tab_registers = NULL;
    } else {
        mb_mapping->tab_registers =
            (uint16_t *) malloc(nb_registers * sizeof(uint16_t));
        if (mb_mapping->tab_registers == NULL) {
            free(mb_mapping->tab_input_bits);
            free(mb_mapping->tab_bits);
            free(mb_mapping);
            return NULL;
        }
        memset(mb_mapping->tab_registers, 0, nb_registers * sizeof(uint16_t));
    }

    /* 3X */
    mb_mapping->nb_input_registers = nb_input_registers;
    if (nb_input_registers == 0) {
        mb_mapping->tab_input_registers = NULL;
    } else {
        mb_mapping->tab_input_registers =
            (uint16_t *) malloc(nb_input_registers * sizeof(uint16_t));
        if (mb_mapping->tab_input_registers == NULL) {
            free(mb_mapping->tab_registers);
            free(mb_mapping->tab_input_bits);
            free(mb_mapping->tab_bits);
            free(mb_mapping);
            return NULL;
        }
        memset(mb_mapping->tab_input_registers, 0,
               nb_input_registers * sizeof(uint16_t));
    }

    return mb_mapping;
}

/* Frees the 4 arrays */
void mc_mapping_free(mc_mapping_t *mb_mapping)
{
    if (mb_mapping == NULL) {
        return;
    }

    free(mb_mapping->tab_input_registers);
    free(mb_mapping->tab_registers);
    free(mb_mapping->tab_input_bits);
    free(mb_mapping->tab_bits);
    free(mb_mapping);
}

#ifndef HAVE_STRLCPY
/*
 * Function strlcpy was originally developed by
 * Todd C. Miller <Todd.Miller@courtesan.com> to simplify writing secure code.
 * See ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/strlcpy.3
 * for more information.
 *
 * Thank you Ulrich Drepper... not!
 *
 * Copy src to string dest of size dest_size.  At most dest_size-1 characters
 * will be copied.  Always NUL terminates (unless dest_size == 0).  Returns
 * strlen(src); if retval >= dest_size, truncation occurred.
 */
size_t strlcpy(char *dest, const char *src, size_t dest_size)
{
    register char *d = dest;
    register const char *s = src;
    register size_t n = dest_size;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dest, add NUL and traverse rest of src */
    if (n == 0) {
        if (dest_size != 0)
            *d = '\0'; /* NUL-terminate dest */
        while (*s++)
            ;
    }

    return (s - src - 1); /* count does not include NUL */
}
#endif
