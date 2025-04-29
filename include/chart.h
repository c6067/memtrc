/**
 * @Author:wizard jack
 * @Date: 2025-4-27 16:24:55
 * @Last modified: 2025-4-27 16:24:55
 * @Description:chart header file,including macros, chart data structure, 
 *              and function declarations
 */

#ifndef CHART_H
#define CHART_H
#include "memtrc.h"

#define MAX_HISTORY 100  // Maximum number of data points to display 
#define CHART_HEIGHT 20
#define CHART_WIDTH  60

typedef struct {
    long data[MAX_HISTORY]; 
    int count;
    long max_value;
    long min_value;
} history_data_t;


void init_history(history_data_t *hist);
void update_history(history_data_t *hist, long value);
void cleanup_history(history_data_t *hist);

void prepare_chart(const history_data_t *hist, 
    char chart[CHART_HEIGHT][CHART_WIDTH + 1]);
int validate_params(const history_data_t *hist, const char *title);

void draw_chart(const history_data_t *hist, const char *title);
void draw_chart_incremental(const history_data_t *hist, const char *title);
void draw_realtime_chart(history_data_t *hist, const char *title,
        int interval_ms, long (*update_func)(void*), void *context);
void stop_realtime_chart();

#endif
