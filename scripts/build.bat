cmake -S . -B ..\build -G "Visual Studio 17 2022" -Wno-dev
cmake --build ..\build  --parallel --target all --clean-first
