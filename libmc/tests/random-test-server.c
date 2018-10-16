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
#include <stdlib.h>
#include <errno.h>

#include <mc.h>

int main(void)
{
    int socket;
    mc_t *ctx;
    mc_mapping_t *mb_mapping;

    ctx = mc_new_tcp("127.0.0.1", 1502);
    /* mc_set_debug(ctx, TRUE); */

    mb_mapping = mc_mapping_new(500, 500, 500, 500);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                mc_strerror(errno));
        mc_free(ctx);
        return -1;
    }

    socket = mc_tcp_listen(ctx, 1);
    mc_tcp_accept(ctx, &socket);

    for (;;) {
        uint8_t query[MC_TCP_MAX_ADU_LENGTH];
        int rc;

        rc = mc_receive(ctx, query);
        if (rc != -1) {
            /* rc is the query size */
            mc_reply(ctx, query, rc, mb_mapping);
        } else {
            /* Connection closed by the client or error */
            break;
        }
    }

    printf("Quit the loop: %s\n", mc_strerror(errno));

    mc_mapping_free(mb_mapping);
    mc_close(ctx);
    mc_free(ctx);

    return 0;
}
