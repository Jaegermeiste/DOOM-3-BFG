#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Microsoft::DirectXTK12" for configuration "Release"
set_property(TARGET Microsoft::DirectXTK12 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Microsoft::DirectXTK12 PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/DirectXTK12.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/DirectXTK12.dll"
  )

list(APPEND _cmake_import_check_targets Microsoft::DirectXTK12 )
list(APPEND _cmake_import_check_files_for_Microsoft::DirectXTK12 "${_IMPORT_PREFIX}/lib/DirectXTK12.lib" "${_IMPORT_PREFIX}/bin/DirectXTK12.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
