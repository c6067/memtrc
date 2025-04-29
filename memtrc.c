/**
 * @Author: wizard jack
 * @Date: 2025-4-27 16:24:55
 * @Last modified: 2025-4-27 16:24:55
 * @Description: 
 * core functions of memtrc, including: analyzing process type, reading memory info, 
 * writing log, manipulating config, monitoring thread, etc.
 * @Note: process type judgement maybe not accurate.
 */

#include "include/memtrc.h"
#include "include/chart.h"

config_t *g_cfg = NULL;   //define global config 

proc_type_t get_process_type(pid_t pid) {
    char path[BUF_SIZE];
    char buffer[BUF_SIZE];
    int is_kernel = 0;
    FILE *fp = NULL;        //first initialize file pointer to NULL
    
    //check if it's PID equals 1 (init process)
    if (pid == 1) {
        return PROC_TYPE_USER;  
    }    
    //check /proc/<pid>/cmdline is whether empty
    //kernel thread's cmdline is usually empty
    snprintf(path, BUF_SIZE, "/proc/%d/cmdline", pid);
    if ((fp = fopen(path, "r"))) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
        fclose(fp);
        fp = NULL;
        
        if (bytes_read == 0) {
            is_kernel = 1;  //maybe a kernel thread
        }
    }    
    //check /proc/<pid>/stat's 3rd field (state)
    //if it contains special flags, also consider it a kernel thread
    if (!is_kernel) {
        snprintf(path, BUF_SIZE, "/proc/%d/stat", pid);
        if ((fp = fopen(path, "r"))) {
            if (fgets(buffer, sizeof(buffer), fp)) {
                char *state = strchr(buffer, ')');
                if (state && *(state + 1) == ' ' && *(state + 2)) {                    
                    if (strchr("Z", *(state + 2)) || strchr("T", *(state + 2))) {
                        is_kernel = 1;
                    }
                }
            }
            fclose(fp);
            fp = NULL;
        }
    }    
    //check /proc/<pid>/status's VmSize field
    //some kernel threads may not have VmSize field
    if (!is_kernel) {
        snprintf(path, BUF_SIZE, "/proc/%d/status", pid);        
        if ((fp = fopen(path, "r")) != NULL) {
            is_kernel = 1;  // Assume it's kernel thread unless we find VmSize
            
            while (fgets(buffer, sizeof(buffer), fp)) {
                if (strncmp(buffer, "VmSize:", 7) == 0) {
                    is_kernel = 0;  // Found VmSize, not a kernel thread
                    break;
                }
            }        
            fclose(fp);
        } else {
            //fopen failed, maybe the process is dead,or the process is a kernel thread
            is_kernel = 1;
        }
    }
    
    return is_kernel ? PROC_TYPE_KERNEL : PROC_TYPE_USER;
}


