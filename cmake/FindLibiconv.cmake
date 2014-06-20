# Find libiconv used by gettext, this modules defines:
# Libiconv_INCLUDE_DIR, where to find iconv.h
# Libiconv_LIBRARY, where to find library
# Libiconv_FOUND, if it is found

include(CheckFunctionExists)

# find iconv.h
find_path(
    Libiconv_INCLUDE_DIR iconv.h
    PATHS
    /usr/include
    /usr/local/include
)

# find libiconv.so
find_library(
    Libiconv_LIBRARY NAMES libiconv iconv
    PATHS_SUFFIXES lib lib64
    PATHS
    /usr/
    /usr/local/
)

# Sometimes iconv is directly in the library
if (NOT Libiconv_LIBRARY)
	check_function_exists(iconv_open _HAVE_ICONV_OPEN_FUNC)
	check_function_exists(iconv_close _HAVE_ICONV_CLOSE_FUNC)
	check_function_exists(iconv _HAVE_ICONV_FUNC)

	if (_HAVE_ICONV_OPEN_FUNC AND _HAVE_ICONV_CLOSE_FUNC AND _HAVE_ICONV_FUNC)
		set(Libiconv_LIBRARY "")
	endif ()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
	Libiconv
	FOUND_VAR Libiconv_FOUND
	REQUIRED_VARS Libiconv_LIBRARY Libiconv_INCLUDE_DIR
)

mark_as_advanced(Libiconv_INCLUDE_DIR Libiconv_LIBRARY)
