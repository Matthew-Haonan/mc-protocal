/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
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

#ifndef _MC_TCP_H_
#define _MC_TCP_H_

#include "mc.h"

MC_BEGIN_DECLS

#if defined(_WIN32) && !defined(__CYGWIN__)
/* Win32 with MinGW, supplement to <errno.h> */
#include <winsock2.h>
#define ECONNRESET   WSAECONNRESET
#define ECONNREFUSED WSAECONNREFUSED
#define ETIMEDOUT    WSAETIMEDOUT
#define ENOPROTOOPT  WSAENOPROTOOPT
#endif

#define MC_TCP_DEFAULT_PORT   502
#define MC_TCP_SLAVE         0xFF

/* Mc_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * TCP MC ADU = 253 bytes + MBAP (7 bytes) = 260 bytes
 */
#define MC_TCP_MAX_ADU_LENGTH  260

mc_t* mc_new_tcp(const char *ip_address, int port);
int mc_tcp_listen(mc_t *ctx, int nb_connection);
int mc_tcp_accept(mc_t *ctx, int *socket);

mc_t* mc_new_tcp_pi(const char *node, const char *service);
int mc_tcp_pi_listen(mc_t *ctx, int nb_connection);
int mc_tcp_pi_accept(mc_t *ctx, int *socket);

MC_END_DECLS

#endif /* _MC_TCP_H_ */
