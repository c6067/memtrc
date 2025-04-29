/**
 * @Author: wizard jack
 * @Date: 2025-4-27 16:24:55
 * @Last modified: 2024-4-27 16:24:55
 * @Description: use fixed array not dynamic array, I think it's better. append real-time 
 *               chart drawing function, and optimize some codes.
 */


#include "include/memtrc.h"
#include "include/chart.h"


//strore the previous chart state for incremental update 
static char previous_chart[CHART_HEIGHT][CHART_WIDTH + 1];
static int previous_count = 0;


void init_history(history_data_t *hist) {
    if (!hist) {
        fprintf(stderr, "Error: history_data_t pointer is NULL\n");
        return;
    }
    memset(hist, 0, sizeof(history_data_t));
    hist->count = 0;
    hist->max_value = LONG_MIN;     //initialize to safe state
    hist->min_value = LONG_MAX;
    
    //initialize the previous chart state
    memset(previous_chart, ' ', sizeof(previous_chart));
    for (int i = 0; i < CHART_HEIGHT; i++) {
        previous_chart[i][CHART_WIDTH] = '\0';
    }
    previous_count = 0;
}


void update_history(history_data_t *hist, long value) {
    if (!hist) {
        fprintf(stderr, "Error: history_data_t pointer is NULL\n");
        return;
    }
    
    //strict range check
    if (hist->count < 0 || hist->count > MAX_HISTORY) {
        fprintf(stderr, "Error: history_data_t count is out of bounds: %d\n", hist->count);
        hist->count = 0; //reset safe state
    }    
    if (value < 0) {
        fprintf(stderr, "Warning: value is negative: %ld, setting to 0\n", value);
        value = 0; //protective handling
    }
    
    //move data when hist is full
    if (hist->count == MAX_HISTORY) {
        memmove(hist->data, hist->data + 1, (MAX_HISTORY - 1) * sizeof(long));
        hist->count--;
    }
    
    //add new data
    if (hist->count < MAX_HISTORY) {
        hist->data[hist->count++] = value;
    } else {
        fprintf(stderr, "Error: Cannot add more data, history is full\n");
        return;
    }
    
    //update max and min values
    if (hist->count > 0) {
        hist->max_value = hist->data[0];
        hist->min_value = hist->data[0];        
        for (int i = 1; i < hist->count; i++) {
            if (hist->data[i] > hist->max_value) hist->max_value = hist->data[i];
            if (hist->data[i] < hist->min_value) hist->min_value = hist->data[i];
        }
    }
}


void cleanup_history(history_data_t *hist) {
    if (!hist) {
        fprintf(stderr, "Error: history_data_t pointer is NULL\n");
        return;
    }
    hist->count = 0;
    hist->max_value = LONG_MIN;    //reinitialize to safe state 
    hist->min_value = LONG_MAX;    
    //cleanup previous chart state
    memset(previous_chart, ' ', sizeof(previous_chart));
    for (int i = 0; i < CHART_HEIGHT; i++) {
        previous_chart[i][CHART_WIDTH] = '\0';
    }
    previous_count = 0;
}


void prepare_chart(const history_data_t *hist, char chart[CHART_HEIGHT][CHART_WIDTH + 1]) {
    if (!hist) {
        fprintf(stderr, "Error: history_data_t pointer is NULL\n");
        return;
    }

    //initialize chart
    for (int i = 0; i < CHART_HEIGHT; i++) {
        memset(chart[i], ' ', CHART_WIDTH);
        chart[i][CHART_WIDTH] = '\0';
    }    
    //draw borders
    for (int j = 0; j < CHART_WIDTH; j++) {
        chart[CHART_HEIGHT - 1][j] = '-';
    }
    for (int i = 0; i < CHART_HEIGHT; i++) {
        chart[i][0] = '|';
    }    
    //draw data points
    if (hist->count > 0) {
        long range = hist->max_value - hist->min_value;
        if (range == 0) range = 1;
        for (int i = 0; i < hist->count && i < CHART_WIDTH - 1; i++) {
            /*
             * need exercise due diligence to ensure the range 
             * and bounds are valid,because we are using chart 
             * with fixed array
            */
            int y = CHART_HEIGHT - 2 - 
                   (int)((hist->data[i] - hist->min_value) * (CHART_HEIGHT - 3) / range);
            if (y >= 0 && y < CHART_HEIGHT - 1) {
                chart[y][i + 1] = '*';
            }
        }
    }
}


