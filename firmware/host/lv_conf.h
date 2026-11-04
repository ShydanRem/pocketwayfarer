/* LVGL configuration for the host preview/test build. The firmware uses the
 * equivalent Kconfig values in sdkconfig.defaults. */
#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 16
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB
#define LV_DEF_REFR_PERIOD 15
#define LV_DPI_DEF 200
#define LV_USE_LOG 0
#define LV_USE_SYSMON 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#endif
