if (EPP)
    add_subdirectory("protocols/epp")
endif ()

if (BroadcomAPI_FOUND)
    add_subdirectory("protocols/plx")
endif ()

if (SERIAL)
    add_subdirectory("protocols/serial")
endif ()

if (USB)
    add_subdirectory("protocols/usb")
endif ()

if (USB2)
    add_subdirectory("protocols/usb2")
endif ()

if (MERCURY)
    add_subdirectory("devices/mercury")
endif ()

if (SATURN)
    add_subdirectory("devices/saturn")
endif ()

if (STJ)
    add_subdirectory("devices/stj")
endif ()

if (UDXP OR UDXPS)
    add_subdirectory("devices/udxp")
endif ()

if (XMAP)
    add_subdirectory("devices/xmap")
endif ()

add_subdirectory(md)
add_subdirectory(utils)
add_subdirectory(xerxes)

if (XUP AND (UDXP OR UDXPS))
    add_subdirectory(xup)
endif ()

add_library(SupportObjLib OBJECT
        fdd.c
        psl.c
)
target_compile_definitions(SupportObjLib PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(SupportObjLib PUBLIC ${PROJECT_SOURCE_DIR}/inc)

add_subdirectory(handel)
add_library(handel SHARED
        $<TARGET_OBJECTS:HandelObjLib>
        # General Object Libraries
        $<TARGET_OBJECTS:AssertObjLib>
        $<TARGET_OBJECTS:FileObjLib>
        $<TARGET_OBJECTS:MdObjLib>
        $<TARGET_OBJECTS:SupportObjLib>
        $<TARGET_OBJECTS:XerxesObjLib>
        $<$<TARGET_EXISTS:XupObjLib>:$<TARGET_OBJECTS:XupObjLib>>
        # Protocol object libraries
        $<$<TARGET_EXISTS:EppObjLib>:$<TARGET_OBJECTS:EppObjLib>>
        $<$<TARGET_EXISTS:PlxObjLib>:$<TARGET_OBJECTS:PlxObjLib>>
        $<$<TARGET_EXISTS:SerialObjLib>:$<TARGET_OBJECTS:SerialObjLib>>
        $<$<TARGET_EXISTS:UsbObjLib>:$<TARGET_OBJECTS:UsbObjLib>>
        $<$<TARGET_EXISTS:Usb2ObjLib>:$<TARGET_OBJECTS:Usb2ObjLib>>
        # Device object libraries
        $<$<TARGET_EXISTS:MercuryObjLib>:$<TARGET_OBJECTS:MercuryObjLib>>
        $<$<TARGET_EXISTS:SaturnObjLib>:$<TARGET_OBJECTS:SaturnObjLib>>
        $<$<TARGET_EXISTS:StjObjLib>:$<TARGET_OBJECTS:StjObjLib>>
        $<$<TARGET_EXISTS:UdxpCommonObjLib>:$<TARGET_OBJECTS:UdxpCommonObjLib>>
        $<$<TARGET_EXISTS:UdxpObjLib>:$<TARGET_OBJECTS:UdxpObjLib>>
        $<$<TARGET_EXISTS:UdxpsObjLib>:$<TARGET_OBJECTS:UdxpsObjLib>>
        $<$<TARGET_EXISTS:XmapObjLib>:$<TARGET_OBJECTS:XmapObjLib>>
)

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    target_sources(handel PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../handel.rc)
endif ()

set_property(TARGET handel PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(handel PUBLIC
        ${PROJECT_SOURCE_DIR}/inc/
        ${EPP_INC_DIR}
        ${PLX_INCLUDE_DIR}
        ${SERIAL_INC_DIR}
)

target_compile_definitions(handel PUBLIC
        ${GENERAL_COMPILE_DEFS}
        ${DEVICE_EXCLUSIONS}
        ${PROTOCOL_EXCLUSIONS}
        ${LIBRARY_EXCLUSIONS}
)

if (MSVC)
    target_link_options(handel PRIVATE
            $<$<NOT:$<BOOL:${X64}>>:/SAFESEH:NO>
            $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )
endif ()

target_link_directories(handel PUBLIC
        ${EPP_LIB_DIR}
        ${PLX_LIBRARY_DIR}
        ${SERIAL_LIB_DIR}
)

target_link_libraries(handel PUBLIC
        ${EPP_LIB}
        ${SERIAL_LIBS}
        ${USB2_LIB}
        ${PLX_STATIC_LIB}
        $<$<C_COMPILER_ID:GNU>:"m">
)

sign_code(handel)
install(TARGETS handel LIBRARY DESTINATION lib)
