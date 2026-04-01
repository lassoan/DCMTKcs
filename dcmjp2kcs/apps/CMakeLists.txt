include_directories("${dcmjp2kcs_SOURCE_DIR}/include" "${ofstd_SOURCE_DIR}/include" "${oflog_SOURCE_DIR}/include" "${dcmdata_SOURCE_DIR}/include" "${dcmimgle_SOURCE_DIR}/include" "${dcmimage_SOURCE_DIR}/include" ${ZLIB_INCDIR})

foreach(PROGRAM dcmcjp2kcs dcmdjp2kcs)
  DCMTK_ADD_EXECUTABLE(${PROGRAM} ${PROGRAM}.cc)
endforeach()

foreach(PROGRAM dcmcjp2kcs dcmdjp2kcs)
  DCMTK_TARGET_LINK_MODULES(${PROGRAM} dcmjp2kcs dcmimage dcmimgle dcmdata oflog ofstd)
endforeach()