int read_user_proc_mem_info(pid_t pid, mem_info_t *info) {
    if (pid <= 0 || info == NULL) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }    
    if (KB_UNIT <= 0) {
        fprintf(stderr, "Invalid KB_UNIT value: %d\n", KB_UNIT);
        return -1;
    }

    char path[BUF_SIZE];
    char line[BUF_SIZE];
    FILE *fp = NULL;
    int ret = -1;

    if (snprintf(path, sizeof(path), "/proc/%d/status", pid) >= (int)sizeof(path)) {
        fprintf(stderr, "Path too long for pid %d\n", pid);
        return -1;
    }

    fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Can't open file %s: %s\n", path, strerror(errno));
        return -1;
    }

    memset(info, 0, sizeof(mem_info_t));
    //read memory fields from /proc/<pid>/status,deal with end of line and truncate    
    while (fgets(line, sizeof(line), fp)) {
        unsigned long value = 0;
        char *nl = strchr(line, '\n');
        
        if (!nl && !feof(fp)) {
            fprintf(stderr, "Warning: Line truncated in %s\n", path);
            int c;
            while ((c = fgetc(fp)) != EOF && c != '\n');
            continue;
        }
        //analyze mem fields
        if (strncmp(line, "VmSize:", 7) == 0 && 
            sscanf(line + 7, "%lu", &value) == 1) {
            if (value > ULONG_MAX / KB_UNIT) {
                fprintf(stderr, "VmSize value too large: %lu\n", value);
                goto cleanup;
            }
            info->vmsize = value * KB_UNIT;
        }
        else if (strncmp(line, "VmRSS:", 6) == 0 && 
                 sscanf(line + 6, "%lu", &value) == 1) {
            info->vmrss = value * KB_UNIT;
        }
        else if (strncmp(line, "VmData:", 7) == 0 && 
                 sscanf(line + 7, "%lu", &value) == 1) {
            info->vmdata = value * KB_UNIT;
        }
        else if (strncmp(line, "VmStk:", 6) == 0 && 
                 sscanf(line + 6, "%lu", &value) == 1) {
            info->vmstk = value * KB_UNIT;
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "Error reading file %s: %s\n", path, strerror(errno));
        goto cleanup;
    }

    ret = 0;

cleanup:
    if (fp && fclose(fp) != 0) {
        fprintf(stderr, "Failed to close %s: %s\n", path, strerror(errno));
        ret = -1;
    }
    return ret;
}


int read_kernel_proc_mem_info(pid_t pid, mem_info_t *info){
    char path[BUF_SIZE];
    char buffer[BUF_SIZE];
    FILE *fp;
    int ret = 0;
    
    //initialize memory info and construct path
    memset(info, 0, sizeof(mem_info_t));
    info->proc_type = PROC_TYPE_KERNEL; //set default process type    
    if (snprintf(path, BUF_SIZE, "/proc/%d/status", pid) >= BUF_SIZE) {
        fprintf(stderr, "Path too long for pid %d\n", pid);
        return -1;
    }
    //deal with open file failure
    fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    //read and analyze mem fields from /proc/<pid>/status
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "VmRSS:", 6) == 0) {
            if (sscanf(buffer + 6, "%ld", &info->vmrss) != 1) {
                fprintf(stderr, "Failed to parse VmRSS for pid %d\n", pid);
                ret = -1;
            }
        } else if (strncmp(buffer, "VmSize:", 7) == 0) {
            if (sscanf(buffer + 7, "%ld", &info->vmsize) != 1) {
                fprintf(stderr, "Failed to parse VmSize for pid %d\n", pid);
                ret = -1;
            }
        } else if (strncmp(buffer, "VmData:", 7) == 0) {
            if (sscanf(buffer + 7, "%ld", &info->vmdata) != 1) {
                fprintf(stderr, "Failed to parse VmData for pid %d\n", pid);
                ret = -1;
            }
        } else if (strncmp(buffer, "VmStk:", 6) == 0) {
            if (sscanf(buffer + 6, "%ld", &info->vmstk) != 1) {
                fprintf(stderr, "Failed to parse VmStk for pid %d\n", pid);
                ret = -1;
            }
        }
    }

    //deal with file error or close failure
    if (ferror(fp)) {
        fprintf(stderr, "Error reading %s: %s\n", path, strerror(errno));
        ret = -1;
    }
    if (fclose(fp) != 0) {
        fprintf(stderr, "Failed to close %s: %s\n", path, strerror(errno));
        ret = -1;
    }

    //mark unread values as special value (-1)
    if (info->vmsize == 0) info->vmsize = -1;
    if (info->vmrss == 0) info->vmrss = -1;
    if (info->vmdata == 0) info->vmdata = -1;
    if (info->vmstk == 0) info->vmstk = -1;
    
    return ret;
}


