cmake_minimum_required(VERSION 3.10)
project(icm20948 LANGUAGES C CXX VERSION 0.0.1)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Optional features ──────────────────────────────────────────────────────────
option(ICM20948_USE_DMP "Enable ICM20948 DMP firmware support" OFF)

# ── Dependencies ───────────────────────────────────────────────────────────────
find_package(GPIOD REQUIRED)

if (LIBGPIOD_VERSION VERSION_GREATER_EQUAL "2.0")
  set(HAVE_LIBGPIOD_V2 1)
endif()

if (ICM20948_USE_DMP)
  add_compile_definitions(ICM20948_USE_DMP)
endif()

# ── Generated config header ────────────────────────────────────────────────────
configure_file(src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

# ── Library sources ────────────────────────────────────────────────────────────
add_library(icm20948 SHARED
    src/ICM20948.cpp
    src/icm20948.c
    src/icm20948_dmp.c
    src/icm20948_i2c.c
    src/icm20948_spi.c
)

target_include_directories(icm20948 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/icm20948>
)

target_link_libraries(icm20948 PUBLIC ${LIBGPIOD_LIBRARIES})
target_compile_options(icm20948 PRIVATE -Wall -Wextra -pedantic)

set_target_properties(icm20948 PROPERTIES
    PUBLIC_HEADER
        "src/ICM20948.h;\
src/icm20948.h;\
src/icm20948_i2c.h;\
src/icm20948_spi.h;\
src/icm20948_registers.h;\
src/icm20948_enumerations.h;\
src/icm20948_dmp.h;\
src/ak09916_enumerations.h;\
src/ak09916_registers.h"
    VERSION   ${PROJECT_VERSION}
    SOVERSION 0
)

# ── Example executable ─────────────────────────────────────────────────────────
add_executable(icm20948_i2c_test examples/i2c_test.cpp)
target_include_directories(icm20948_i2c_test PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(icm20948_i2c_test icm20948)

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS icm20948
    EXPORT ICM20948Targets
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/icm20948
)

include(CMakePackageConfigHelpers)
configure_package_config_file(ICM20948Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/ICM20948Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ICM20948"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ICM20948ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT ICM20948Targets
    FILE      ICM20948Targets.cmake
    NAMESPACE ICM20948::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ICM20948
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ICM20948Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ICM20948ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ICM20948"
)

# ── pkg-config ─────────────────────────────────────────────────────────────────
configure_file(icm20948.pc.in icm20948.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icm20948.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)
