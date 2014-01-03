project(md5cracker)
find_package(Boost COMPONENTS thread system)
add_executable(cracker cracker.cpp md5.cpp)
target_link_libraries(cracker ${Boost_LIBRARIES})