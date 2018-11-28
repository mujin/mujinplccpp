# -*- coding: utf-8 -*-

add_library(mujinplc SHARED plcmemory.cpp)
set_target_properties(mujinplc PROPERTIES COMPILE_FLAGS "${libzmq_CFLAGS_OTHER}" LINK_FLAGS "${libzmq_LDFLAGS_OTHER}")
target_link_libraries(mujinplc PUBLIC ${libzmq_LIBRARIES})
install(TARGETS mujinplc DESTINATION lib${LIB_SUFFIX})
