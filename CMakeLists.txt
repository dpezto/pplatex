#
# Author: Stefan Hepp <stefan@stefant.org>
#
# CMake build scripts for pplatex. 
#
# This is an alternative build script for the SCons based build script.
# It is recommended to build in a separate directory. Assuming this git
# repository has been checked out to 'pplatex', run:
#   mkdir ../build
#   cd ../build
#   cmake ../pplatex
#   make
#
# Requirements on Linux/macOS are:
# - pkg-config
# - libpcre2-dev (Debian/Ubuntu) or pcre2 (Homebrew)
# Requirements on Windows are:
# - MSVC Compiler (should work with Cygwin/MinGW as well)
# - PCRE2
#
# Without pkg-config, set PCRE2_INCLUDE_DIR to the directory containing
# pcre2posix.h and set PCRE2_POSIX_LIBRARY and PCRE2_LIBRARY to the
# pcre2-posix and pcre2-8 libraries respectively. When linking against a
# PCRE2 DLL rather than a static library, also set PCRE2_SHARED=ON.
#

# CMake 4.0 removed compatibility with projects requiring < 3.5, so the old
# 3.0 floor made this project unconfigurable. 3.20 is a conservative modern
# baseline: it ships in Debian 12, Ubuntu 22.04 and RHEL 9.
cmake_minimum_required(VERSION 3.20)

# Main project name. PPLATEX_VERSION is the release string reported by
# --version; release-please rewrites the annotated line on every release.
project(pplatex VERSION 1.0)
set(PPLATEX_VERSION "1.0.0") # x-release-please-version

# Find PCRE2 via its POSIX compatibility layer. pkg-config is the only
# discovery mechanism PCRE2 ships, so fall back to a manual search for
# platforms without it (typically Windows).
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PCRE2 QUIET IMPORTED_TARGET libpcre2-posix)
endif()

if (PCRE2_FOUND)
  set(PCRE2_LINK_LIBRARIES PkgConfig::PCRE2)
else()
  find_path(PCRE2_INCLUDE_DIR pcre2posix.h)
  find_library(PCRE2_POSIX_LIBRARY NAMES pcre2-posix pcre2-posix-static)
  find_library(PCRE2_LIBRARY NAMES pcre2-8 pcre2-8-static)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PCRE2 DEFAULT_MSG
    PCRE2_INCLUDE_DIR PCRE2_POSIX_LIBRARY PCRE2_LIBRARY)

  set(PCRE2_LINK_LIBRARIES ${PCRE2_POSIX_LIBRARY} ${PCRE2_LIBRARY})
  set(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
  mark_as_advanced(PCRE2_INCLUDE_DIR PCRE2_POSIX_LIBRARY PCRE2_LIBRARY)
endif()

# Standard bin/share layout, so packagers can override CMAKE_INSTALL_PREFIX
include(GNUInstallDirs)

# Build pplatex
add_subdirectory(src)

