cmake_minimum_required(VERSION 3.16)
# All dependencies are pinned public Git sources (scripts/firmware-deps.py);
# the component registry is never consulted.
set(ENV{IDF_COMPONENT_MANAGER} 0)
set(EXTRA_COMPONENT_DIRS
    ".deps/lvgl"
    ".deps/esp-bsp/components/lcd_touch/esp_lcd_touch"
    ".deps/waveshare/display/touch/esp_lcd_touch_cst9217"
    ".deps/esp-iot/components/display/lcd/esp_lcd_co5300"
    ".deps/esp-iot/tools/cmake_utilities")
set(COMPONENTS main cmake_utilities)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/.deps/esp-iot/tools/cmake_utilities")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pocketwayfarer VERSION 0.1.0)
idf_component_get_property(cst_lib esp_lcd_touch_cst9217 COMPONENT_LIB)
idf_component_get_property(touch_lib esp_lcd_touch COMPONENT_LIB)
target_link_libraries(${cst_lib} PUBLIC ${touch_lib})
