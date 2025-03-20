#define _GNU_SOURCE
#define CONFIG_H "/dev/null"
#include "busybox.h"

const char *applet_name = "env";

void perror_msg_and_die(const char *fmt, ...){ _exit(1); }
void show_usage(){ _exit(1); }

#undef strlen
int puts(const char *str){ write(1, str, strlen(str)); write(1, "\n", 1); return 0; }

#define STAND
#define env_main main
#include "env.c"
