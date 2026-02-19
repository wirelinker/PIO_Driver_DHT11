
set(TARGET_NAME dht11_pio)
add_library(${TARGET_NAME}
    dht11_pio.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_stdlib
    hardware_pio
    )

pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/dht11.pio)

