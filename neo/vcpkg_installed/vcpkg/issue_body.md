Package: zlib:x64-windows@1.3.1

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35217.0
- CMake Version: 3.31.5
-    vcpkg-tool version: 2025-10-10-09baed229fa02ec5242fcf8e0cada24c6a81b0d7
    vcpkg-scripts version: 9f7500be5d 2022-10-27 (3 years ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/madler/zlib/archive/v1.3.1.tar.gz -> madler-zlib-v1.3.1.tar.gz
Successfully downloaded madler-zlib-v1.3.1.tar.gz
-- Extracting source C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/downloads/madler-zlib-v1.3.1.tar.gz
-- Applying patch 0001-Prevent-invalid-inclusions-when-HAVE_-is-set-to-0.patch
-- Applying patch 0002-build-static-or-shared-not-both.patch
-- Applying patch 0003-android-and-mingw-fixes.patch
-- Using source at C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/buildtrees/zlib/src/v1.3.1-2e5db616bf.clean
-- Configuring x64-windows
-- Building x64-windows-dbg
-- Building x64-windows-rel
-- Installing: C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/packages/zlib_x64-windows/share/zlib/vcpkg-cmake-wrapper.cmake
-- Fixing pkgconfig file: C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/packages/zlib_x64-windows/lib/pkgconfig/zlib.pc
CMake Error at scripts/cmake/vcpkg_find_acquire_program.cmake:123 (message):
  unknown tool PKGCONFIG -- unable to acquire.
Call Stack (most recent call first):
  scripts/cmake/vcpkg_fixup_pkgconfig.cmake:193 (vcpkg_find_acquire_program)
  C:/Users/jason/AppData/Local/vcpkg/registries/git-trees/3f05e04b9aededb96786a911a16193cdb711f0c9/portfile.cmake:43 (vcpkg_fixup_pkgconfig)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "dependencies": [
    "ms-gsl",
    "directxmath",
    "directxmesh",
    {
      "name": "directxtex",
      "features": [
        "dx12",
        "openexr"
      ]
    },
    "directxtk12",
    "magic-enum",
    {
      "name": "minizip-ng",
      "features": [
        "bzip2",
        "lzma",
        "zlib",
        "zstd"
      ]
    },
    "steam-audio",
    "zlib-ng",
    "libjpeg-turbo",
    "ms-gdk"
  ]
}

```
</details>
