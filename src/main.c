#define _POSIX_SOURCE
#define _DEFAULT_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "conf_parse.h"

struct my_conf {
    int pid;
    int percent;
    long interval_ms;
};

static struct my_conf my_conf;
static struct my_conf *conf = &my_conf;

int parse_args(parse_command_t *cmds, int argc, char const *argv[]) {
    if (conf_init(cmds) < 0) {
        fprintf(stderr, "conf_init failed\n");
        return -1;
    }
    if (conf_parse_args(cmds, argc, argv) < 0) {
        fprintf(stderr, "conf_parse_args failed\n");
        return -1;
    }
    return 0;
}

void usage(parse_command_t *cmds, char const *name) {
    printf("Usage: %s [options]\n\n", name);
    conf_print_usage(stdout, cmds);
    printf("\n");
}

int fork_exec(int argc, const char *argv[], char *envp[]) {
    int pid = 0;
    (void)argc;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork() failed: %s)\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        execve(argv[0], (char *const *)argv, envp);
        return 0;
    }

    return pid;
}

long long get_total_cpu_usage() {
    long long user, nice, system, idle;
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        fprintf(stderr, "fopen(/proc/stat) failed: %s\n", strerror(errno));
        return -1;
    }
    fscanf(fp, "cpu %lld %lld %lld %lld", &user, &nice, &system, &idle);
    fclose(fp);
    return user + nice + system + idle;
}

struct time_history {
    long utime, stime, cutime, cstime;
    long long total_cpu_usage;
};

#define MAX_HISTORY_LEN 30
struct time_history time_history[MAX_HISTORY_LEN];
int cur_history_idx = 0;

#define MAX_PATH_LEN 256
int loop(struct my_conf *conf) {
    char stat_file[MAX_PATH_LEN];

    long nproc = get_nprocs();

    (void)nproc;

    long long p_total_cpu_usage = 0;
    long p_utime, p_stime, p_cutime, p_cstime;

    long utime, stime, cutime, cstime;
    long long starttime;

    int full = 0;

    int is_stop = 0;

    sprintf(stat_file, "/proc/%d/stat", conf->pid);
    for (;;) {
        long long total_cpu_usage = get_total_cpu_usage();

        FILE *fp = fopen(stat_file, "r");
        if (fp == NULL) {
            fprintf(stdout, "pid %d exited %s\n", conf->pid, strerror(errno));
            return -1;
        }
        fscanf(fp,
               "%*d %*s %*c %*d "  // pid,command,state,ppid
               "%*d %*d %*d %*d %*u %*u %*u %*u %*u "
               "%ld %ld %ld %ld "  // utime,stime,cutime,cstime
               "%*d %*d %*d %*d "
               "%lld",
               &utime, &stime, &cutime, &cstime, &starttime);
        fclose(fp);

        struct time_history *th = &time_history[cur_history_idx];
        th->utime = utime;
        th->stime = stime;
        th->cutime = cutime;
        th->cstime = cstime;
        th->total_cpu_usage = total_cpu_usage;

        cur_history_idx++;
        if (cur_history_idx >= MAX_HISTORY_LEN) {
            cur_history_idx = 0;
            full = 1;
        }
        if (!full) {
            usleep(1000L * conf->interval_ms);
            continue;
        }

        struct time_history *th_prev = &time_history[cur_history_idx];
        p_utime = th_prev->utime;
        p_stime = th_prev->stime;
        p_cutime = th_prev->cutime;
        p_cstime = th_prev->cstime;
        p_total_cpu_usage = th_prev->total_cpu_usage;

        long long proc_time_since = utime - p_utime + stime - p_stime + cutime -
                                    p_cutime + cstime - p_cstime;
        long long total_time_since = total_cpu_usage - p_total_cpu_usage;

        double cpu_usage =
            (proc_time_since * (double)100.0) / total_time_since * nproc;

        if (cpu_usage >= conf->percent && !is_stop) {
            kill(conf->pid, SIGSTOP);
            is_stop = 1;
#ifdef DEBUG
            printf("STP:1 %lf >= %d\n", cpu_usage, conf->percent);
#endif
        }
        if (cpu_usage < conf->percent && is_stop) {
            kill(conf->pid, SIGCONT);
            is_stop = 0;
#ifdef DEBUG
            printf("STP:0 %lf < %d\n", cpu_usage, conf->percent);
#endif
        }

        usleep(1000L * conf->interval_ms);
    }
}

int main(int argc, char const *argv[], char *envp[]) {
    int r_argc = 0;
    parse_command_t cmds[] = {
        CONF_CMD_INT(
            conf, pid, "0",
            "pid that you want to limit, 0/not-specify means cpu_limit_run "
            "spawn a new process and limit cpu usage of it, the program spawn "
            "and args follow by ' -- ' , for example: cpu_limit_run -- du -sh "
            "*"),
        CONF_CMD_INT(conf, percent, "50", "maximun percent of cpu usage"),
        // CONF_CMD_TIME(conf, interval_ms, "10", "interval of cpu usage
        // check"),
        CONF_CMD_END(),
    };

    if (argc < 2) {
        usage(cmds, argv[0]);
        return -1;
    }

    for (r_argc = 1; r_argc < argc; r_argc++) {
        if (strcmp(argv[r_argc], "--") == 0) {
            break;
        }
    }

    if (parse_args(cmds, r_argc, argv) < 0) {
        usage(cmds, argv[0]);
        return -1;
    }
    conf->interval_ms = 10;

    // conf_print_conf(stdout, cmds);
    // return -1;

    int pid = 0;
    r_argc++;
    if (r_argc < argc) {
        pid = fork_exec(argc - r_argc, argv + r_argc, envp);
        if (pid == 0) {
            return 0;
        } else if (pid < 0) {
            return pid;
        }
    } else if (r_argc >= argc) {
        pid = conf->pid;
    }

    if (pid == 0) {
        fprintf(stderr, "--pid is not specified\n");
        return -1;
    }

    conf->pid = pid;

    if (conf->interval_ms <= 0) {
        fprintf(stderr, "--interval_ms must larger then 0\n");
        return -1;
    }

    loop(conf);

    return 0;
}
