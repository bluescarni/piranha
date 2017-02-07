find_package(BZip2 REQUIRED)
if(NOT TARGET BZip2::BZip2)
  # Some older CMake versions do not provide the imported target for bzip2.
  message(STATUS "The 'BZip2::BZip2' imported target is missing, creating it.")
  add_library(BZip2::BZip2 UNKNOWN IMPORTED)
  set_target_properties(BZip2::BZip2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BZIP2_INCLUDE_DIRS}")
  set_property(TARGET BZip2::BZip2 APPEND PROPERTY IMPORTED_LOCATION "${BZIP2_LIBRARIES}")
endif()
