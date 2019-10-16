#ifndef _MREC_H_
#define _MREC_H_
#include <stdint.h>
#include <time.h>


typedef struct {
    uint64_t session;
    uint32_t id;
    uint32_t len;
    void* body;
} mrec_message;

int mrec_start(const char* ip, const char* port);
void mrec_shutdown();
mrec_message*  mrec_read_next_message();
void mrec_free_message(mrec_message* m);
void mrec_dump_message(mrec_message* m);
time_t mrec_extract_session_time(const uint64_t session, struct tm** tm,  int* msecs);


#endif
