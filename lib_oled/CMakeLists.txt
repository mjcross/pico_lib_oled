add_library(lib_oled INTERFACE)

target_sources(lib_oled INTERFACE
    lib_oled.c
    )

target_link_libraries(lib_oled INTERFACE
    pico_stdlib
    hardware_spi
    hardware_dma
    )