int read_mem_info(pid_t pid, mem_info_t *info) {
    
    memset(info, 0, sizeof(mem_info_t));
    
    //get process type
    proc_type_t proc_type = get_process_type(pid);
    info->proc_type = proc_type;
    
    //according to process type, use different read method
    if (proc_type == PROC_TYPE_KERNEL) {
        return read_kernel_proc_mem_info(pid, info);
    } else {
        return read_user_proc_mem_info(pid, info);
    }
}


void display_mem_info(const mem_info_t *info) {
    printf("==== process memory info ====\n");    
    
    printf("process type: %s\n", 
           info->proc_type == PROC_TYPE_KERNEL ? "kernel process" :
           info->proc_type == PROC_TYPE_USER ? "user process" :
           info->proc_type == PROC_TYPE_UNKNOWN ? "unknown process" : "unknown");
    printf("process ID: %d\n", getpid());
    
    //according to process type, display different info
    if (info->proc_type == PROC_TYPE_KERNEL) {
        printf("NOTE: kernel process memory info maybe incomplete.\n");
        
        if (info->vmrss > 0) {
            printf("physical memory (RSS): %ld KB\n", info->vmrss);
        } else {
            printf("physical memory (RSS): unavailable\n");
        }        
        if (info->vmsize > 0) {
            printf("virtual memory (VSZ): %ld KB\n", info->vmsize);
        } else {
            printf("virtual memory (VSZ): unavailable\n");
        }
        
        printf("kernel process comment share kernel space," 
            "the above values may not represent the actual exclusive memory.\n");
    } else {
        // user process display way
        printf("process type: %s\n", info->proc_type == 
                PROC_TYPE_USER ? "user process" : "unknown process");
        printf("virtual memory (VSZ): %ld KB\n", info->vmsize);
        printf("physical memory (RSS): %ld KB\n", info->vmrss);
        printf("data segment (Data): %ld KB\n", info->vmdata);
        printf("stack space (Stack): %ld KB\n", info->vmstk);
    }
    
    printf("=====================\n");
}


void write_log(FILE *fp, const mem_info_t *info) {
    if (!fp || !info) {
        fprintf(stderr, "Error: Invalid arguments to write_log()\n");
        return;
    }

    //set time format and time string
    time_t now;
    struct tm *tm_info;
    char time_str[30];
    //get current time and format it
    time(&now);
    tm_info = localtime(&now);
    //check time info and format validity
    if (!tm_info) {
        fprintf(stderr, "Error getting local time\n");
        return;
    }
    if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        fprintf(stderr, "Error formatting time string\n");
        return;
    }
    
    // Thread-safe logging
    if (info->proc_type == PROC_TYPE_KERNEL) {
        fprintf(fp, "[%s] type: kernel process, RSS: %ld KB, VSZ: %ld KB\n", 
                time_str,
                info->vmrss > 0 ? info->vmrss : -1,
                info->vmsize > 0 ? info->vmsize : -1);
    } else {
        fprintf(fp, "[%s] type: user process, VSZ: %ld KB, RSS: %ld KB, Data: %ld KB, Stack: %ld KB\n",
                time_str,
                info->vmsize,
                info->vmrss,
                info->vmdata,
                info->vmstk);
    }
    
    //check if log file is valid
    if (fflush(fp) != 0) {
        fprintf(stderr, "Error flushing log file: %s\n", strerror(errno));
    }
}


