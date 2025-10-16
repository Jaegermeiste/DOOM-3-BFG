get_filename_component(_ext_root "${CMAKE_CURRENT_LIST_DIR}" PATH)
get_filename_component(_ext_root "${_ext_root}" PATH)

set(_ext_lib "${_ext_root}/lib/xgameruntime.lib")
if (EXISTS "${_ext_lib}")

    add_library(Xbox::GameRuntime STATIC IMPORTED)
    set_target_properties(Xbox::GameRuntime PROPERTIES
      IMPORTED_LOCATION                  "${_ext_lib}"
      MAP_IMPORTED_CONFIG_MINSIZEREL     ""
      MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
      INTERFACE_INCLUDE_DIRECTORIES      "${_ext_root}/include"
      INTERFACE_COMPILE_FEATURES         "cxx_std_11"
      IMPORTED_LINK_INTERFACE_LANGUAGES  "CXX")

    set(Xbox.GameRuntime_FOUND TRUE)

else()

    set(Xbox.GameRuntime_FOUND FALSE)

endif()

unset(_ext_lib)
unset(_ext_root)
