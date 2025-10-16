#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Microsoft::DirectXMesh" for configuration "Release"
set_property(TARGET Microsoft::DirectXMesh APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Microsoft::DirectXMesh PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/DirectXMesh.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/DirectXMesh.dll"
  )

list(APPEND _cmake_import_check_targets Microsoft::DirectXMesh )
list(APPEND _cmake_import_check_files_for_Microsoft::DirectXMesh "${_IMPORT_PREFIX}/lib/DirectXMesh.lib" "${_IMPORT_PREFIX}/bin/DirectXMesh.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
