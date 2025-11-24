add_executable(test_utils src/test_utils.c)
target_link_libraries(test_utils)
target_include_directories(test_utils PUBLIC
        ${PROJECT_SOURCE_DIR}/inc/
        ${PROJECT_SOURCE_DIR}/externals/acutest/
)