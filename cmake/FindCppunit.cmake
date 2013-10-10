# Find cppunit, this modules defines:
# CPPUNIT_INCLUDE_DIR, where to find cppunit/TestCase.h
# CPPUNIT_LIBRARY, where to find library
# CPPUNIT_FOUND, if it is found

# find cppunit/TestCase.h
find_path(
    CPPUNIT_INCLUDE_DIR cppunit/TestCase.h
    PATHS
    /usr/include
    /usr/local/include
)

# find libcppunit.so
find_library(
    CPPUNIT_LIBRARY NAMES libcppunit cppunit
    PATHS_SUFFIXES lib lib64
    PATHS
    /usr/
    /usr/local/
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    CPPUNIT
    REQUIRED_VARS CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY
)

mark_as_advanced(CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY)
