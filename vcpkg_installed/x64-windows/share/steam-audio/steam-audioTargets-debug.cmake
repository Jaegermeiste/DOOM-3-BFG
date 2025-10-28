#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::steam-audio::phonon" for configuration "Debug"
set_property(TARGET unofficial::steam-audio::phonon APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::steam-audio::phonon PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/phonon.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/phonon.dll"
  )

list(APPEND _cmake_import_check_targets unofficial::steam-audio::phonon )
list(APPEND _cmake_import_check_files_for_unofficial::steam-audio::phonon "${_IMPORT_PREFIX}/debug/lib/phonon.lib" "${_IMPORT_PREFIX}/debug/bin/phonon.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
