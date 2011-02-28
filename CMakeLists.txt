PROJECT(tcp_tap)

ADD_EXECUTABLE(tcp_tap main.cpp)

target_link_libraries(tcp_tap pthread)

