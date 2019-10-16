#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include "inc/urec.h"
#include "inc/utils.h"

#define MAX_PACKET_SIZE 508
#define HEADER_SIZE 20


typedef struct {
    uint32_t session_high;
    uint32_t session_low;
    uint32_t id;
    uint32_t len;
    uint32_t offset;
} packet_header;


unsigned char buf[MAX_PACKET_SIZE+1];
int socketd;

int urec_listen(const char* ip, const char* port) {

    struct addrinfo hints;
    struct addrinfo *si, *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (ip == NULL) {
        hints.ai_flags = AI_PASSIVE;
    }

    int gai_result;
    if ((gai_result = getaddrinfo(ip, port, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(gai_result));
        return 0;
    }
    for (si = result; si != NULL; si = si->ai_next) {
        socketd = socket(si->ai_family, si->ai_socktype, si->ai_protocol);
        if (socketd == -1) {
            continue;
        }
        if (bind(socketd, si->ai_addr, si->ai_addrlen) == 0) {
            break;
        } else {
            perror("bind");
        }
        close(socketd);
    }
    freeaddrinfo(result);

    if (socketd == 0) {
        fprintf(stderr, "Could not bind %s:%s\n", ip, port);
        return 0;
    }
    return 1;
}


void urec_shutdown() {
    if (socketd) {
        close(socketd);
    }
}


int urec_read_packet(urec_packet* p)
{
    struct sockaddr addr; //skip checking inbound addr for now
    socklen_t addr_size = sizeof(addr);

    int nb = recvfrom(socketd, (void *)buf, MAX_PACKET_SIZE, 0, &addr, &addr_size);
    if (nb < 0) {
        perror("recvfrom");
        return -1;
    }

    if (nb <= HEADER_SIZE) {
        perror("Packet size is too small!");
        return -1;
    }

    packet_header* ph =  (packet_header*) buf;

    uint32_t sh = ntohl(ph->session_high);
    uint32_t sl = ntohl(ph->session_low);

    p->session = ((uint64_t) sh << 32) | (uint64_t) sl;
    p->id = ntohl(ph->id);
    p->len = ntohl(ph->len);
    p->offset = ntohl(ph->offset);
    p->payload = buf + HEADER_SIZE;
    p->size = nb - HEADER_SIZE;
    return nb;
}


void urec_dump_packet(urec_packet* p)
{
    utils_hexdump(buf, HEADER_SIZE);
    printf("Session:%lx ID:%u Len: %u Offset: %u Size: %u\n", p->session, p->id, p->len, p->offset, p->size);

    buf[p->size + HEADER_SIZE] = 0; // for printf
    printf("\nBody:\n%s\n", (char*) p->payload);
}
