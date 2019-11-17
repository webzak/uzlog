#define _XOPEN_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "inc/mrec.h"
#include "inc/mlog.h"
#include "inc/filesave.h"

#define MAX_PORT 65536


void assign_signal_handler();
int get_port(const char* arg, char **port);
int get_addr(const char* arg, char **ip);
int get_path(const char* arg, char **path);
void echo_usage_info(char* arg);


char* ip = NULL;
char* port = NULL;
char* wpath = NULL;
char* fpath = NULL;

int main (int argc, char *argv[]) {

    int opt, mlog_options = 0;

    while ((opt = getopt(argc, argv, "hp:a:w:s:ocf")) != -1) {
        switch (opt) {
        case 'p':
            if(!get_port(optarg, &port)) {
                fprintf(stderr, "Wrong port: %s\n", optarg);
                exit(1);
            }
            break;
        case 'a':
            if (!get_addr(optarg, &ip)) {
                fprintf(stderr, "Wrong ip address: %s\n", optarg);
                exit(1);
            }
            break;
        case 'w':
            if (!get_path(optarg, &wpath)) {
                fprintf(stderr, "Path is not writable: %s\n", optarg);
                exit(1);
            }
            break;
        case 's':
            if (!get_path(optarg, &fpath)) {
                fprintf(stderr, "Path is not writable: %s\n", optarg);
                exit(1);
            }
            break;
        case 'o':
            mlog_options |= UZP_MLOG_STDOUT;
            break;
        case 'c':
            mlog_options |= UZP_MLOG_COLOR_OPT;
            break;
        case 'f':
            mlog_options |= UZP_MLOG_FORMAT_OPT;
            break;
        case 'h':
        default:
            echo_usage_info(argv[0]);
            exit(0);
        }
    }

    if (port == NULL) {
        fprintf(stderr, "invalid options!\n");
        echo_usage_info(argv[0]);
        exit(1);
    }

    assign_signal_handler();

    if (!mrec_start(ip, port)) {
        exit(1);
    }

    mlog_init(mlog_options, wpath);
    filesave_init(fpath);

    printf("Listening on %s:%s", (ip == NULL) ? "0.0.0.0" : ip, port);
    if (wpath != NULL) {
        printf (", writing logs to %s ", wpath);
    }
    if (fpath != NULL) {
        printf (", saving files to %s ", fpath);
    }
    printf(" ...\n");

    while (1) {
        mrec_message* m = mrec_read_next_message();
        if (m != NULL) {
            unsigned char mtype = ((unsigned char*)m->body)[0];
            if (mtype == UZP_MSG_TYPE_LOG) {
                mlog_handle(m);
            } else if (mtype == UZP_MSG_TYPE_FILESAVE) {
                filesave_handle(m);
            }
            free(m->body);
            free(m);
        }
    }
    return 0;
}


void signal_handler(int signum)
{
     mrec_shutdown();
     mlog_shutdown();
     free(ip);
     free(port);
     free(wpath);
     free(fpath);
     printf("\nGood bye!\n");
     exit(0);
}

void assign_signal_handler()
{
    static struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = signal_handler;
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);
}

int get_port(const char* arg, char **port) {

    char *endptr = NULL;

    if (arg != NULL) {
        long p = strtol(optarg, &endptr, 10);
        if (errno == 0 && *endptr == 0 && p > 0 && p <= MAX_PORT) {
            *port = malloc(strlen(arg) + 1);
            strcpy(*port, arg);
            return 1;
        }
    }
    return 0;
}

int get_addr(const char* arg, char **ip) {

    struct sockaddr_in sa;
    int ln;

    ln = strlen(arg);
    if(ln > 15 || !inet_pton(AF_INET, arg, &(sa.sin_addr))) {
        return 0;
    }
    *ip = malloc(ln + 1);
    strcpy(*ip, arg);
    return 1;
}

int get_path(const char* arg, char **path) {
    if (arg == NULL || (access(arg, W_OK) != 0)) {
        return 0;
    }
    *path = malloc(strlen(arg) + 1);
    strcpy(*path, arg);
    return 1;
}

void echo_usage_info(char* arg) {
    printf("Usage: %s -p port [-a addr] [-o] [-c] [-f] [-w path] [-s path]\n", arg);
}
