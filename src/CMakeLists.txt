# Build openMVG plugin
create_ofx_plugin(hdrTools .)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_property(TARGET hdrTools PROPERTY LINK_FLAGS "-Wl,--no-undefined") 
endif()
