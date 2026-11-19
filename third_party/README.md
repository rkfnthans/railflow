# Runtime dependency notices

The bundled camera programs statically link OpenCV 4.12.0 and its selected JPEG,
PNG and zlib dependencies. Windows programs use the LLVM-MinGW C++ runtime.
Corresponding upstream notices are included in `licenses` and in runtime packages.

The Windows OpenCV build includes the MinGW Media Foundation compatibility changes
in `licenses/opencv-mingw-msmf.patch`. The source project can use an installed
OpenCV 4 development package. Upstream source is available from
[OpenCV](https://github.com/opencv/opencv/tree/4.12.0) and
[LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw).
