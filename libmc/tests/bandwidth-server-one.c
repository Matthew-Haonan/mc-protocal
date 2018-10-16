/*
 * Copyright © 2008-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <mc.h>

enum {
    TCP,
    RTU
};

int main(int argc, char *argv[])
{
    int socket;
    mc_t *ctx;
    mc_mapping_t *mb_mapping;
    int rc;
    int use_backend;

     /* TCP */
    if (argc > 1) {
        if (strcmp(argv[1], "tcp") == 0) {
            use_backend = TCP;
        } else if (strcmp(argv[1], "rtu") == 0) {
            use_backend = RTU;
        } else {
            printf("Usage:\n  %s [tcp|rtu] - Mc client to measure data bandwith\n\n", argv[0]);
            exit(1);
        }
    } else {
        /* By default */
        use_backend = TCP;
    }

    if (use_backend == TCP) {
        ctx = mc_new_tcp("127.0.0.1", 1502);
        socket = mc_tcp_listen(ctx, 1);
        mc_tcp_accept(ctx, &socket);

    } else {
        ctx = mc_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1);
        mc_set_slave(ctx, 1);
        mc_connect(ctx);
    }

    mb_mapping = mc_mapping_new(MC_MAX_READ_BITS, 0,
                                    MC_MAX_READ_REGISTERS, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                mc_strerror(errno));
        mc_free(ctx);
        return -1;
    }

    for(;;) {
        uint8_t query[MC_TCP_MAX_ADU_LENGTH];

        rc = mc_receive(ctx, query);
        if (rc >= 0) {
            mc_reply(ctx, query, rc, mb_mapping);
        } else {
            /* Connection closed by the client or server */
            break;
        }
    }

    printf("Quit the loop: %s\n", mc_strerror(errno));

    mc_mapping_free(mb_mapping);
    close(socket);
    mc_free(ctx);

    return 0;
}
