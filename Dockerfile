# Dockerfile for LumenOS deterministic build environment
# This Dockerfile pins all toolchain versions to ensure reproducible builds.
# Build: docker build -f Dockerfile -t lumenos-build:latest .
# Usage: docker run --rm -v $(pwd):/workspace lumenos-build:latest make build

FROM ubuntu:22.04

LABEL maintainer="LumenOS Team"
LABEL description="LumenOS reproducible build environment with pinned toolchain versions"

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Set toolchain environment variables (match toolchain.json)
ENV CC=clang-18
ENV CXX=clang++-18
ENV LD=ld.lld-18
ENV AR=llvm-ar-18
ENV OBJCOPY=llvm-objcopy-18
ENV OBJDUMP=llvm-objdump-18

# Base system updates and essential tools
RUN apt-get update -y && \
    apt-get install -y \
      ca-certificates \
      curl \
      git \
      wget \
      software-properties-common \
      gnupg \
      lsb-release && \
    rm -rf /var/lib/apt/lists/*

# Add LLVM repository (version 18)
RUN curl https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main"

# Install LLVM 18 toolchain pinned versions
RUN apt-get update -y && \
    apt-get install -y \
      clang-18=1:18.2.0-1~ubuntu0.22.04.1 \
      llvm-18=1:18.2.0-1~ubuntu0.22.04.1 \
      lld-18=1:18.2.0-1~ubuntu0.22.04.1 \
      llvm-18-tools=1:18.2.0-1~ubuntu0.22.04.1 && \
    rm -rf /var/lib/apt/lists/*

# Create symbolic links so unversioned names work too
RUN for tool in clang clang++ llc llvm-ar llvm-objcopy llvm-objdump; do \
      ln -sfn /usr/bin/${tool}-18 /usr/bin/${tool}; \
    done && \
    ln -sfn /usr/bin/lld-18 /usr/bin/ld.lld

# Install additional build tools
RUN apt-get update -y && \
    apt-get install -y \
      build-essential \
      nasm=2.15.05-1 \
      make=4.3-4.1 \
      pkg-config \
      && \
    rm -rf /var/lib/apt/lists/*

# Install Rust (for userspace utilities)
# Using rustup to get exact version 1.75.0
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- \
      --default-toolchain 1.75.0 \
      --default-host x86_64-unknown-linux-gnu \
      -y && \
    . $HOME/.cargo/env && \
    rustup component add rustfmt clippy

# Install QEMU and OVMF (for testing)
RUN apt-get update -y && \
    apt-get install -y \
      qemu-system-x86=1:7.2+dfsg-5+deb12u5~ubuntu.22.04.1 \
      ovmf=2022.02-3ulfg1+1~ubuntu0.22.04.1 && \
    rm -rf /var/lib/apt/lists/*

# Install GDB for debugging (optional but recommended)
RUN apt-get update -y && \
    apt-get install -y \
      gdb=12.1-0ubuntu1~22.04.2 && \
    rm -rf /var/lib/apt/lists/*

# Create workspace directory
WORKDIR /workspace

# Verify toolchain versions on startup
RUN echo "=== Toolchain Version Report ===" && \
    echo "Clang:" && clang-18 --version && \
    echo "\nLLD:" && ld.lld-18 --version && \
    echo "\nNASM:" && nasm -version && \
    echo "\nRust:" && rustc --version && \
    echo "\nQEMU:" && qemu-system-x86_64 --version && \
    echo "\n=== End Report ==="

# Default command: print version info
CMD ["/bin/bash"]
