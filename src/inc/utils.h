#ifndef _UTILS_H_
#define _UTILS_H_
#include <stddef.h>
#include <netdb.h>

int ensure_dir(const char* dirpath);
void utils_hexdump(const void* data, size_t size);
//void utils_show_ai(addrinfo* ai);

#endif
