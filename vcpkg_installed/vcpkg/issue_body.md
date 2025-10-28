Package: directx-headers:x64-windows@1.618.2

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35219.0
- CMake Version: 3.31.5
-    vcpkg-tool version: 2025-10-10-09baed229fa02ec5242fcf8e0cada24c6a81b0d7
    vcpkg-scripts version: 123c6ca1f1 2025-10-16 (9 days ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
-- Using cached Microsoft-DirectX-Headers-v1.618.2.tar.gz
-- Cleaning sources at C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/buildtrees/directx-headers/src/v1.618.2-0d044a4cd1.clean. Use --editable to skip cleaning for the packages you specify.
-- Extracting source C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/downloads/Microsoft-DirectX-Headers-v1.618.2.tar.gz
-- Using source at C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg/buildtrees/directx-headers/src/v1.618.2-0d044a4cd1.clean
CMake Error at scripts/cmake/vcpkg_host_path_list.cmake:60 (message):
  Operation REMOVE_DUPLICATES not recognized.
Call Stack (most recent call first):
  K:/Repositories/DOOM-3-BFG/vcpkg_installed/x64-windows/share/vcpkg-cmake/vcpkg_cmake_configure.cmake:246 (vcpkg_host_path_list)
  C:/Users/jason/AppData/Local/vcpkg/registries/git-trees/db0687285536cd47b693f0fc56a4b495e670b7b6/portfile.cmake:9 (vcpkg_cmake_configure)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "dependencies": [
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
    "libjpeg-turbo",
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
    "ms-gdk",
    "ms-gsl",
    "steam-audio",
    "zlib-ng",
    "directx12-agility"
  ]
}

```
</details>
