/**
 * @Author: wizard jack
 * @Date: 2025-4-27 16:24:55
 * @Last modified: 2025-4-27 16:24:55
 * @Description:test units, including test cases for memtrc, chart, and config
 */


#include "include/memtrc.h"
#include "include/chart.h"
#include <assert.h>


//global variables needed for tests
extern config_t *g_cfg;
static config_t *saved_g_cfg = NULL;


//setup and teardown functions for tests
void setup(void) {
    //save the original global config
    saved_g_cfg = g_cfg;
}

void teardown(void) {
    //restore the original global config
    g_cfg = saved_g_cfg;
}


void cleanup_tests(void) {
    //free any resources allocated during tests
    if (g_cfg && g_cfg != saved_g_cfg) {
        cleanup_config(g_cfg);
        free(g_cfg);
        g_cfg = saved_g_cfg;
    }
}


void test_update_history(void) {
    printf("Testing history update functionality...\n");
    history_data_t hist;
    init_history(&hist);
    
    //test initial state
    assert(hist.count == 0);
    assert(hist.max_value == LONG_MIN);
    assert(hist.min_value == LONG_MAX);
    
    //test adding a single data point
    update_history(&hist, 10);
    assert(hist.count == 1);
    assert(hist.data[0] == 10);
    assert(hist.max_value == 10);
    assert(hist.min_value == 10);
    
    //test adding multiple data points
    update_history(&hist, 20);
    assert(hist.count == 2);
    assert(hist.max_value == 20);
    assert(hist.min_value == 10);
    
    //test handling negative values (should be treated as 0)
    update_history(&hist, -5);
    assert(hist.count == 3);
    assert(hist.data[2] == 0); // -5 should be converted to 0
    
    assert(hist.min_value == 0);

    //test history capacity limit
    for (int i = 0; i < MAX_HISTORY; i++) {
        update_history(&hist, i);
    }
    //history should not exceed MAX_HISTORY
    assert(hist.count <= MAX_HISTORY);
    
    cleanup_history(&hist);
    assert(hist.count == 0);
    assert(hist.max_value == LONG_MIN);
    assert(hist.min_value == LONG_MAX);
    
    printf("test_update_history passed!\n");
}


void test_memtrc(void) {
    printf("Testing memory info reading functionality...\n");
    mem_info_t info;
    pid_t pid = getpid();
    
    //test reading current process mem info
    assert(read_mem_info(pid, &info) == 0);
    assert(info.vmsize > 0);
    assert(info.vmrss > 0);
    assert(info.vmdata > 0);
    assert(info.vmstk > 0);
    assert(info.proc_type == PROC_TYPE_USER);
    
    //test process type detection
    proc_type_t type = get_process_type(pid);
    assert(type == PROC_TYPE_USER);
    
    //test displaying mem info (just make sure it doesn't crash)
    display_mem_info(&info);
    
    printf("test_memtrc passed!\n");
}


void test_draw_chart(void) {
    printf("Testing chart drawing functionality...\n");
    history_data_t hist;
    init_history(&hist);
    
    //add test data with a more realistic pattern
    //create a sine wave pattern for better visual testing
    for(int i = 0; i < 20; i++) {
        double angle = i * (3.14159 / 10);
        long value = (long)(500 + 400 * sin(angle));
        update_history(&hist, value);
    }
    
    //test validate_params function
    assert(validate_params(&hist, "Test Chart") == 1);
    assert(validate_params(NULL, "Test Chart") == 0);
    assert(validate_params(&hist, NULL) == 0);
    assert(validate_params(&hist, "") == 0);
    
    //test draw_chart & draw_chart_incremental function
    printf("Chart display test:\n");
    draw_chart(&hist, "Test Chart");
    printf("Incremental Chart display test:\n");
    draw_chart_incremental(&hist, "Test Chart");
    
    cleanup_history(&hist);
    printf("test_draw_chart passed!\n");
}


//dummy update function for real-time chart test, return incrementing values
static long dummy_update_func(void *context) {
    static long counter = 0;
    (void)context;
    return counter += 50; //increase by 50 at each call
}


void test_real_time_chart(void) {
    printf("Testing real-time chart functionality...\n");

    history_data_t hist;
    init_history(&hist);
    
    //run the real-time chart for a short period (simulate 3 iterations)
    volatile int real_time_iterations = 3;
    int keep_running = 1;
    
    /*
     *run the real-time chart update in a separate thread
     *for test, simulate part of the call in the main thread
     *last use a nanosleep() to simulate the delay
    */
    for (int i = 0; i < real_time_iterations && keep_running; i++) {
        long new_value = dummy_update_func(NULL);
        update_history(&hist, new_value);
        draw_chart_incremental(&hist, "Real-Time Test Chart");
        {
            struct timespec ts;
            ts.tv_sec = 200 / 1000;             // part of 200ms in seconds
            ts.tv_nsec = (200 % 1000) * 1000000;  // 200ms in nanoseconds
            nanosleep(&ts, NULL);
        }
    }
    
    //only test the stop_realtime_chart function 
    //as start_realtime_chart would run in a separate thread
    stop_realtime_chart();
    
    printf("\nReal-time chart test done.\n");    
    cleanup_history(&hist);
    printf("test_real_time_chart passed!\n");
}


