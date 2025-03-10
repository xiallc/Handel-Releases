add_library(XupObjLib OBJECT
        xup.c
        xup_writer.c
)
target_compile_definitions(XupObjLib PUBLIC __EXPORT)
target_include_directories(XupObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)