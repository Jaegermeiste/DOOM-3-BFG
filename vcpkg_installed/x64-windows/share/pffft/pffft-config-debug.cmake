#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pffft::pffft" for configuration "Debug"
set_property(TARGET pffft::pffft APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(pffft::pffft PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/pffft.lib"
  )

list(APPEND _cmake_import_check_targets pffft::pffft )
list(APPEND _cmake_import_check_files_for_pffft::pffft "${_IMPORT_PREFIX}/debug/lib/pffft.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
