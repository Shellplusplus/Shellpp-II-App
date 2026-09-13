#include <stdint.h>
#include "shellpp_firmware_abi.h"
#include "shellpp_native_ui.h"

typedef int32_t (*vela_open_t)(const char *, int32_t, ...);
typedef int32_t (*vela_write_t)(int32_t, const void *, uint32_t);
typedef int32_t (*vela_close_t)(int32_t);
typedef int32_t (*vela_ioctl_t)(int32_t, int32_t, unsigned long);

#define VELA_OPEN ((vela_open_t)SHELLPP_ABI_OPEN_ADDR)
#define VELA_WRITE ((vela_write_t)SHELLPP_ABI_WRITE_ADDR)
#define VELA_CLOSE ((vela_close_t)SHELLPP_ABI_CLOSE_ADDR)
#define VELA_IOCTL ((vela_ioctl_t)SHELLPP_ABI_IOCTL_ADDR)
#define VIBRATOR_PATH "/dev/lra0"
#define EVIOCSFF 0x3f01
#define EVIOCRMFF 0x3f02
#define FF_PERIODIC 0x0001
#define FF_SINE 0x0001
#define VIBRATOR_FD_INVALID (-1)

/* Xiaomi's BES Vela driver consumes the NuttX/Linux-compatible 0x2c-byte
 * ff_effect record. Keep the layout explicit so no host headers leak in. */
struct shellpp_ff_periodic {
    uint16_t waveform;
    uint16_t period;
    int16_t magnitude;
    int16_t offset;
    uint16_t phase;
    uint16_t attack_length;
    uint16_t attack_level;
    uint16_t fade_length;
    uint16_t fade_level;
    uint32_t custom_len;
    const void *custom_data;
};
struct shellpp_ff_effect {
    uint16_t type;
    int16_t id;
    uint16_t direction;
    uint16_t trigger_button;
    uint16_t trigger_interval;
    uint16_t replay_length;
    uint16_t replay_delay;
    union {
        struct shellpp_ff_periodic periodic;
        uint8_t raw[28];
    } u;
};

_Static_assert(sizeof(struct shellpp_ff_effect) == 44, "ff_effect size");

static int g_vibration_fd = VIBRATOR_FD_INVALID;
static int16_t g_vibration_effect_id = -1;

static void zero_bytes(void *ptr, uint32_t length) {
    uint8_t *bytes = (uint8_t *)ptr;
    while (length--) *bytes++ = 0;
}

int shellpp_vibration_stop(void) {
    int result = 0;
    if (g_vibration_fd >= 0 && g_vibration_effect_id >= 0) {
        result = VELA_IOCTL(g_vibration_fd, EVIOCRMFF,
            (unsigned long)(uint16_t)g_vibration_effect_id);
    }
    if (g_vibration_fd >= 0) {
        int close_result = VELA_CLOSE(g_vibration_fd);
        if (result == 0 && close_result < 0) result = close_result;
    }
    g_vibration_fd = VIBRATOR_FD_INVALID;
    g_vibration_effect_id = -1;
    return result < 0 ? -1 : 0;
}

int shellpp_vibration_pulse(uint8_t strong) {
    struct shellpp_ff_effect effect;
    int32_t fd;
    int32_t result;
    (void)shellpp_vibration_stop();
    fd = VELA_OPEN(VIBRATOR_PATH, SHELLPP_ABI_O_WRONLY);
    if (fd < 0) return -1;
    zero_bytes(&effect, sizeof(effect));
    effect.type = FF_PERIODIC;
    effect.id = -1;
    effect.u.periodic.waveform = FF_SINE;
    effect.u.periodic.period = strong ? 80u : 110u;
    effect.u.periodic.magnitude = strong ? 0x7fffu : 0x5000;
    effect.u.periodic.offset = 0;
    effect.u.periodic.phase = 0;
    effect.replay_length = strong ? 260u : 140u;
    effect.replay_delay = 0;
    result = VELA_IOCTL(fd, EVIOCSFF, (unsigned long)&effect);
    if (result < 0 || effect.id < 0) {
        (void)VELA_CLOSE(fd);
        return -2;
    }
    g_vibration_fd = fd;
    g_vibration_effect_id = effect.id;
    result = VELA_WRITE(fd, &effect.id, sizeof(effect.id));
    if (result < 0) {
        (void)shellpp_vibration_stop();
        return -3;
    }
    return 0;
}
