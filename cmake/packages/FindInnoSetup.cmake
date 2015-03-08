# FindInnoSetup
# -------------
#
# Find InnoSetup builder, this module defines:
#
# InnoSetup_COMPILER, where to find iscc.exe
# InnoSetup_FOUND, if the InnoSetup installation was found
#

find_program(
	InnoSetup_COMPILER
	NAMES iscc
	DOC "InnoSetup command line compiler"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
	InnoSetup
	FOUND_VAR InnoSetup_FOUND
	REQUIRED_VARS InnoSetup_COMPILER
)

mark_as_advanced(InnoSetup_COMPILER)
