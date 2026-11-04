# Which game to build: rpg (default) or pet (the v0.1 companion game).
set(WF_GAME "rpg" CACHE STRING "Game to build: rpg or pet")
set(G "${CMAKE_CURRENT_LIST_DIR}/../${WF_GAME}")
file(GLOB GAME_SRCS "${G}/core/*.c" "${G}/ui/*.c" "${G}/ui/art/*.c")
file(GLOB COMMON_SRCS "${CMAKE_CURRENT_LIST_DIR}/../common/*.c" "${CMAKE_CURRENT_LIST_DIR}/../common/ui/*.c"
     "${CMAKE_CURRENT_LIST_DIR}/../common/ui/fonts/*.c")
idf_component_register(
    SRCS "main.c" "board.c" ${GAME_SRCS} ${COMMON_SRCS}
    INCLUDE_DIRS "." "../common" "../common/ui" "${G}/core" "${G}/ui"
    REQUIRES lvgl esp_lcd_co5300 esp_lcd_touch_cst9217 esp_lcd_touch esp_driver_gpio esp_driver_spi
             esp_driver_i2c esp_driver_usb_serial_jtag nvs_flash esp_timer esp_wifi esp_event esp_netif
             bootloader_support esp_system)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-parameter)
