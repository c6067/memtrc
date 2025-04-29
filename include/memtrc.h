/**
 * @Author: wizard jack
 * @Date: 2025-4-27 16:24:55
 * @Last modified: 2024-4-27 16:24:55
 * @Description: Linux system C program to trace memory usage of a process 
 */
#ifndef MEMTRC_H
#define MEMTRC_H


/*feature test macros MUST be defined before any header files are included*/
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/signal.h>
#include <pthread.h>
#include <sys/types.h>


#define BUF_SIZE 256
#define MAX_HISTORY 100
#define CMD_BUF_SIZE 1024
#define MAX_ARGS 16
#define KB_UNIT 1024    //1KB = 1024 bytes
#define MAX_CMD_LENGTH 256


typedef enum {
    PROC_TYPE_USER,     
    PROC_TYPE_KERNEL,   
    PROC_TYPE_UNKNOWN   //process type
} proc_type_t;

typedef struct {
    long vmsize;    //virtual memory size
    long vmrss;     //physical memory size
    long vmdata;    //data segment size
    long vmstk;     //stack segment size
    proc_type_t proc_type; //process type
} mem_info_t;

typedef struct {
    char *log_file;     //log file path
    FILE *log_fp;       //log file pointer
    int interval;       //monitor interval
    int continuous;     //continue monitoring    
    int monitoring;     //monitoring flag
    pid_t target_pid;   //target pid
    pthread_mutex_t lock; //mutex lock for thread safety    
} config_t;

typedef enum {
    CMD_TRACE,    //trace process memory
    CMD_HELP,     //display help
    CMD_QUIT,     //quit
    CMD_UNKNOWN   //unknown command
} cmd_type_t;

extern config_t *g_cfg; //global config pointer

proc_type_t get_process_type(pid_t pid);
int read_user_proc_mem_info(pid_t pid, mem_info_t *info);
int read_kernel_proc_mem_info(pid_t pid, mem_info_t *info);

int read_mem_info(pid_t pid, mem_info_t *info);
void display_mem_info(const mem_info_t *info);
void write_log(FILE *fp, const mem_info_t *info);

int init_config(config_t *cfg);
void cleanup_config(config_t *cfg);
void sigint_handler(int sig, siginfo_t *si, void *context);
void *monitor_thread(void *arg);

void show_menu(void);
cmd_type_t parse_command(char *cmd_line, char *args[], int *arg_count);
int execute_command(config_t *cfg, cmd_type_t cmd, char *args[], int arg_count);


#endif
