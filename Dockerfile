FROM archlinux:latest

# Update system
RUN pacman -Syu --noconfirm

# Install build dependencies (similar to CI)
RUN pacman -S --noconfirm \
    base-devel \
    git \
    mingw-w64-gcc \
    mingw-w64-headers \
    mingw-w64-crt \
    mingw-w64-winpthreads \
    mingw-w64-pkg-config \
    mingw-w64-glslang \
    meson \
    ninja \
    python

WORKDIR /dxvk