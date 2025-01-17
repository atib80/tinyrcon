cmake_minimum_required(VERSION 3.22)

project(TinyRcon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

add_compile_options("/permissive- /MP /GS /GL /analyze- /W4 /Gy /Zc:wchar_t /Gm- /O2 /Ob2 /sdl /Zc:inline /fp:fast /DWIN32 /DNDEBUG /D_WINDOWS /D_MBCS /D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06030000 /DWINVER=0x0A00 /D_WIN32_WINNT=0x0A00 /errorReport:prompt /GF /WX- /Zc:forScope /Gd /Oy- /Oi /MT /std:c++20 /FC /EHsc /nologo /Ot /diagnostics:column")

add_subdirectory(TinyRconClient)
add_subdirectory(TinyRconPrivateClient)
add_subdirectory(TinyRconPrivateClient_v2)
add_subdirectory(TinyRconPrivateServer)
add_subdirectory(TinyRconServer)