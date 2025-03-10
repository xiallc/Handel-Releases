add_library(UsbObjLib OBJECT xia_usb.c)
target_include_directories(UsbObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/externals/cypress/drv/usb1
        DESTINATION drivers/cypress)