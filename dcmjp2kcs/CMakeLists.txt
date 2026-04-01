# declare project
project(dcmjp2kcs)

# Find required dependencies before importing targets
# OpenJPEG is required for JPEG 2000 support
if(DCMTK_WITH_OPENJPEG)
  if(NOT TARGET openjp2)
    find_package(OpenJPEG QUIET)
    if(NOT OPENJPEG_FOUND AND NOT TARGET openjp2)
      message(WARNING "DCMTK was built with OpenJPEG support but OpenJPEG was not found. "
                      "You may need to set OpenJPEG_DIR to the location of OpenJPEGConfig.cmake")
    endif()
  endif()
endif()

# External modules include directories
list(APPEND DCMTK_INCLUDE_DIR "${DCMTK_SOURCE_DIR}/ExternalModules/dcmjp2kcs/include")

# recurse into subdirectories
foreach(SUBDIR libsrc include apps tests)
  add_subdirectory(${SUBDIR})
endforeach()
