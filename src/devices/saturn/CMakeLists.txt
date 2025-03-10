add_library(SaturnObjLib OBJECT saturn_psl.c saturn.c)
target_include_directories(SaturnObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(SaturnObjLib PUBLIC ${PROTOCOL_EXCLUSIONS})