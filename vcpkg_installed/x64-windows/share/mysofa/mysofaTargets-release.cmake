#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mysofa::mysofa-shared" for configuration "Release"
set_property(TARGET mysofa::mysofa-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mysofa::mysofa-shared PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/mysofa_shared.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/mysofa.dll"
  )

list(APPEND _cmake_import_check_targets mysofa::mysofa-shared )
list(APPEND _cmake_import_check_files_for_mysofa::mysofa-shared "${_IMPORT_PREFIX}/lib/mysofa_shared.lib" "${_IMPORT_PREFIX}/bin/mysofa.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
