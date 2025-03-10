set(handel_release_headers
        Dlldefs.h
        handel.h
        handeldef.h
        handel_constants.h
        handel_errors.h
        handel_generic.h
        md_generic.h
        xia_common.h
        xia_version.h
)

install(FILES ${handel_release_headers} DESTINATION inc)