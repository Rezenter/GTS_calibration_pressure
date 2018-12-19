cmake_minimum_required(VERSION 3.5)
project(U1208FSP)

set(CMAKE_CXX_STANDARD 11)

if(NOT EXISTS ../installed.flag)#if this code will be moved to _custom_comand
    set(requirements "libudev-dev" "libusb-1.0-0-dev" "libfox-1.6-dev" "autotools-dev" "autoconf" "automake" "libtool")
    message("Required packages: ${requirements} \n")
    message(STATUS "installing requirerments: \n")
    set(count 1)
    foreach(PACKAGE ${requirements})
        list(LENGTH requirements curr)
        message(STATUS "installing ${count}/${curr} ${PACKAGE}:")
            execute_process(COMMAND apt-get install ${PACKAGE})
        message(STATUS "${PACKAGE} installed" \n)
        MATH(EXPR count "${count}+1")
    endforeach()
    if(NOT EXISTS hidapi/)
        message(STATUS "installing hidapi:")
        execute_process(COMMAND git clone git://github.com/signal11/hidapi.git)
        message(STATUS "hidapi installed" \n)
    endif()
    message(STATUS "configuring hidapi:")
    execute_process(COMMAND ./bootstrap WORKING_DIRECTORY hidapi/)
    execute_process(COMMAND ./configure WORKING_DIRECTORY hidapi/)
    execute_process(COMMAND make WORKING_DIRECTORY hidapi/)
    execute_process(COMMAND make install WORKING_DIRECTORY hidapi/)
    message(STATUS "hidapi configured")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mcc-libusb/61-mcc.rules DESTINATION /etc/udev/rules.d/)
    execute_process(COMMAND udevadm control --reload-rules)
    execute_process(COMMAND udevadm trigger)
    message(STATUS "rules updated")
    message(STATUS "building mcc-libusb")
    if(NOT EXISTS ../mcc-libusb/)
        message(FATAL_ERROR "mcc-libusb not found!")
    endif()
    execute_process(COMMAND make WORKING_DIRECTORY ../mcc-libusb/)
    execute_process(COMMAND make install WORKING_DIRECTORY ../mcc-libusb/)
    execute_process(COMMAND ldconfig WORKING_DIRECTORY ../mcc-libusb/)
    message(STATUS "mcc-libusb builded")
    message(STATUS "requirerments installed")
    file(WRITE installed.flag "true")
else()
    message(STATUS "requirerments already installed")
endif()

include_directories( /usr/include/labbot
        /usr/include/jsoncpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mcc-libusb/
        /usr/local/lib
        /usr/lib)

set(CURRENT_SOURCE_FILES src/u1208fsp.cpp src/u1208fsp.h)
add_library(u1208fsp SHARED ${CURRENT_SOURCE_FILES})
target_link_libraries(u1208fsp usb-1.0)
target_link_libraries(u1208fsp mccusb)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libu1208fsp.so
        DESTINATION /usr/share/labbot/modules/)
        
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html/devices
        DESTINATION /usr/share/labbot/nginx/html/)