int init_config(config_t *cfg) {
    if (!cfg){ 
        fprintf(stderr, "Error: config_t pointer is NULL\n");    
        return 0;
    }

    cfg->log_file = NULL;
    cfg->log_fp = NULL;
    cfg->interval = 1;
    cfg->continuous = 0;    
    cfg->monitoring = 0;  
    cfg->target_pid = 0;
    
    //NOTE:mutex lock initialization is here
    if (pthread_mutex_init(&cfg->lock, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize mutex\n");
        return 0;
    }
    
    return 1;
}


void cleanup_config(config_t *cfg) {
    if (!cfg) {
        fprintf(stderr, "Error: config_t pointer is NULL\n");
        return;
    }
    //first ensure the monitoring flag is set to 0
    __atomic_store_n(&cfg->monitoring, 0, __ATOMIC_SEQ_CST);
    
    int lock_ret;
    do {
        lock_ret = pthread_mutex_trylock(&cfg->lock);
        if (lock_ret == EBUSY) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000; // 1ms = 1,000,000ns
            nanosleep(&ts, NULL);
        } else if (lock_ret != 0) {
            fprintf(stderr, "Error: Failed to acquire mutex: %s\n", 
                    strerror(lock_ret));
            return; //or handle error appropriately
        }
    } while (lock_ret == EBUSY);
    
    //clean resources while holding the lock
    if (cfg->log_fp && cfg->log_fp != stdout && cfg->log_fp != stderr) {
        fclose(cfg->log_fp);
    }
    free(cfg->log_file); // free is safe at this time 
    cfg->log_fp = NULL;
    cfg->log_file = NULL;
    
    pthread_mutex_unlock(&cfg->lock);
    
    //reset other config values
    cfg->target_pid = 0;
    cfg->interval = 1;
    cfg->continuous = 0;    
    
    int destroy_ret = pthread_mutex_destroy(&cfg->lock);
    if (destroy_ret != 0) {
        fprintf(stderr, "Warning: Failed to destroy mutex: %s\n", 
                strerror(destroy_ret));
    }
}


void *monitor_thread(void *arg) {
    config_t *cfg = (config_t *)arg;
    if (!cfg) return NULL;
    
    mem_info_t info;
    history_data_t vmrss_hist, vmsize_hist;
    int retry_count = 0;
    const int max_retries = 3;  //max retry count
    
    init_history(&vmrss_hist);
    init_history(&vmsize_hist);
    
    while(1) {
        pthread_mutex_lock(&cfg->lock);
        int monitoring = cfg->monitoring;
        pthread_mutex_unlock(&cfg->lock);
        
        if (!monitoring) break;
        //check the process is whether still alive
        if (kill(cfg->target_pid, 0) == -1) {
            printf("Process %d terminated\n", cfg->target_pid);
            break;
        }
        
        if (read_mem_info(cfg->target_pid, &info) == 0) {
            retry_count = 0;
            
            update_history(&vmrss_hist, info.vmrss);
            update_history(&vmsize_hist, info.vmsize);            
            display_mem_info(&info);
            
            if (vmrss_hist.count > MAX_HISTORY) {
                cleanup_history(&vmrss_hist);
                init_history(&vmrss_hist);
            }
            if (vmsize_hist.count > MAX_HISTORY) {
                cleanup_history(&vmsize_hist);
                init_history(&vmsize_hist);
            }
            
            draw_chart(&vmrss_hist, "RSS History");
            draw_chart(&vmsize_hist, "VSZ History");
            
            pthread_mutex_lock(&cfg->lock);
            if (cfg->log_fp) {
                write_log(cfg->log_fp, &info);
            }
            pthread_mutex_unlock(&cfg->lock);
            
        } else {
            printf("Failed to read memory info of process %d (attempt %d/%d)\n", 
                  cfg->target_pid, ++retry_count, max_retries);
            if (retry_count >= max_retries) {
                printf("Reached max retry count, stopping monitoring\n");
                break;
            }
        }
        
        sleep(cfg->interval);   //sleep() enough     
    }

    cleanup_history(&vmrss_hist);
    cleanup_history(&vmsize_hist);
    
    printf("Monitor thread exited\n");
    return NULL;
}


