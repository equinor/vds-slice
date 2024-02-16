find_package(openvds CONFIG REQUIRED)

add_library(cppcore
  attribute.cpp
  axis.cpp
  boundingbox.cpp
  cppapi_data.cpp
  cppapi_metadata.cpp
  datahandle.hpp
  datahandle.cpp
  datasource.cpp
  direction.cpp
  metadatahandle.cpp
  regularsurface.cpp
  subcube.cpp
  subvolume.cpp
  volumedatalayout.cpp
)

target_include_directories(cppcore
  PUBLIC ${CMAKE_SOURCE_DIR}/internal/core
)

target_link_libraries(cppcore
  PUBLIC openvds::openvds
)

find_package(Boost REQUIRED)
target_include_directories(cppcore
  PRIVATE ${Boost_INCLUDE_DIRS}
)
