
set(TARGET_NAME dht11_pio_main)
add_executable(${TARGET_NAME}
    main.c
    )

add_subdirectory(dht11_pio)

target_link_libraries(${TARGET_NAME} PRIVATE
    pico_stdlib
    hardware_pio
    dht11_pio
    )

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/dht11_pio
    )

pico_enable_stdio_usb(${TARGET_NAME} 0)
pico_enable_stdio_uart(${TARGET_NAME} 0)
pico_add_extra_outputs(${TARGET_NAME})
