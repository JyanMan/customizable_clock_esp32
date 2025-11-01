file(GLOB SRC_FILES
    "main.c"
    "src/*.c"
)

idf_component_register(SRCS ${SRC_FILES}
                       INCLUDE_DIRS "./include")