void test_config_init(void) {
    printf("Testing configuration initialization...\n");
    
    //dynamically allocate memory
    config_t *cfg = malloc(sizeof(config_t));
    assert(cfg != NULL);
    
    assert(init_config(cfg) != 0);
    
    //verify default configuration values
    assert(cfg->log_file == NULL);
    assert(cfg->log_fp == NULL);
    assert(cfg->interval == 1);
    assert(cfg->continuous == 0);    
    assert(cfg->monitoring == 0);
    assert(cfg->target_pid == 0);
    
    //test cleanup_config
    cleanup_config(cfg);
    
    //test with NULL pointer (shouldn't crash)
    init_config(NULL);
    cleanup_config(NULL);
    
    free(cfg);
    printf("test_config_init passed!\n");
}


void test_parse_command(void) {
    printf("Testing command parsing functionality...\n");
    char *args[MAX_ARGS];
    int arg_count;
    char cmd_line[128];
    
    //test basic command
    strcpy(cmd_line, "trace 1234");
    assert(parse_command(cmd_line, args, &arg_count) == CMD_TRACE);
    assert(arg_count == 2);
    assert(strcmp(args[0], "trace") == 0);
    assert(strcmp(args[1], "1234") == 0);
    
    //text command with options
    strcpy(cmd_line, "trace 5678 -i 2 -l log.txt");
    assert(parse_command(cmd_line, args, &arg_count) == CMD_TRACE);
    assert(arg_count == 6);
    
    //test help command
    strcpy(cmd_line, "help");
    assert(parse_command(cmd_line, args, &arg_count) == CMD_HELP);
    assert(arg_count == 1);
    
    //test quit command
    strcpy(cmd_line, "quit");
    assert(parse_command(cmd_line, args, &arg_count) == CMD_QUIT);
    assert(arg_count == 1);
    
    //test empty command
    strcpy(cmd_line, "");
    assert(parse_command(cmd_line, args, &arg_count) == CMD_UNKNOWN);
    assert(arg_count == 0);
    
    //test invalid command
    strcpy(cmd_line, "invalid_command");
    assert(parse_command(cmd_line, args, &arg_count) == CMD_UNKNOWN);
    
    //test NULL parameters
    assert(parse_command(NULL, args, &arg_count) == CMD_UNKNOWN);
    assert(parse_command(cmd_line, NULL, &arg_count) == CMD_UNKNOWN);
    assert(parse_command(cmd_line, args, NULL) == CMD_UNKNOWN);
    
    printf("test_parse_command passed!\n");
}


void test_write_log(void) {
    printf("Testing log writing functionality...\n");    
    //setup test data
    mem_info_t info = {
        .proc_type = PROC_TYPE_USER,
        .vmsize = 1024,
        .vmrss = 512,
        .vmdata = 256,
        .vmstk = 128
    };
    
    //create temporary file
    FILE *tmp = tmpfile();
    assert(tmp != NULL);
    
    //test write_log
    write_log(tmp, &info);
    rewind(tmp);
    
    //verify log content
    char buf[256];
    assert(fgets(buf, sizeof(buf), tmp) != NULL);
    assert(strstr(buf, "type: user process") != NULL);
    assert(strstr(buf, "VSZ: 1024 KB") != NULL);
    assert(strstr(buf, "RSS: 512 KB") != NULL);
    
    //test with kernel process type
    info.proc_type = PROC_TYPE_KERNEL;
    rewind(tmp);
    write_log(tmp, &info);
    rewind(tmp);
    assert(fgets(buf, sizeof(buf), tmp) != NULL);
    assert(strstr(buf, "type: kernel process") != NULL);
    
    //test with NULL parameters (shouldn't crash)
    write_log(NULL, &info);
    write_log(tmp, NULL);
    
    fclose(tmp);
    printf("test_write_log passed!\n");
}


void test_signal_handler(void) {
    printf("Testing signal handler functionality...\n");
    
    //setup test config
    config_t *cfg = malloc(sizeof(config_t));
    assert(cfg != NULL);
    init_config(cfg);
    
    //save original g_cfg and set test one
    config_t *old_cfg = g_cfg;
    g_cfg = cfg;
    
    //test signal handler (simulate SIGINT)
    cfg->monitoring = 1;
    sigint_handler(SIGINT, NULL, NULL);
    assert(cfg->monitoring == 0);
    
    //test with NULL g_cfg (shouldn't crash)
    g_cfg = NULL;
    sigint_handler(SIGINT, NULL, NULL);
    
    //restore original g_cfg
    g_cfg = old_cfg;
    
    cleanup_config(cfg);
    free(cfg);
    
    printf("test_signal_handler passed!\n");
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n====== Running MemTrace Tests ======\n\n");
    
    setup();
    
    //run all tests
    test_update_history();
    test_memtrc();
    test_draw_chart();
    test_real_time_chart();
    test_config_init();
    test_parse_command();
    test_write_log();
    test_signal_handler();
    
    teardown();
    cleanup_tests();
    
    printf("\n====== All tests passed! ======\n");
    return 0;
}
