#include "pstree.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "jerrno.h"
#include "variable_array.h"

#define MAX_LINE_LENGTH 256

struct option long_options[] = {
    {"show-pids", no_argument, NULL, 'p'},
    {"version", no_argument, NULL, 'V'},
    {"numberic_sort", no_argument, NULL, 'n'},
    {0, 0, 0, 0}
};

static int32_t check_args(int argc, char *argv[])
{
    int option_index = 0;
    int opt;
    g_config = 0;
    while ((opt = getopt_long(argc, argv, "pVn", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'p':
            g_config |= CONFIG_SHOW_PIDS;
            break;
        case 'V':
            g_config |= CONFIG_VERSION;
            break;
        case 'n':
            g_config |= CONFIG_NUMBERIC_SORT;
            break;
        default:
            return E_UNKNOWN_PARA;
        }
    }
    return OK;
}

static int32_t get_pid_max(void)
{
    int32_t pid_max = -1;
    int32_t ret;
    FILE *fp = fopen(PID_MAX_PATH, "r");
    if (fp == NULL) {
        printf("[%s] open %s fail.\n", __func__, PID_MAX_PATH);
        return E_NOK;
    }
    if (fscanf(fp, "%d", &pid_max) != 1) {
        printf("[%s] scanf pid_max fail.\n", __func__);
        pid_max = -1;
    }

    (void)fclose(fp);
    return pid_max;
}

static void parse_status_file(FILE *filep, struct process *proc)
{
    char line[MAX_LINE_LENGTH] = {0};

    while (fgets(line, sizeof(line) - 1, filep) != NULL) {
        if (strstr(line, "PPid:") == line) {
            sscanf(line, "PPid: %u", &proc->ppid);
        }
        if (strstr(line, "Name:") == line) {
            sscanf(line, "Name: %s", proc->p_name);
        }
    }
}

static jerrno_t get_process_msg(struct process *proc)
{
    FILE *filep = NULL;
    char fpath[PATH_MAX] = {0};
    jerrno_t ret;

    ret = snprintf(fpath, sizeof(fpath) - 1, PROC_STATUS_FILENAME, proc->pid);
    if (ret < 0) {
        fprintf(stderr, "[%s] get status path of pid %d fail\n", __func__, proc->pid);
        return E_NOK;
    }

    filep = fopen(fpath, "r");
    if (filep == NULL) {
        fprintf(stderr, "[%s] open file %s fail: %s\n", __func__, fpath, strerror(errno));
        return E_NOK;
    }

    parse_status_file(filep, proc);

    fclose(filep);
    return OK;
}

static struct variable_array *get_process_array(void)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat statbuf;
    char fullpath[PATH_MAX] = {0};
    jerrno_t ret;
    int64_t pid;

    struct variable_array *ps = (struct variable_array *)variable_array_init(sizeof(struct process));
    if (ps == NULL) {
        return NULL;
    }

    dir = opendir(PROC_PATH);
    if (dir == NULL) {
        printf("open dir fail.\n");
        return NULL;
    }

    /* loop all pid files in /proc/ */
    while ((entry = readdir(dir)) != NULL) {
        pid = strtol(entry->d_name, NULL, 10);
        /* not pid dir */
        if (pid <= 0) {
            continue;
        }

        struct process proc = {0};
        proc.pid = pid;
        ret = get_process_msg(&proc);
        if (ret != OK) {
            free(ps);
            ps = NULL;
            break;
        }

        ret = variable_array_add(ps, &proc);
        if (ret != OK) {
            free(ps);
            ps = NULL;
            break;
        }
    }

    closedir(dir);
    return ps;
}

static jerrno_t setting_its_par(struct variable_array *ps,  struct process *proc)
{
    uint32_t proc_index = ARRAY_INDEX(ps, proc);
    uint32_t ppid = proc->ppid;
    struct process *p = NULL;
    jerrno_t ret;
    ARRAY_LOOP(p, ps) {
        if (p->pid == proc->ppid) {
            if (p->pchildren_index == NULL) {
                p->pchildren_index = variable_array_init(sizeof(proc_index));
            }
            if (p->pchildren_index == NULL) {
                fprintf(stderr, "[%s] malloc for children of pid: %d fail\n", __func__, p->pid);
                return E_NOMEM;
            }
            ret = variable_array_add(p->pchildren_index, &proc_index);
            if (ret != OK) {
                fprintf(stderr, "[%s] add child for pid %d fail\n", __func__, p->pid);
                return ret;
            }
        }
    }
    return OK;
}

static jerrno_t complete_children_of_each_process(struct variable_array *ps)
{
    struct process *p = NULL;
    jerrno_t ret;
    ARRAY_LOOP(p, ps) {
        ret = setting_its_par(ps, p);
        if (ret != OK) {
            fprintf(stderr, "[%s] set parent for pid %d fail\n", __func__, p->pid);
            return ret;
        }
    }
    return OK;
}

static void release_proc_array(struct variable_array *ps)
{
    struct process *p = NULL;
    ARRAY_LOOP(p, ps) {
        variable_array_release(p->pchildren_index);
    }
    variable_array_release(ps);
}

static struct variable_array *get_procs(void)
{
    jerrno_t ret;
    /* get all process messages, without setting their children */
    struct variable_array *ps = get_process_array();
    if (ps == NULL) {
        return NULL;
    }

    /* test */
    // struct process *p;
    // ARRAY_LOOP(p, ps) {
        // printf("pid: %u, pname: %s, ppid: %u\n", p->pid, p->p_name, p->ppid);
    // }

    /* setting each process's children */
    ret = complete_children_of_each_process(ps);
    if (ret != OK) {
        fprintf(stderr, "[%s] complete children of each process fail\n", __func__);
        release_proc_array(ps);
        ps = NULL;
    }
    /* test */
    printf("complete_children_of_each_process ok\n");

    return ps;
}

static void print_process(struct variable_array *ps, struct process *proc, uint32_t tab_num)
{
    struct variable_array *pchildren_index = proc->pchildren_index;
    uint32_t *pindex;
    struct process *p = NULL;
    for (uint32_t i = 0; i < tab_num; i++) {
        printf("\t");
    }
    printf("%s(%u)\n", proc->p_name, proc->pid);
    if (pchildren_index == NULL) {
        return;
    }
    ARRAY_LOOP(pindex, pchildren_index) {
        p = (struct process *)variable_array_get(ps, *pindex);
        print_process(ps, p, tab_num + 1);
    }
}

static jerrno_t output_pstree(struct variable_array *ps)
{
    print_process(ps, (struct process *)variable_array_get(ps, 0), 0);
    return OK;
}

static inline void init_config(void)
{
    g_config = 0;
}

int main(int argc, char *argv[])
{
    jerrno_t ret;
    struct variable_array *ps;

    init_config();

    ret = check_args(argc, argv);
    if (ret < 0) {
        printf("parse arguments error, please check your input.\n");
        return ret;
    }

    if(g_config & CONFIG_VERSION) {
        printf("pstree by joel, version: %s\n", PSTREE_VERSION);
        return OK;
    }

    ps = get_procs();
    if (ps == NULL) {
        printf("get pids failed.\n");
        return E_NOK;
    }

    ret = output_pstree(ps);

    release_proc_array(ps);

    return ret;
}