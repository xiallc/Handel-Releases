add_library(MercuryObjLib OBJECT mercury_psl.c mercury.c)
target_include_directories(MercuryObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(MercuryObjLib PUBLIC ${PROTOCOL_EXCLUSIONS})