void show_menu(void) {
    printf("\n");
    printf("\033[1;31m   __  __  _____  __  __  _____  _____   _____  \033[0m\n");
    printf("\033[1;37m  |  \\/  || ____||  \\/  ||_   _||  __ \\ / ____| \033[0m\n");
    printf("\033[1;34m  | \\  / || |__  | \\  / |  | |  | |__) | |      \033[0m\n");
    printf("\033[1;31m  | |\\/| ||  __| | |\\/| |  | |  |  _  /| |      \033[0m\n");
    printf("\033[1;37m  | |  | || |___ | |  | |  | |  | | \\ \\| |____  \033[0m\n");
    printf("\033[1;34m  |_|  |_||_____||_|  |_|  |_|  |_|  \\_\\\\_____| \033[0m\n");
    printf("\n");
    printf("1. trace <pid> - view the memory usage of a process\n");
    printf("   options:\n");
    printf("     -i interval - set the monitoring interval (seconds)\n");
    printf("     -c - enable continuous monitoring mode\n");
    printf("     -l logfile - specify the log file\n");
    printf("2. help - display this help message\n");
    printf("3. quit - exit the program\n");
    printf("=====================\n");
}


cmd_type_t parse_command(char *cmd_line, char *args[], int *arg_count) {
    if (!cmd_line || !args || !arg_count) {
        fprintf(stderr, "Error: Invalid arguments to parse_command()\n");
        return CMD_UNKNOWN; 
    }
    
    //copy command line to prevent original input from being modified
    char cmd_copy[MAX_CMD_LENGTH];
    strncpy(cmd_copy, cmd_line, MAX_CMD_LENGTH - 1);
    cmd_copy[MAX_CMD_LENGTH - 1] = '\0';
    
    //jump to the first non-space character
    char *cmd_ptr = cmd_copy;
    while (*cmd_ptr && isspace(*cmd_ptr)) cmd_ptr++;
    
    //remove trailing newline and spaces
    size_t len = strlen(cmd_ptr);
    while (len > 0 && (cmd_ptr[len - 1] == '\n' || isspace(cmd_ptr[len - 1]))) {
        cmd_ptr[--len] = '\0';
    }
    
    if (len == 0) {
        *arg_count = 0;
        return CMD_UNKNOWN;
    }
    
    //parse command line into arguments
    char *token = strtok(cmd_ptr, " \t");
    int count = 0;    
    while (token && count < MAX_ARGS) {
        args[count++] = token;
        token = strtok(NULL, " \t");
    }
    
    *arg_count = count;
    
    //check command type
    if (count > 0) {
        if (strcmp(args[0], "trace") == 0) {
            return CMD_TRACE;
        } else if (strcmp(args[0], "help") == 0) {
            return CMD_HELP;
        } else if (strcmp(args[0], "quit") == 0) {
            return CMD_QUIT;
        }
    }
    
    return CMD_UNKNOWN;
}


//deal with SIGINT signal
void sigint_handler(int sig, siginfo_t *si, void *ucontext) {
    (void)sig;  //avoid unused parameter warning
    (void)si;
    (void)ucontext;
    const char msg[] = "\nReceived SIGINT signal, stopping monitoring...\n";
    
    if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) == -1) {
        //error handling is not possible in the signal handler, just ignore
        return;
    }
    
    if (g_cfg) {
        __atomic_store_n(&g_cfg->monitoring, 0, __ATOMIC_SEQ_CST);
    }   
}


