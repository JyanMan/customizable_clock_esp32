cmake_minimum_required(VERSION 3.22)

add_compile_definitions(LV_CONF_PATH="${CMAKE_SOURCE_DIR}/main/include/lv_conf.h")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(spi_lcd_touch)
