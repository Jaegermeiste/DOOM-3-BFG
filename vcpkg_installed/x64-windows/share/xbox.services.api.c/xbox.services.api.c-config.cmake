get_filename_component(_ext_root "${CMAKE_CURRENT_LIST_DIR}" PATH)
get_filename_component(_ext_root "${_ext_root}" PATH)

set(_ext_lib "${_ext_root}/lib/Microsoft.Xbox.Services.142.GDK.C.lib")
if (EXISTS "${_ext_lib}")

    set(_dbg_lib "${_ext_root}/debug/lib/Microsoft.Xbox.Services.142.GDK.C.lib")
    if (NOT (EXISTS "${_dbg_lib}"))
      set(_dbg_lib "${_ext_lib}")
    endif()

    add_library(Xbox::XSAPI STATIC IMPORTED)
    set_target_properties(Xbox::XSAPI PROPERTIES
      IMPORTED_LOCATION_RELEASE          "${_ext_lib}"
      IMPORTED_LOCATION_DEBUG            "${_dbg_lib}"
      IMPORTED_CONFIGURATIONS            "Debug;Release"
      MAP_IMPORTED_CONFIG_MINSIZEREL     Release
      MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
      INTERFACE_INCLUDE_DIRECTORIES      "${_ext_root}/include"
      IMPORTED_LINK_INTERFACE_LANGUAGES  "CXX")

    target_link_libraries(Xbox::XSAPI INTERFACE appnotify.lib winhttp.lib crypt32.lib)

    if(TARGET Xbox::XCurl)
      target_link_libraries(Xbox::XSAPI INTERFACE Xbox::XCurl)
    endif()

    if(TARGET Xbox::HTTPClient)
      target_link_libraries(Xbox::XSAPI INTERFACE Xbox::HTTPClient)
    endif()

    set(Xbox.Services.API.C_FOUND TRUE)

else()

    set(Xbox.Services.API.C_FOUND FALSE)

endif()

unset(_dbg_lib)
unset(_ext_lib)
unset(_ext_root)
