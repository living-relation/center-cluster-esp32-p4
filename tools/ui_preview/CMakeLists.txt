cmake_minimum_required(VERSION 3.16)
project(ui_preview C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "Path to lv_conf.h")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../managed_components/lvgl__lvgl" "${CMAKE_CURRENT_BINARY_DIR}/lvgl")

add_executable(ui_preview
    main_preview.c
    ../../main/tach_ui/ui.c
    ../../main/tach_ui/ui_helpers.c
    ../../main/tach_ui/screens/ui_Screen1.c
    ../../main/tach_ui/images/ui_img_1656279599.c
    ../../main/tach_ui/fonts/ui_font_DotoLarge.c
    ../../main/tach_ui/fonts/ui_font_Doto_48.c
    ../../main/tach_ui/fonts/school_bell_48.c
    ../../main/tach_ui/ui_theme.c
)

target_include_directories(ui_preview PRIVATE
    ../../main
    ../../main/tach_ui
    ../../main/tach_ui/screens
    ../../main/tach_ui/fonts
    ../../main/tach_ui/images
)

target_compile_definitions(ui_preview PRIVATE LV_CONF_INCLUDE_SIMPLE)
target_link_libraries(ui_preview PRIVATE lvgl m)
