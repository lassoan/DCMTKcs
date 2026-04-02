cmake_minimum_required(VERSION 3.15)


project(DCMTKcs)

# Enable testing
include(CTest)
enable_testing()

# Option to build OpenJPEG as static library (enabled by default)
option(DCMTKCS_OPENJPEG_STATIC "Build OpenJPEG as a static library" ON)
set(OPENJPEG_BUILD_SHARED_LIBS OFF)
if(NOT DCMTKCS_OPENJPEG_STATIC)
    set(OPENJPEG_BUILD_SHARED_LIBS ON)
endif()

include(ExternalProject)

# --- OpenJPEG ExternalProject ---
set(OPENJPEG_GIT_REPOSITORY "https://github.com/uclouvain/openjpeg.git")
set(OPENJPEG_GIT_TAG "v2.5.3")
set(OPENJPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/OpenJPEG-install)



# Add /FS and /EHsc to MSVC flags to avoid PDB write conflicts and enable exception handling
if(MSVC)
    set(DCMTKCS_MSVC_FLAGS "/FS /EHsc")
else()
    set(DCMTKCS_MSVC_FLAGS "")
endif()

ExternalProject_Add(OpenJPEG
    GIT_REPOSITORY ${OPENJPEG_GIT_REPOSITORY}
    GIT_TAG ${OPENJPEG_GIT_TAG}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/OpenJPEG-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/OpenJPEG-build
    INSTALL_DIR ${OPENJPEG_INSTALL_DIR}
    CMAKE_CACHE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=${OPENJPEG_BUILD_SHARED_LIBS}
        -DBUILD_TESTING:BOOL=OFF
        -DBUILD_CODEC:BOOL=OFF
        -DCMAKE_INSTALL_LIBDIR:STRING=lib
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_CXX_FLAGS:STRING=${DCMTKCS_MSVC_FLAGS}
        -DCMAKE_C_FLAGS:STRING=${DCMTKCS_MSVC_FLAGS}
    UPDATE_COMMAND ""
    BUILD_ALWAYS 1
)

# Path to dcmjp2kcs external module
get_filename_component(DCMJP2KCS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/dcmjp2kcs" ABSOLUTE)

# --- DCMTK ExternalProject ---
set(DCMTK_GIT_REPOSITORY "https://github.com/lassoan/DCMTK.git")
set(DCMTK_GIT_TAG "improve-external-module-support")

ExternalProject_Add(
    DCMTK
    GIT_REPOSITORY ${DCMTK_GIT_REPOSITORY}
    GIT_TAG ${DCMTK_GIT_TAG}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/DCMTK-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/DCMTK-build
    CMAKE_CACHE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/dcmtk-install
        -DDCMTK_EXTERNAL_MODULES:STRING=${DCMJP2KCS_SOURCE_DIR}
        -DDCMTK_WITH_OPENJPEG:BOOL=ON
        -DOpenJPEG_DIR:PATH=${OPENJPEG_INSTALL_DIR}/lib/cmake/openjpeg-2.5
        -DDCMTK_USE_FIND_PACKAGE:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=ON
        -DDCMTK_WITH_DOXYGEN:BOOL=OFF
        -DDCMTK_WITH_ZLIB:BOOL=OFF
        -DDCMTK_WITH_PNG:BOOL=OFF
        -DDCMTK_WITH_TIFF:BOOL=OFF
        -DDCMTK_WITH_XML:BOOL=OFF
        -DDCMTK_WITH_ICONV:BOOL=OFF
        -DDCMTK_WITH_SNDFILE:BOOL=OFF
        -DDCMTK_OVERWRITE_WIN32_COMPILER_FLAGS:BOOL=OFF
        -DDCMTK_DEFAULT_DICT:STRING=builtin
        -DDCMTK_ENABLE_PRIVATE_TAGS:BOOL=ON
        -DCMAKE_CXX_FLAGS:STRING=${DCMTKCS_MSVC_FLAGS}
        -DCMAKE_C_FLAGS:STRING=${DCMTKCS_MSVC_FLAGS}
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    BUILD_ALWAYS 1
    DEPENDS OpenJPEG
)


# Add tests for each *_tests.exe in the DCMTK-build/bin/Release directory
set(DCMTK_TEST_EXECUTABLES
    dcmdata_tests.exe
    dcmect_tests.exe
    dcmfg_tests.exe
    dcmiod_tests.exe
    dcmjp2kcs_tests.exe
    dcmnet_tests.exe
    dcmrt_tests.exe
    dcmseg_tests.exe
    dcmsr_tests.exe
    dcmtls_tests.exe
    dcmtract_tests.exe
    oficonv_tests.exe
    ofstd_tests.exe
)

foreach(test_exec ${DCMTK_TEST_EXECUTABLES})
    add_test(
        NAME ${test_exec}
        COMMAND ${CMAKE_BINARY_DIR}/DCMTK-build/bin/Release/${test_exec}
    )
endforeach()
