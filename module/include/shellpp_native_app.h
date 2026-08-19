#ifndef SHELLPP_NATIVE_APP_H
#define SHELLPP_NATIVE_APP_H

#include <stdint.h>

/* Runs the native-App half of the installer protocol. Stage 1 creates the
 * PackageManager/ActivityManager records, and stage 2 publishes Launcher. */
int shellpp_native_install_stage(uint32_t stage);

/* Submit the one-time Supervisor-loaded notification after the control driver
 * has been registered and Lua has staged the launcher icon. */
int shellpp_native_notify_loaded(void);

struct shellpp_native_status {
    uint32_t app_id;
    uint32_t registered;
    uint32_t published;
    uint32_t loaded_notified;
    int install_result;
    int launcher_result;
    int notification_result;
};

/* Copies diagnostic state into caller-owned storage without exposing the
 * private firmware descriptors. */
void shellpp_native_get_status(struct shellpp_native_status *status);

/* Firmware registries retain callback pointers into the module after stage 1.
 * The loader must not unload it while those pointers remain resident. */
int shellpp_native_can_unload(void);

/* There is no confirmed inverse ABI yet, so this rejects removal rather than
 * leaving dangling ActivityManager and Launcher callbacks. */
int shellpp_native_uninstall(void);

#endif
