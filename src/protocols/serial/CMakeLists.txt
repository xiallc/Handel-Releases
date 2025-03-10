set(SERIAL_BASE_DIR ${PROJECT_SOURCE_DIR}/externals/comm-drv)
set(SERIAL_LIB_DIR ${SERIAL_BASE_DIR}/lib PARENT_SCOPE)
set(SERIAL_INC_DIR ${SERIAL_BASE_DIR}/inc PARENT_SCOPE)
set(SERIAL_LIBS cdrvdl32 cdrvhf32 cdrvxf32 PARENT_SCOPE)

add_library(SerialObjLib OBJECT xia_serial.c)
target_include_directories(SerialObjLib PUBLIC
        ${PROJECT_SOURCE_DIR}/externals/comm-drv/inc
        ${PROJECT_SOURCE_DIR}/inc
)

install(FILES
        ${SERIAL_BASE_DIR}/bin/cdrvdl32.dll
        ${SERIAL_BASE_DIR}/bin/cdrvhf32.dll
        DESTINATION bin
)