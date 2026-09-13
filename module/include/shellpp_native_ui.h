#ifndef SHELLPP_NATIVE_UI_H
#define SHELLPP_NATIVE_UI_H

#include <stdint.h>

void shellpp_ui_reset(void);
struct shellpp_cpu_bench_result {
    uint32_t overall_score;
    uint32_t integer_score;
    uint32_t float_score;
    uint32_t memory_score;
    uint32_t mixed_score;
    uint32_t elapsed_ms;
    uint32_t checksum;
};

int shellpp_cpu_bench_run(struct shellpp_cpu_bench_result *result);
int shellpp_vibration_pulse(uint8_t strong);
int shellpp_vibration_stop(void);
int shellpp_ui_page_create(uint32_t page_index, void *descriptor, void *root);
int shellpp_ui_page_resume(uint32_t page_index, void *descriptor);
int shellpp_ui_page_pause(uint32_t page_index);
int shellpp_ui_page_destroy(uint32_t page_index);

#endif
