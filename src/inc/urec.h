#ifndef _UREC_H_
#define _UREC_H_

#include <stdio.h>
#include <stdint.h>


typedef struct {
    uint64_t session;
    uint32_t id;
    uint32_t len;
    uint32_t offset;
    void* payload;
    unsigned int size;
} urec_packet;

int urec_listen(const char* ip, const char* port);
void urec_shutdown();
int urec_read_packet(urec_packet* p);
void urec_dump_packet(urec_packet* p);

#endif
