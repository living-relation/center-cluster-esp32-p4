#ifndef LV_CONF_PREVIEW_H
#define LV_CONF_PREVIEW_H

#include "../../main/lv_conf.h"

#undef LV_TICK_CUSTOM
#define LV_TICK_CUSTOM 1

#undef LV_TICK_CUSTOM_INCLUDE
#define LV_TICK_CUSTOM_INCLUDE <stdint.h>

#undef LV_TICK_CUSTOM_SYS_TIME_EXPR
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (preview_get_tick_ms())

#undef LV_USE_PERF_MONITOR
#define LV_USE_PERF_MONITOR 0

#undef LV_USE_MEM_MONITOR
#define LV_USE_MEM_MONITOR 0

#undef LV_USE_SNAPSHOT
#define LV_USE_SNAPSHOT 1

#ifdef __cplusplus
extern "C" {
#endif

uint32_t preview_get_tick_ms(void);

#ifdef __cplusplus
}
#endif

#endif
