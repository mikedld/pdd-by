# adding path since macports' cmake finds iconv.h in /opt/local/include insread of system one
find_path(ICONV_INCLUDE_DIR NAMES iconv.h)

include(CheckFunctionExists)
check_function_exists(iconv_open ICONV_IN_GLIBC)

if(ICONV_IN_GLIBC)
    set(ICONV_FOUND 1)
    message(STATUS "Found ICONV in glibc")
else()
    find_library(ICONV_LIBRARY NAMES iconv)

    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(ICONV DEFAULT_MSG ICONV_LIBRARY ICONV_INCLUDE_DIR)
endif()

if(ICONV_FOUND)
    set(ICONV_LIBRARIES ${ICONV_LIBRARY})
    set(ICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR})
else()
    set(ICONV_LIBRARIES)
    set(ICONV_INCLUDE_DIRS)
endif()

mark_as_advanced(ICONV_INCLUDE_DIRS ICONV_LIBRARIES)
