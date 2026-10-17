cmake_minimum_required(VERSION 3.20)
project(libcco C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Compiler flags and definitions
if(MSVC)
    add_compile_options(/W4 /WX /std:c11 /utf-8)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
    )
    # Align release optimization with benchmarks/README.md baseline (-O2)
    if(CMAKE_C_FLAGS_RELEASE MATCHES "-O3")
        string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "Release C flags" FORCE)
    endif()
endif()

# Feature flags
option(BUILD_SHARED_LIBS "Default target cco resolves to shared rather than static" OFF)
option(CCO_ENABLE_FORMAT "Enable $format builtin" OFF)
option(CCO_ENABLE_EVAL "Enable expression evaluation" OFF)
option(CCO_ENABLE_COMMENT_PRESERVE "Preserve comments" OFF)
option(CCO_ENABLE_ERROR_RECOVERY "Enable error recovery" OFF)
option(CCO_ENABLE_ENV "Enable $env builtin" OFF)
option(CCO_ENABLE_STATIC_CALLS "Enable static calls" ON)
option(CCO_ENABLE_CONSTRUCTORS "Enable constructors" ON)

# Source files
set(SOURCES
    src/internal/arena.c
    src/internal/strbuf.c
    src/internal/hash.c
    src/internal/diag.c
    src/internal/lexer.c
    src/internal/object.c
    src/internal/parser.c
    src/internal/symtab.c
    src/internal/inst.c
    src/internal/serializer.c
    src/internal/eval.c
    src/internal/call.c
    src/internal/sandbox.c
    src/internal/scope.c
    src/internal/expr_parser.c
    src/internal/cco.c
)

# ---------------------------------------------------------------------------
# Targets: Static Library (cco_static)
# ---------------------------------------------------------------------------
add_library(cco_static STATIC ${SOURCES})

target_include_directories(cco_static
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
)

# Consumers of static library must define CCO_STATIC
target_compile_definitions(cco_static PUBLIC CCO_STATIC)

if(MSVC)
    # Disambiguate static library on MSVC to avoid collision with shared import cco.lib
    set_target_properties(cco_static PROPERTIES OUTPUT_NAME "cco_static")
else()
    set_target_properties(cco_static PROPERTIES OUTPUT_NAME "cco")
    target_link_libraries(cco_static PRIVATE m)
endif()

# ---------------------------------------------------------------------------
# Targets: Shared Library (cco_shared)
# ---------------------------------------------------------------------------
add_library(cco_shared SHARED ${SOURCES})

target_include_directories(cco_shared
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
)

set_target_properties(cco_shared PROPERTIES
    OUTPUT_NAME "cco"
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    DEFINE_SYMBOL "CCO_EXPORTS"
)

if(NOT MSVC)
    target_link_libraries(cco_shared PRIVATE m)
endif()

# ---------------------------------------------------------------------------
# Aliases
# ---------------------------------------------------------------------------
if(BUILD_SHARED_LIBS)
    add_library(cco ALIAS cco_shared)
    add_library(cnt::cco ALIAS cco_shared)
else()
    add_library(cco ALIAS cco_static)
    add_library(cnt::cco ALIAS cco_static)
endif()

add_library(cnt::cco_static ALIAS cco_static)
add_library(cnt::cco_shared ALIAS cco_shared)
add_library(libcco ALIAS cco_static)

# ---------------------------------------------------------------------------
# Installation & Packaging
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS cco_static cco_shared
    EXPORT ccoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/cnt
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(EXPORT ccoTargets
    FILE ccoTargets.cmake
    NAMESPACE cnt::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cco
)

# Minimal config file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ccoConfig.cmake
"include(\"\${CMAKE_CURRENT_LIST_DIR}/ccoTargets.cmake\")\n\
if(NOT TARGET cnt::cco)\n\
    if(TARGET cnt::cco_shared)\n\
        add_library(cnt::cco ALIAS cnt::cco_shared)\n\
    else()\n\
        add_library(cnt::cco ALIAS cnt::cco_static)\n\
    endif()\n\
endif()\n"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ccoConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cco
)

# Add custom target for linting
add_custom_target(lint
    COMMAND clang-format --dry-run -Werror include/cnt/*.h src/internal/*.c src/internal/*.h test/*.c 2>/dev/null || true
    COMMENT "Running clang-format"
)

# Benchmarks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
endif()