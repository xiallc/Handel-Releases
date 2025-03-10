add_library(PlxObjLib OBJECT xia_plx.c)
target_compile_definitions(PlxObjLib PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(PlxObjLib PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PLX_INCLUDE_DIR}
)