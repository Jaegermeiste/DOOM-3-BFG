get_filename_component(_ext_root "${CMAKE_CURRENT_LIST_DIR}" PATH)
get_filename_component(_ext_root "${_ext_root}" PATH)

set(_ext_lib "${_ext_root}/lib/GameChat2.lib")
if (EXISTS "${_ext_lib}")

    add_library(Xbox::GameChat2 SHARED IMPORTED)
    set_target_properties(Xbox::GameChat2 PROPERTIES
      IMPORTED_LOCATION                  "${_ext_root}/bin/GameChat2.dll"
      IMPORTED_IMPLIB                    "${_ext_lib}"
      MAP_IMPORTED_CONFIG_MINSIZEREL     ""
      MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
      INTERFACE_INCLUDE_DIRECTORIES      "${_ext_root}/include")

    set(Xbox.Game.Chat.2.Cpp.API_FOUND TRUE)

else()

    set(Xbox.Game.Chat.2.Cpp.API_FOUND FALSE)

endif()

unset(_ext_lib)
unset(_ext_root)
