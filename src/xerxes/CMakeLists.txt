add_library(XerxesObjLib OBJECT xerxes.c xerxes_io.c)
target_include_directories(XerxesObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(XerxesObjLib PUBLIC
        ${PROTOCOL_EXCLUSIONS}
        ${DEVICE_EXCLUSIONS}
)


