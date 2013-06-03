# Find libircclient, this modules defines:
# LIBIRCCLIENT_INCLUDE_DIR, where to find libircclient.h
# LIBIRCCLIENT_LIBRARIES, where to find library
# LIBIRCCLIENT_FOUND, if it is found

find_package(OpenSSL REQUIRED)

# find libircclient.h
find_path(
    LIBIRCCLIENT_INCLUDE_DIR libircclient.h
    PATHS
    /usr/include
    /usr/local/include
)

# find libircclient.so
find_library(
    LIBIRCCLIENT_LIBRARY NAMES libircclient ircclient
    PATHS_SUFFIXES lib lib64
    PATHS
    /usr/
    /usr/local/
)

include(FindPackageHandleStandardArgs)

# libircclient needs SSL
if (LIBIRCCLIENT_LIBRARY AND OPENSSL_LIBRARIES)
	set(
		LIBIRCCLIENT_LIBRARIES
		${LIBIRCCLIENT_LIBRARY}
		${OPENSSL_LIBRARIES}
	)
endif ()

find_package_handle_standard_args(
    LIBIRCCLIENT
    REQUIRED_VARS LIBIRCCLIENT_INCLUDE_DIR LIBIRCCLIENT_LIBRARIES
)

mark_as_advanced(LIBIRCCLIENT_INCLUDE_DIR LIBIRCCLIENT_LIBRARY)
