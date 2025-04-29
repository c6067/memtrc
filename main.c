/*
 * @Author: wizard jack
 * @Date: 2025-04-27 22:01:55
 * @LastEditors: wizard jack
 * @LastEditTime: 2025-04-27 22:24:55
 * @FilePath: memtrc/main.c
 * @Description: Main entry point for MemTrace.
 */
#include "include/memtrc.h"
#include "include/chart.h"


int main(void) {
    config_t cfg;
    char cmd_line[CMD_BUF_SIZE];
    char *args[MAX_ARGS];
    int arg_count;
    cmd_type_t cmd;
    int quit = 0;
    int ret = EXIT_SUCCESS;

    //initialize config
    if (!init_config(&cfg)) {
        fprintf(stderr, "Failed to initialize configuration.\n");
        return EXIT_FAILURE;
    }
    g_cfg = &cfg;

    //set singal handling
    struct sigaction sa = {
        .sa_sigaction = sigint_handler,
        .sa_flags = SA_SIGINFO | SA_RESTART
    };
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);  //deal with SIGINT when blocking SIGTERM    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to set signal handler");
        cleanup_config(&cfg);
        return EXIT_FAILURE;
    }

    printf("\nWelcome to MemTrace - Linux Memory Tracer Tool\n");
    printf("input 'help' to get help, input 'quit' to quit program\n\n");

    while (!quit) {
        show_menu();
        
        //read command line
        if (fgets(cmd_line, CMD_BUF_SIZE, stdin) == NULL) {
            printf("\nDetected input end, program will exit\n");
            break;  // EOF or error
        }
        
        //parse command
        cmd = parse_command(cmd_line, args, &arg_count);
        
        //execute command
        int result = execute_command(&cfg, cmd, args, arg_count);
        if (result == 1) {
            quit = 1;  //set exit flag
        } else if (result < 0) {
            fprintf(stderr, "Command execution failed\n");
            //deal with error,but continue
        }
    }

    //clear resources
    cleanup_config(&cfg);
    g_cfg = NULL;  //avoid dangling pointer

    if (ret == EXIT_SUCCESS) {
        printf("\nThank you for using MemTrace, goodbye!\n");
    } else {
        fprintf(stderr, "\nProgram terminated with errors\n");
    }

    return ret;
}
