#ifndef _FILESAVE_H_
#define _FILESAVE_H_
#include "mrec.h"

#define UZP_MSG_TYPE_FILESAVE 2

void filesave_init(char* savedir);
void filesave_handle(mrec_message* m);
void filesave_shutdown();

#endif
