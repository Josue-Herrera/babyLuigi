# Devcontainer for C++ with LLVM/Clang and IWYU
# Note: This Dockerfile includes a corrected IWYU build step that
# passes explicit CMake variables for compilers and LLVM/Clang configs
# to avoid configure failures.

FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04

ARG DEBIAN_FRONTEND=noninteractive

# Adjustable versions and paths
ARG LLVM_VER=17
ARG IWYU_SRC=/opt/iwyu/src
ARG IWYU_BUILD=/opt/iwyu/build

# Install LLVM/Clang toolchain and dependencies
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        wget gnupg ca-certificates lsb-release software-properties-common \
    && update-ca-certificates \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh ${LLVM_VER} \
    && rm -f llvm.sh \
    && apt-get install -y --no-install-recommends \
        build-essential cmake git ninja-build \
        clang-${LLVM_VER} clang-tools-${LLVM_VER} \
        llvm-${LLVM_VER} llvm-${LLVM_VER}-dev libclang-${LLVM_VER}-dev \
    && rm -rf /var/lib/apt/lists/*

# Ensure the selected LLVM binaries are first in PATH
ENV PATH="/usr/lib/llvm-${LLVM_VER}/bin:${PATH}"

# Clone IWYU matching the LLVM version (branch naming: clang_<ver>)
RUN git clone --depth 1 -b clang_${LLVM_VER} \
        https://github.com/include-what-you-use/include-what-you-use.git \
        "${IWYU_SRC}"

# Build and install IWYU with explicit CMake variables
# This fixes the previous failure by pointing CMake to LLVM/Clang configs
RUN cmake -S "${IWYU_SRC}" \
        -B "${IWYU_BUILD}" \
        -G "Unix Makefiles" \
        -DCMAKE_C_COMPILER="clang-${LLVM_VER}" \
        -DCMAKE_CXX_COMPILER="clang++-${LLVM_VER}" \
        -DCMAKE_PREFIX_PATH="/usr/lib/llvm-${LLVM_VER}" \
        -DLLVM_DIR="/usr/lib/llvm-${LLVM_VER}/lib/cmake/llvm" \
        -DClang_DIR="/usr/lib/llvm-${LLVM_VER}/lib/cmake/clang" \
    && cmake --build "${IWYU_BUILD}" -j \
    && cmake --install "${IWYU_BUILD}"

