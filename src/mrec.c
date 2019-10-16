#include <stdlib.h>
#include <string.h>
#include "inc/urec.h"
#include "inc/mrec.h"
#include "inc/utils.h"
#include "inc/crc32.h"

typedef struct msgin msgin;

struct msgin {
    mrec_message* msg;
    unsigned int received;
    msgin* prev;
    msgin* next;
};


// module vars
struct msgin* pool;
urec_packet p;


int mrec_start(const char* ip, const char* port) {

    pool = NULL;
    return urec_listen(ip, port);
}


void mrec_shutdown() {

    msgin* ptr = pool;

    while (ptr != NULL) {
        msgin* mi = ptr;
        ptr = ptr->next;
        free(mi->msg->body);
        free(mi->msg);
        free(mi);
    }
    urec_shutdown();
}

void mrec_free_message(mrec_message *m) {

    free(m->body);
    free(m);
}

msgin* pool_find(urec_packet* p) {

    msgin* mi = NULL;
    msgin* last = NULL;
    msgin* ptr = pool;

    // lookup for message already proceed
    while (ptr != NULL) {
        if (ptr->msg->session == p->session && ptr->msg->id == p->id) {
            mi = ptr;
            break;
        }
        last = ptr;
        ptr = ptr->next;
    }

    // if not found create new
    if (mi == NULL) {
        mrec_message* m = malloc(sizeof(mrec_message));
        m->session = p->session;
        m->id = p->id;
        m->len = p->len;
        m->body = malloc(p->len + 1);

        mi = malloc(sizeof(msgin));
        mi->msg = m;
        mi->received = 0;
        mi->prev = last;
        mi->next = NULL;

        if (pool == NULL) {
            pool = mi;
        } else {
            last->next = mi;
        }
    }
    return mi;
}

void pool_clean(msgin* mi) {

   if (mi->next != NULL) {
       mi->next->prev = mi->prev;
   } else if (mi->prev == NULL) {
       pool = NULL;
   }
   free(mi);
}

mrec_message* mrec_read_next_message() {

    msgin* mi;
    mrec_message* m;

    while (1) {
        int n = urec_read_packet(&p);
        if (n <= 0) {
            continue;
        }
        mi = pool_find(&p);
        m = mi->msg;
        memcpy((char *)m->body + p.offset, p.payload, p.size);
        mi->received += p.size;
        if(mi->received >= m->len){  //TODO change to verification by CRC
            pool_clean(mi);
            break;
        }
    }
    m->len -= 4; // cut crc32

    uint32_t crc = ntohl(*((uint32_t*) ((char*)m->body + m->len)));
    uint32_t crc_calc = crc32(0, m->body, m->len);

    ((char*) m->body)[m->len] = 0; // for char* interpretation

    if (crc != crc_calc) {
        fprintf(stderr, "CRC error: %x : %x\n", crc, crc_calc);
        return NULL;
    }
    return m;
}

time_t mrec_extract_session_time(const uint64_t session, struct tm **tm, int* msecs) {

    uint64_t unixtime = session / 1000000;
    if (unixtime < 1){
        return 0;
    }
    uint64_t ms = session % 1000000;
    *msecs = (int)ms;

    time_t t = (time_t) unixtime;
    *tm = localtime(&t);
    return t;
}


void mrec_dump_message(mrec_message* m)
{
    printf("Session:%lx ID:%u Len: %u\n", m->session, m->id, m->len);
    printf("\nBody:\n%s\n", (char*) m->body);
}
