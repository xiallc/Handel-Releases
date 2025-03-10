add_library(XmapObjLib OBJECT xmap_psl.c xmap.c)
target_include_directories(XmapObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_compile_definitions(XmapObjLib PUBLIC ${PROTOCOL_EXCLUSIONS})