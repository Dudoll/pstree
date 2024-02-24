#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <ftw.h>
#include <linux/limits.h>
#include <linux/stat.h>
#include "variable_array.h"

#define TRUE 1
#define FALSE 0

#define MAX_NAME_LEN 256
#define PROC_PATH "/proc"
#define PROC_STATUS_FILENAME "/proc/%d/status"
#define PID_MAX_PATH "/proc/sys/kernel/pid_max"
#define PSTREE_VERSION "V0.0.0"

static int g_config;
#define CONFIG_SHOW_PIDS        0x1
#define CONFIG_NUMBERIC_SORT    0x2
#define CONFIG_VERSION          0x4

#define LOOP_ONCE(X) do { \
                            X; \
                        } while(0)

struct process {
    int pid;
    int ppid;
    char p_name[MAX_NAME_LEN];
    // struct process *children;
    struct variable_array *pchildren_index;
};
