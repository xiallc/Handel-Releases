add_library(HandelObjLib OBJECT
        handel.c
        handel_dbg.c
        handel_detchan.c
        handel_dyn_default.c
        handel_dyn_detector.c
        handel_dyn_firmware.c
        handel_dyn_module.c
        handel_error.c
        handel_file.c
        handel_log.c
        handel_run_control.c
        handel_run_params.c
        handel_sort.c
        handel_system.c
        handel_xerxes.c
        ${CMAKE_SOURCE_DIR}/inc/xia_version.h)
target_include_directories(HandelObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(HandelObjLib PUBLIC
        ${GENERAL_COMPILE_DEFS}
        ${DEVICE_EXCLUSIONS}
        ${PROTOCOL_EXCLUSIONS}
        ${LIBRARY_EXCLUSIONS}
)