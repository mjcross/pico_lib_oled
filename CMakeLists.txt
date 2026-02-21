add_library(pico_lib_oled INTERFACE)

target_sources(pico_lib_oled INTERFACE
    lib_oled.c
    )

target_include_directories(pico_lib_oled INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    )

target_link_libraries(pico_lib_oled INTERFACE
    pico_stdlib
    hardware_spi
    hardware_dma
    )

# enable debug messages for CMAKE_BUILD_TYPE debug
# see https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#build-configurations
target_compile_definitions(pico_lib_oled INTERFACE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
)