int execute_command(config_t *cfg, cmd_type_t cmd, char *args[], int arg_count) {
    switch (cmd) {
        case CMD_TRACE: {
            if (arg_count < 2) {
                printf("error: missing PID argument\n");
                return 0;
            }
            
            pid_t pid = atoi(args[1]);
            if (pid <= 0) {
                printf("error: invalid PID\n");
                return 0;
            }
            
            //cleanup previous config
            if (cfg->log_fp) {
                fclose(cfg->log_fp);
                cfg->log_fp = NULL;
            }
            if (cfg->log_file) {
                free(cfg->log_file);
                cfg->log_file = NULL;
            }
            //default config
            cfg->interval = 1;
            cfg->continuous = 0;
            cfg->target_pid = pid;
            
            //parse optional arguments
            for (int i = 2; i < arg_count; i++) {
                if (strcmp(args[i], "-i") == 0 && i + 1 < arg_count) {
                    int interval = atoi(args[i + 1]);
                    if (interval <= 0) {
                        printf("error: invalid interval\n");
                        return 0;
                    }
                    cfg->interval = interval;
                    i++;
                } else if (strcmp(args[i], "-c") == 0) {
                    cfg->continuous = 1;
                } else if (strcmp(args[i], "-l") == 0 && i + 1 < arg_count) {
                    cfg->log_file = strdup(args[i + 1]);
                    if (!cfg->log_file) {
                        printf("error: memory allocation failed\n");
                        return 0;
                    }
                    cfg->log_fp = fopen(cfg->log_file, "a");
                    if (!cfg->log_fp) {
                        printf("error:can't open log file%s\n", cfg->log_file);
                        free(cfg->log_file);
                        cfg->log_file = NULL;
                        return 0;
                    }
                    i++;
                }
            }
            
            //non-continuous mode, read once
            if (!cfg->continuous) {
                mem_info_t info;
                if (read_mem_info(pid, &info) == 0) {
                    display_mem_info(&info);
                    if (cfg->log_fp) {
                        write_log(cfg->log_fp, &info);
                    }
                } else {
                    printf("error: can't read memory info of process %d\n", pid);
                }
                return 0;
            }
            
            //continuous mode
            printf("start monitoring the process%d (each %d updated once second)\n", pid, cfg->interval);
            printf("press Ctrl+C or enter to stop monitoring...\n");
            
            /*
            1.unified initialization of sigaction structure
            2.verify sigint_handler validity
            3.initialize signal mask
            4.set signal handler
            */
            struct sigaction sa = {
                .sa_sigaction = sigint_handler,   //function pointer
                .sa_flags = SA_SIGINFO | SA_RESTART
            };            
            sigemptyset(&sa.sa_mask);            
            if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("Failed to set SIGINT handler");
                exit(EXIT_FAILURE);
            }
            
            //create monitoring thread
            pthread_t tid;
            pthread_mutex_lock(&cfg->lock);
            cfg->monitoring = 1;
            pthread_mutex_unlock(&cfg->lock);
            
            //check return value
            int ret = pthread_create(&tid, NULL, monitor_thread, cfg);
            if (ret != 0) {
                printf("error: failed to create monitoring thread: %s\n", strerror(ret));
                pthread_mutex_lock(&cfg->lock);
                cfg->monitoring = 0;
                pthread_mutex_unlock(&cfg->lock);
                return 0;
            }
            
            //wait for user input to stop
            getchar();
            
            //stop monitoring
            pthread_mutex_lock(&cfg->lock);
            cfg->monitoring = 0;
            pthread_mutex_unlock(&cfg->lock);
            
            //wait for monitoring thread to finish
            pthread_join(tid, NULL);            
            printf("monitoring stopped\n");
            return 0;
        }
        
        case CMD_HELP:
            printf("\nUsage:\n");
            printf("1. trace <pid> - view the memory usage of a process\n");
            printf("   options:\n");
            printf("     -i interval - set the monitoring interval (seconds)\n");
            printf("     -c - enable continuous monitoring mode\n");
            printf("     -l logfile - specify the log file\n");
            printf("     example:\n");
            printf("     trace 1234 -c -i 2 -l memory.log\n");
            printf("2. help - display this help information\n");
            printf("3. quit - exit the program\n");            
            return 0;
            
            case CMD_QUIT:
            //ensure in exit,clean up resources
            if (cfg->log_fp) {
                fclose(cfg->log_fp);
                cfg->log_fp = NULL;
            }
            if (cfg->log_file) {
                free(cfg->log_file);
                cfg->log_file = NULL;
            }
            return 1;  //exit
            
        case CMD_UNKNOWN:
        default:
            printf("unrecognized command. type 'help' to get help.\n");
            return 0;
    }
}

