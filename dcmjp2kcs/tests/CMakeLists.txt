# declare executables
DCMTK_ADD_TEST_EXECUTABLE(dcmjp2kcs_tests
  tests.cc
  t_codec.cc
)

# make sure executables are linked to the corresponding libraries
DCMTK_TARGET_LINK_MODULES(dcmjp2kcs_tests dcmjp2kcs dcmdata oflog ofstd)

# This macro parses tests.cc and registers all tests
DCMTK_ADD_TESTS(dcmjp2kcs)
