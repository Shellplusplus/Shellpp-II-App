#ifndef SHELLPP_II_MODULE_H
#define SHELLPP_II_MODULE_H

#include <stdint.h>

struct shellpp_ii_mod_info {
    int (*uninitializer)(void *arg);
    void *arg;
    const void *exports;
    uint32_t nexports;
};

int module_initialize(struct shellpp_ii_mod_info *modinfo);
int shellpp_app_register(void);
int shellpp_app_publish(void);
int shellpp_app_remove(void);

#endif