int validate_params(const history_data_t *hist, const char *title) {
    if (!hist) {
        fprintf(stderr, "Error: history_data_t pointer is NULL\n");
        return 0;
    }
    if (!title || !*title) {
        fprintf(stderr, "Error: title is NULL or empty\n");
        return 0;
    }
    if (strlen(title) > 80) {
        fprintf(stderr, "Error: title too long (max %d chars)\n", 80);
        return 0;
    }
    if (strpbrk(title, "\n\r\t")) {
        fprintf(stderr, "Error: title contains invalid characters\n");
        return 0;
    }
    return 1;
}


void draw_chart(const history_data_t *hist, const char *title) {
    if (!validate_params(hist, title)) {
        fprintf(stderr, "Error: verified parameters failed validation\n");
        return;
    }
    
    char chart[CHART_HEIGHT][CHART_WIDTH + 1];
    prepare_chart(hist, chart);    
    //output buffer handling
    char output[CHART_HEIGHT * (CHART_WIDTH + 3) + 100];
    size_t pos = snprintf(output, sizeof(output), "%s\n", title);
    for (int i = 0; i < CHART_HEIGHT && pos < sizeof(output) - (CHART_WIDTH + 3); i++) {
        pos += snprintf(output + pos, sizeof(output) - pos, "%s\n", chart[i]);
    }
    printf("%s", output);
}


void draw_chart_incremental(const history_data_t *hist, const char *title) {
    if (!validate_params(hist, title)) {
        fprintf(stderr, "Error: verified parameters failed validation\n");
        return;
    }

    //set static variables for incremental update
    static char previous_title[81] = {0};    
    static int initialized = 0;
    
    //ensure correct initialization on first call
    if (!initialized) {
        memset(previous_chart, ' ', sizeof(previous_chart) - CHART_HEIGHT);
        for (int i = 0; i < CHART_HEIGHT; i++) {
            previous_chart[i][CHART_WIDTH] = '\0';
        }
        initialized = 1;
    }
    
    char current_chart[CHART_HEIGHT][CHART_WIDTH + 1];
    prepare_chart(hist, current_chart);
    
    //incremental update
    printf("\033[s");  // Save cursor position    
    if (strcmp(previous_title, title) != 0) {
        printf("\033[1;1H\033[K%s\n", title);
        strncpy(previous_title, title, 80);
        previous_title[80] = '\0';  //ensure string is null-terminated
    }
    
    for (int i = 0; i < CHART_HEIGHT; i++) {
        for (int j = 0; j < CHART_WIDTH; j++) {
            if (current_chart[i][j] != previous_chart[i][j]) {
                printf("\033[%d;%dH%c", i + 2, j + 1, current_chart[i][j]);
            }
        }
    }
    
    printf("\033[u");
    fflush(stdout);
    
    memcpy(previous_chart, current_chart, sizeof(current_chart));
}

volatile int keep_running = 1;  //control the loop flag

void draw_realtime_chart(history_data_t *hist, const char *title, 
                         int interval_ms, long (*update_func)(void*), void *context) {
    //verify parameter validation
    if (!hist) {
        fprintf(stderr, "Error: history_data_t pointer is NULL\n");
        return;
    }
    if (!title) {
        fprintf(stderr, "Error: title is NULL\n");
        return;
    }
    if (!update_func) {
        fprintf(stderr, "Error: update_func is NULL\n");
        return;
    }
    if (interval_ms <= 0) {
        fprintf(stderr, "Error: invalid interval_ms: %d\n", interval_ms);
        return;
    }
    
    //initialize chart
    printf("\033[?25l"); //hide cursor
    printf("\033[2J");   //clear screen
    printf("\033[H");    //move to top-left corner
    
    //draw first full chart
    long new_value = update_func(context);
    if (new_value < 0) {
        fprintf(stderr, "Error: update_func returned negative value: %ld\n", new_value);
        new_value = 0; //protective handling
    }
    update_history(hist, new_value);
    draw_chart(hist, title);
    
    //set initial state    
    previous_count = hist->count;
    
    while (keep_running) {
        //get new data
        new_value = update_func(context);
        if (new_value < 0) {
            fprintf(stderr, "Error: update_func returned negative value: %ld\n", new_value);
            new_value = 0; 
        }
        
        update_history(hist, new_value);        
        draw_chart_incremental(hist, title);
        
        //wait for the next interval, use nanosleep() more accurately
        struct timespec ts;
        ts.tv_sec = interval_ms / 1000;
        ts.tv_nsec = (interval_ms % 1000) * 1000000;
        if (nanosleep(&ts, NULL) == -1 && errno != EINTR) {
            perror("nanosleep failed");
            break;
        }
    }
    
    //restore terminal state
    printf("\033[?25h"); //display cursor
    printf("\n\n");
}


void stop_realtime_chart() {
    keep_running = 0;
    //restore terminal state
    printf("\033[?25h"); //display cursor
    printf("\n\n");
}
