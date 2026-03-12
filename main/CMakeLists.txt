# Collect all protocol JSON files
file(GLOB CAN_PROTOCOL_JSON
    "${CMAKE_CURRENT_SOURCE_DIR}/canbus/protocols/*.json"
)

# Generate protocol list file
set(PROTOCOL_LIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/protocol_list.c)

file(WRITE ${PROTOCOL_LIST_FILE} "#include \"protocol_loader.h\"\n\n")

foreach(json_file ${CAN_PROTOCOL_JSON})

    get_filename_component(json_name ${json_file} NAME)
    string(REPLACE "." "_" json_symbol ${json_name})
    string(REPLACE "-" "_" json_symbol ${json_symbol})

    file(APPEND ${PROTOCOL_LIST_FILE}
        "extern const char _binary_canbus_protocols_${json_symbol}_start[];\n"
    )

endforeach()

file(APPEND ${PROTOCOL_LIST_FILE} "\nconst char* protocol_json_list[] = {\n")

foreach(json_file ${CAN_PROTOCOL_JSON})

    get_filename_component(json_name ${json_file} NAME)
    string(REPLACE "." "_" json_symbol ${json_name})
    string(REPLACE "-" "_" json_symbol ${json_symbol})

    file(APPEND ${PROTOCOL_LIST_FILE}
        "    _binary_canbus_protocols_${json_symbol}_start,\n"
    )

endforeach()

file(APPEND ${PROTOCOL_LIST_FILE} "};\n")

file(APPEND ${PROTOCOL_LIST_FILE}
    "const int protocol_json_count = sizeof(protocol_json_list)/sizeof(protocol_json_list[0]);\n"
)

idf_component_register(
    SRCS 
        "main.c"
        "tach_ui/ui.c"
        "tach_ui/screens/ui_Screen1.c"
        "tach_ui/images/ui_img_1656279599.c"
        "tach_ui/fonts/ui_font_DotoLarge.c"
        "tach_ui/fonts/ui_font_Doto_48.c"
        "tach_ui/fonts/school_bell_48.c"
        "tach_ui/images/ui_img_sti_logo_png.c"
        "tach_ui/boot/boot_screen.c"
        "odometer/odometer.c"
        "lap_timer/lap_timer.c"
        "gps_wrap/gps_wrapper.cpp"
        "canbus/canbus.c"
        ${PROTOCOL_LIST_FILE}

    INCLUDE_DIRS 
        "./tach_ui"
        "./lap_timer"
        "./odometer"
        "./gps_wrap"
        "./canbus"
        "."
    
    EMBED_TXTFILES
        ${CAN_PROTOCOL_JSON}
    
)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB} 
    PRIVATE 
        -DLV_LVGL_H_INCLUDE_SIMPLE
        -DLV_USE_DEMO_MUSIC
)
