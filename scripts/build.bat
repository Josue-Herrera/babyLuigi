cmake -S .. -B ..\build -G "Visual Studio 17 2022" -A x64
cmake --build ..\build  --parallel --clean-first
