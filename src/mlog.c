#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "inc/mrec.h"
#include "inc/mlog.h"

#define HEADER_SIZE 4
#define SESSION_FG 16
#define SESSION_BG 118
#define AUTO_CLOSE_DELAY 60
#define MAX_PATH_LEN 2000
#define MAX_FILENAME_LEN 100

typedef struct logfile logfile;

struct logfile {
    uint64_t session;
    FILE* fd;
    time_t last_used;
    logfile* next;
};

struct logfile* logs;

char path[MAX_PATH_LEN + MAX_FILENAME_LEN];
int pathlen;
int opt;
uint64_t current_session;
time_t gc_last;
char sbuf[100];


void send_out(mrec_message* m, int session_changed);
void write_to_log(mrec_message* m);
void close_inactive_logs();


/**
 * module init
 */
void mlog_init(int options, char *logdir) {
    opt = options;
    if (logdir) {
        int n = strlen(logdir);
        if (n <= MAX_PATH_LEN) {
            strcpy(path, logdir);
            if (path[n - 1] != '/') {
                path[n] = '/';
                n++;
            }
            path[n] = '\0';
            pathlen = n;
        }
    }
    gc_last = time(NULL);
}


/**
 * main entry to handle message
 */
void mlog_handle(mrec_message* m) {

    int session_changed;
    if (current_session != m->session){
        current_session = m->session;
        session_changed = 1;
    } else {
        session_changed = 0;
    }
    if (opt & UZP_MLOG_STDOUT) {
        send_out(m, session_changed);
    }

    if (pathlen > 0) {
        write_to_log(m);
        time_t t = time(NULL);
        if ((t - gc_last) > AUTO_CLOSE_DELAY) {
            close_inactive_logs();
            gc_last = t;
        }
    }
}

/**
 * cleaning on exit
 */
void mlog_shutdown() {

    struct logfile* f = logs;
    while (f != NULL) {
        struct logfile* next = f->next;
        fflush(f->fd);
        fclose(f->fd);
        free(f);
        f = next;
    }
}



void show_header(mrec_message *m) {
    if (!(opt & UZP_MLOG_FORMAT_OPT)){
        return;
    }
    struct tm* tm;
    int msec;

    if (mrec_extract_session_time(m->session, &tm, &msec)) {
        strftime(sbuf, 30, "%Y-%m-%d %H:%I:%S", tm);
    } else {
        sbuf[0] = '\0';
    }
    if (opt & UZP_MLOG_COLOR_OPT) {
        printf("\x1b[38;5;%dm\x1b[48;5;%dm[%lx:%u] %s.%d\x1b[0m\n", SESSION_FG, SESSION_BG, m->session, m->id, sbuf, msec);
    } else {
       printf("[%lx:%u]\n", m->session, m->id);
    }
}


void show_body(mrec_message* m) {

    unsigned char fgcolor = ((unsigned char*)m->body)[1];
    unsigned char bgcolor = ((unsigned char*)m->body)[2];
    char* text = (char*)m->body + HEADER_SIZE;

    if (opt & UZP_MLOG_COLOR_OPT) {
        if (fgcolor > 0) {
            printf("\x1b[38;5;%um", fgcolor);
        }
        if (bgcolor > 0) {
            printf("\x1b[48;5;%um", bgcolor);
        }
    }
    if (opt & UZP_MLOG_FORMAT_OPT) {
        printf("%s", text);
    } else {
        printf("[%lx:%u] %s",  m->session, m->id, text);
    }
    if (opt & UZP_MLOG_COLOR_OPT && (bgcolor > 0 ||  fgcolor > 0)) {
        printf("\x1b[0m");
    }
    printf("\n");
}

void send_out(mrec_message* m, int session_changed) {
    if (session_changed) {
        show_header(m);
    }
    show_body(m);
}


int ensure_date_dir(const char* path) {

    struct stat s;

    int res = stat(path, &s);
    if(res == 0 && S_ISDIR(s.st_mode)) {
        if(access(path, W_OK) != 0) {
            fprintf(stderr, "Directory is not writable: %s", path);
            return 0;
        }
        return 1;
    }
    if (errno == ENOENT) {
        mkdir(path, 0755);
    } else {
        perror(path);
        return 0;
    }
    return 1;
}


struct logfile* attach_logfile(uint64_t session) {

    int msecs;
    struct tm* tm;
    struct logfile* ret;

    if (!mrec_extract_session_time(session, &tm, &msecs)) {
        return NULL;
    }
    path[pathlen] = '\0'; // restore base path
    sprintf(&path[pathlen], "%d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    if (! ensure_date_dir(path)) {
        return NULL;
    }
    sprintf(&path[pathlen+8], "/%02d%02d%02d.%d.log", tm->tm_hour, tm->tm_min, tm->tm_sec, msecs);

    FILE* f = fopen(path, "a");
    if (f == NULL) {
        return NULL;
    }
    ret = malloc(sizeof(logfile));
    ret->session = session;
    ret->fd = f;
    ret->last_used = time(NULL);
    return ret;
}


/**
 *  Lookup or autocreate logfile
 */
logfile* findlog(uint64_t session) {

    logfile* ret = NULL;
    struct logfile* f = logs;

    while (f != NULL) {
        if (f->session == session) {
            f->last_used = time(NULL);
            ret = f;
            break;
        }
        f = f->next;
    }
    if (ret == NULL) {
        f = attach_logfile(session);
        if (f) {
            f->next = logs;
            logs = f;
            ret = f;
        }
    }
    return ret;
}

/**
 * Close files for inactive sessions
 */
void close_inactive_logs() {

    time_t t = time(NULL);
    struct logfile* f = logs;
    struct logfile* prev = NULL;

    while (f != NULL) {
        struct logfile* next = f->next;
        t = time(NULL);
        if ((t - f->last_used) > AUTO_CLOSE_DELAY) {
            if (prev == NULL) {
                logs = next;
            } else {
                prev->next = next;
            }
            printf("inactive %p", f->fd);
            fflush(f->fd);
            fclose(f->fd);
            free(f);
        } else {
            prev = f;
        }
        f = next;
    }
}

/**
 * Write message to log file
 */
void write_to_log(mrec_message* m) {

    logfile* log = findlog(m->session);
    if (log == NULL) {
        return;
    }
    char* text = (char*)m->body + HEADER_SIZE;
    //fprintf(log->fd, "%s\n", text);
    fputs(text, log->fd);
    fputs("\n", log->fd);
}
