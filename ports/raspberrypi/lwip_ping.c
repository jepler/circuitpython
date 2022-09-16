/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Portions Copyright (c) 2022 Jeff Epler for Adafruit Industries
 */

#include "lwip_ping.h"
#include "lwip/ip_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/sockets.h"
#include "lwip/prot/icmp.h"
#include "lwip/prot/ip4.h"

#define PING_ID (0xC14C)
#define PING_DATA_SIZE (32)

static const ip_addr_t *ping_target;
static u16_t ping_seq_num;

/** Prepare a echo ICMP request */
static void
ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len) {
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = PING_ID;
    iecho->seqno = lwip_htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++) {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    iecho->chksum = inet_chksum(iecho, len);
}

/* Ping using the socket ip */
err_t
ping_send(int s, const ip_addr_t *addr) {
    int err;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_storage to;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

    #if LWIP_IPV6
    if (IP_IS_V6(addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(addr))) {
        /* todo: support ICMP6 echo */
        return ERR_VAL;
    }
    #endif /* LWIP_IPV6 */

    iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
    if (!iecho) {
        return ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t)ping_size);

    #if LWIP_IPV4
    if (IP_IS_V4(addr)) {
        struct sockaddr_in *to4 = (struct sockaddr_in *)&to;
        to4->sin_len = sizeof(*to4);
        to4->sin_family = AF_INET;
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
    }
    #endif /* LWIP_IPV4 */

    #if LWIP_IPV6
    if (IP_IS_V6(addr)) {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&to;
        to6->sin6_len = sizeof(*to6);
        to6->sin6_family = AF_INET6;
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(addr));
    }
    #endif /* LWIP_IPV6 */

    err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(to));

    mem_free(iecho);

    return err ? ERR_OK : ERR_VAL;
}

#define PING_RESULT(x) do { if ((x) && cb()) { return; } } while (0)

void
ping_recv(int s, bool (*cb)(void)) {
    char buf[64];
    int len;
    struct sockaddr_storage from;
    int fromlen = sizeof(from);

    while ((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
        if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
            ip_addr_t fromaddr;
            memset(&fromaddr, 0, sizeof(fromaddr));

            #if LWIP_IPV4
            if (from.ss_family == AF_INET) {
                struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
                inet_addr_to_ip4addr(ip_2_ip4(&fromaddr), &from4->sin_addr);
                IP_SET_TYPE_VAL(fromaddr, IPADDR_TYPE_V4);
            }
            #endif /* LWIP_IPV4 */

            #if LWIP_IPV6
            if (from.ss_family == AF_INET6) {
                struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
                inet6_addr_to_ip6addr(ip_2_ip6(&fromaddr), &from6->sin6_addr);
                IP_SET_TYPE_VAL(fromaddr, IPADDR_TYPE_V6);
            }
            #endif /* LWIP_IPV6 */

            LWIP_DEBUGF(PING_DEBUG, ("ping: recv "));
            ip_addr_debug_print_val(PING_DEBUG, fromaddr);
            LWIP_DEBUGF(PING_DEBUG, (" %"U32_F " ms\n", (sys_now() - ping_time)));

            /* todo: support ICMP6 echo */
            #if LWIP_IPV4
            if (IP_IS_V4_VAL(fromaddr)) {
                struct ip_hdr *iphdr;
                struct icmp_echo_hdr *iecho;

                iphdr = (struct ip_hdr *)buf;
                iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
                if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
                    /* do some ping result processing */
                    PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
                    return;
                } else {
                    LWIP_DEBUGF(PING_DEBUG, ("ping: drop\n"));
                }
            }
            #endif /* LWIP_IPV4 */
        }
        fromlen = sizeof(from);
    }

    if (len == 0) {
        LWIP_DEBUGF(PING_DEBUG, ("ping: recv - %"U32_F " ms - timeout\n", (sys_now() - ping_time)));
    }

    /* do some ping result processing */
    PING_RESULT(0);
}
