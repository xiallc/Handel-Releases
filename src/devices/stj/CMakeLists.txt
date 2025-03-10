add_library(StjObjLib OBJECT stj_psl.c stj.c)
target_include_directories(StjObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(StjObjLib PUBLIC ${PROTOCOL_EXCLUSIONS})