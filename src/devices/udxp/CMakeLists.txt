add_library(UdxpCommonObjLib OBJECT udxp_common.c)
target_include_directories(UdxpCommonObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(UdxpCommonObjLib PUBLIC
        ${DEVICE_EXCLUSIONS}
        ${PROTOCOL_EXCLUSIONS}
        ${LIBRARY_EXCLUSIONS}
)

if (UDXP)
    add_library(UdxpObjLib OBJECT udxp_psl.c udxp.c)
    target_include_directories(UdxpObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
    target_compile_definitions(UdxpObjLib PUBLIC
            ${DEVICE_EXCLUSIONS}
            ${PROTOCOL_EXCLUSIONS}
            ${LIBRARY_EXCLUSIONS}
    )
endif ()

if (UDXPS)
    add_library(UdxpsObjLib OBJECT udxps_psl.c udxps.c)
    target_include_directories(UdxpsObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
    target_compile_definitions(UdxpsObjLib PUBLIC
            ${DEVICE_EXCLUSIONS}
            ${PROTOCOL_EXCLUSIONS}
            ${LIBRARY_EXCLUSIONS}
    )
endif ()