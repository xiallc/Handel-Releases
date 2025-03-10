# Source: https://github.com/khozaei/gsmapp/tree/29bfdcfe8c758525560cff934e928cc529e5c49f
# We've made the following modifications:
# 1. Updated the command that gets the tag to append the "-dirty" pre-release if there
#    are local uncommitted changes to the repository.
# 2. Sanitize the xray-monorepo prefix of `handel-` to clean the handel tag.
# 3. Remove the FULL_VERSION in favor of the PROJECT_VERSION being the FULL_VERSION.
function(get_version_from_git)
    find_package(Git QUIET)
    if (NOT Git_FOUND)
        message(WARNING "Git not found")
        return()
    endif ()

    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags --abbrev=0 --dirty=-dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE GIT_RESULT
            ERROR_QUIET
    )

    if (NOT GIT_RESULT EQUAL 0)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 --dirty=-dirty
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE GIT_RESULT
                ERROR_QUIET
        )
    endif ()

    if (NOT GIT_RESULT EQUAL 0)
        message(WARNING "failed to get tag, setting unknown version")
        set(PROJECT_VERSION_MAJOR 0 PARENT_SCOPE)
        set(PROJECT_VERSION_MINOR 0 PARENT_SCOPE)
        set(PROJECT_VERSION_PATCH 0 PARENT_SCOPE)
        set(PROJECT_VERSION "0.0.0-unknown" PARENT_SCOPE)
        return()
    endif ()

    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_SHORT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )

    string(REGEX REPLACE "^handel-" "" CLEAN_TAG "${GIT_TAG}")
    if (CLEAN_TAG MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-.*)?$")
        set(PROJECT_VERSION "${CLEAN_TAG}" PARENT_SCOPE)
        set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1} PARENT_SCOPE)
        set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
        set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2} PARENT_SCOPE)
        set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
        set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3} PARENT_SCOPE)
        set(PROJECT_VERSION_BUILD ${GIT_COMMIT_SHORT_HASH} PARENT_SCOPE)

        if (CLEAN_TAG MATCHES "\-dirty")
            set(PROJECT_VERSION "${CLEAN_TAG}+${GIT_COMMIT_SHORT_HASH}" PARENT_SCOPE)
        endif ()
    else ()
        message(WARNING "Tag '${CLEAN_TAG}' does not match semver format")
    endif ()
endfunction()