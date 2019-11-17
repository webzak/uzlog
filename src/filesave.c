#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "inc/mrec.h"
#include "inc/filesave.h"
#include "inc/utils.h"

#define HEADER_SIZE 2

char* basepath = NULL;

int ensure_dir(const char*);

/**
 * module init
 */
void filesave_init(char *savedir) {
    if (savedir) {
        basepath = savedir;
    }
}


/**
 * main entry to handle message
 */
void filesave_handle(mrec_message* m) {

    if (basepath) {
        unsigned char append = ((unsigned char*)m->body)[1];
        char* fname = (char*) m->body + HEADER_SIZE;
        int payload_start = HEADER_SIZE + strlen(fname) + 1;
        char* path = malloc(strlen(basepath) + strlen(fname) + 16 + 3);
        sprintf(path, "%s/%8lx", basepath, m->session);

        if (ensure_dir(path)) {
           sprintf(path, "%s/%8lx/%s", basepath, m->session, fname);
           FILE* f;
           if (append) {
               f = fopen(path, "a");
           } else {
               f = fopen(path, "w");
           }
           fwrite(m->body + payload_start, m->len - payload_start, 1, f);
           fflush(f);
           fclose(f);
           printf("%c:%8lx/%s\n", (append) ? 'A' : 'W', m->session, fname);
        }
        free(path);
    }
}

/**
 * cleaning on exit
 */
void filesave_shutdown() {
}
