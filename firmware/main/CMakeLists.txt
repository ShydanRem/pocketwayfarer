file(GLOB CORE_SRCS "${CMAKE_CURRENT_LIST_DIR}/../core/*.c")
file(GLOB UI_SRCS "${CMAKE_CURRENT_LIST_DIR}/../ui/*.c" "${CMAKE_CURRENT_LIST_DIR}/../ui/art/*.c")
idf_component_register(
    SRCS "main.c" "board.c" ${CORE_SRCS} ${UI_SRCS}
    INCLUDE_DIRS "." "../core" "../ui"
    REQUIRES lvgl esp_lcd_co5300 esp_lcd_touch_cst9217 esp_lcd_touch esp_driver_gpio esp_driver_spi
             esp_driver_i2c esp_driver_usb_serial_jtag nvs_flash esp_timer esp_wifi esp_event esp_netif
             bootloader_support esp_system)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-parameter)
