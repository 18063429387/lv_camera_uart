# Usage:
# cmake -DCMAKE_TOOLCHAIN_FILE=./user_cross_compile_setup.cmake -B build -S .
# cmake --build build -j$(nproc)
#
# Tina/OpenWrt riscv64: do NOT set CMAKE_SYSROOT to staging_dir/target alone —
# crt1.o and libc live in the toolchain; target staging has app libs (freetype2).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(tools /home/xiaozhi/Downloads/f133/mx-hxx003/tina5.0-f133/out/f133-mx-hxx/prototype/openwrt/staging_dir/toolchain)

set(CMAKE_C_COMPILER ${tools}/bin/riscv64-unknown-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/riscv64-unknown-linux-gnu-g++)

# Target staging: headers/libs for pkg-config (freetype2.pc, etc.)
set(SYSROOT "/home/xiaozhi/Downloads/f133/mx-hxx003/tina5.0-f133/out/f133-mx-hxx/prototype/openwrt/staging_dir/target")

# pkg-config only — finds freetype2 in SYSROOT without breaking toolchain link
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig")
set(ENV{STAGING_DIR} "${SYSROOT}")

# Help linker find target .so (e.g. libfreetype) when linking executables
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${SYSROOT}/usr/lib -Wl,-rpath-link,${SYSROOT}/usr/lib")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L${SYSROOT}/usr/lib -Wl,-rpath-link,${SYSROOT}/usr/lib")