#ifndef SHELLPP_NATIVE_UI_H
#define SHELLPP_NATIVE_UI_H

#include <stdint.h>

void shellpp_ui_reset(void);
int shellpp_ui_page_create(uint32_t page_index, void *descriptor, void *root);
int shellpp_ui_page_resume(uint32_t page_index, void *descriptor);
int shellpp_ui_page_pause(uint32_t page_index);
int shellpp_ui_page_destroy(uint32_t page_index);

#endif
