# -*- coding: utf-8 -*-

add_executable(mujinplcexample main.cpp)
set_target_properties(mujinplcexample PROPERTIES COMPILE_FLAGS "${libzmq_CFLAGS_OTHER}" LINK_FLAGS "${libzmq_LDFLAGS_OTHER}")
target_link_libraries(mujinplcexample PUBLIC mujinplc ${libzmq_LIBRARIES})
install(TARGETS mujinplcexample DESTINATION bin)
