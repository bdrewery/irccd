# Find libircclient, this modules defines:
# LIBIRCCLIENT_INCLUDE_DIR, where to find libircclient.h
# LIBIRCCLIENT_LIBRARIES, where to find library
# LIBIRCCLIENT_FOUND, if it is found

find_package(OpenSSL REQUIRED)

# find libircclient.h
find_path(
	LIBIRCCLIENT_INCLUDE_DIR libircclient.h
	PATH_SUFFIXES include/libircclient libircclient
	PATHS
	/usr/include
	/usr/local/include
)

# find libircclient.so
find_library(
	LIBIRCCLIENT_LIBRARY NAMES libircclient ircclient
	PATH_SUFFIXES lib lib64
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
	set(
		LIBIRCCLIENT_INCLUDE_DIRS
		${LIBIRCCLIENT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
	)
endif ()

find_package_handle_standard_args(
	Libircclient
	REQUIRED_VARS LIBIRCCLIENT_INCLUDE_DIRS LIBIRCCLIENT_LIBRARIES
)

mark_as_advanced(LIBIRCCLIENT_INCLUDE_DIRS LIBIRCCLIENT_LIBRARIES)
