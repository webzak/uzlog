#ifndef _MLOG_H_
#define _MLOG_H_
#include "mrec.h"

#define UZP_MLOG_COLOR_OPT 1
#define UZP_MLOG_FORMAT_OPT 2
#define UZP_MLOG_STDOUT 4

void mlog_init(int options, char* logdir);
void mlog_handle(mrec_message* m);
void mlog_shutdown();

#endif
