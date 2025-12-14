

SET CMAKE_DEFS=^
-DCMAKE_MAKE_PROGRAM="C:\Program Files\JetBrains\CLion 2024.1.4\bin\ninja\win\x64\ninja.exe" ^
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
-DWITH_TLS:BOOL=OFF ^
-DSTDEXEC_BUILD_EXAMPLES:BOOL=OFF ^
-DLIBZMQ_WERROR:BOOL=ON ^
-DENABLE_PRECOMPILED:BOOL=OFF ^
-DCMAKE_AR="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.40.33807/bin/Hostx64/x64/lib.exe" ^
-DCMAKE_BUILD_TYPE=Debug ^
-DCMAKE_COLOR_DIAGNOSTICS=ON ^
-DCMAKE_CXX_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.40.33807/bin/Hostx64/x64/cl.exe" ^
-DCMAKE_CXX_FLAGS="/DWIN32 /D_WINDOWS /EHsc" ^
-DCMAKE_CXX_FLAGS_DEBUG="/Ob0 /Od /RTC1" ^
-DCMAKE_CXX_FLAGS_MINSIZEREL="/O1 /Ob1 /DNDEBUG" ^
-DCMAKE_CXX_FLAGS_RELEASE="/O2 /Ob2 /DNDEBUG" ^
-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/O2 /Ob1 /DNDEBUG" ^
-DCMAKE_CXX_STANDARD_LIBRARIES="kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib" ^
-DCMAKE_C_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.40.33807/bin/Hostx64/x64/cl.exe" ^
-DCMAKE_C_FLAGS="/DWIN32 /D_WINDOWS" ^
-DCMAKE_C_FLAGS_DEBUG="/Ob0 /Od /RTC1" ^
-DCMAKE_C_FLAGS_MINSIZEREL="/O1 /Ob1 /DNDEBUG" ^
-DCMAKE_C_FLAGS_RELEASE="/O2 /Ob2 /DNDEBUG" ^
-DCMAKE_C_FLAGS_RELWITHDEBINFO="/O2 /Ob1 /DNDEBUG" ^
-DCMAKE_C_STANDARD_LIBRARIES="kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib" ^
-DCMAKE_EXE_LINKER_FLAGS=/machine:x64 ^
-DCMAKE_EXE_LINKER_FLAGS_DEBUG="/debug /INCREMENTAL" ^
-DCMAKE_EXE_LINKER_FLAGS_MINSIZEREL=/INCREMENTAL:NO ^
-DCMAKE_EXE_LINKER_FLAGS_RELEASE=/INCREMENTAL:NO ^
-DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO="/debug /INCREMENTAL"

cmake -S .. -B ..\build -G Ninja %CMAKE_DEFS%
cmake --build ..\build  --parallel --clean-first
