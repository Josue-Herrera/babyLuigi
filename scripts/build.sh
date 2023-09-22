cmake -S . -B ../build -Wno-dev
cmake --build ../build  --parallel --target all --clean-first
