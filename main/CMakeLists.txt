idf_component_register(
    SRCS
        "main.c"
        "bsp.c"
        "canbus.c"
        "uart_bridge.c"
        "alarm_task.c"
        "inputs.c"
        "dash_data.c"
        "ui/ui.c"
        "ui/ui_rpm_arc.c"
        "ui/ui_shift_leds.c"
        "ui/ui_gear_box.c"
        "ui/ui_odometer.c"
        "ui/ui_boot.c"
        "ui/fonts/aerospace_29.c"
        "ui/fonts/aerospace_87.c"
        "ui/fonts/aerospace_188.c"
        "ui/fonts/racehead_14.c"
        "ui/fonts/racehead_22.c"
        "ui/fonts/racehead_24.c"
        "ui/assets/toyota_mark.c"
        "ui/assets/toyota_wordmark.c"

    INCLUDE_DIRS
        "."
        "ui"

    PRIV_REQUIRES
        driver
        esp_lcd
        esp_driver_gpio
        esp_driver_uart
        esp_driver_twai
        esp_psram
        nvs_flash
        spi_flash
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)
