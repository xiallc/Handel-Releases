add_library(AssertObjLib OBJECT xia_assert.c)
target_compile_definitions(AssertObjLib PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(AssertObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)

add_library(FileObjLib OBJECT xia_file.c)
target_compile_definitions(FileObjLib PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(FileObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)