set(NETRELAY_SOURCES
    netrelay.c
)

add_executable(netrelay ${NETRELAY_SOURCES})
set_target_properties(netrelay PROPERTIES
    LINKER_LANGUAGE C
)
