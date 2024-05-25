# CMakeList.txt : CMake project for RF24GatewayController, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.18)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project ("RF24GatewayController")

file(GLOB RF24GatewayControllerSources
	CONFIGURE_DEPENDS 
	"*.h" 
	"*.cpp"
)

add_executable (RF24GatewayController ${RF24GatewayControllerSources})
set_property(TARGET RF24GatewayController PROPERTY CXX_STANDARD 20)

if (UNIX)

    # RF24
    find_library(RF24 rf24 REQUIRED)
    message(STATUS "using RF24 library: ${RF24}")

    find_library(RF24Network rf24network REQUIRED)
    message(STATUS "using RF24Network library: ${RF24Network}")

    find_library(RF24Mesh rf24mesh REQUIRED)
    message(STATUS "using RF24Mesh library: ${RF24Mesh}")

    find_library(RF24Gateway rf24gateway REQUIRED)
    message(STATUS "using RF24Gateway library: ${RF24Gateway}")

    set(rf24_linked_libs 
        ${RF24}
        pthread # Notice we specify pthread as a linked lib here
        ${RF24Network}
        ${RF24Mesh}
        ${RF24Gateway}
    )

    if("${RF24_DRIVER}" STREQUAL "MRAA")
        if(NOT "${LibMRAA}" STREQUAL "LibMRAA-NOTFOUND")
            message(STATUS "linking to ${LibMRAA}")
            list(APPEND rf24_linked_libs ${LibMRAA})
        else()
            message(FATAL "Lib ${RF24_DRIVER} not found.")
        endif()
    elseif("${RF24_DRIVER}" STREQUAL "wiringPi")
        if(NOT "${LibWiringPi}" STREQUAL "LibWiringPi-NOTFOUND")
            message(STATUS "linking to ${LibWiringPi}")
            list(APPEND rf24_linked_libs ${LibWiringPi})
        else()
            message(FATAL "Lib ${RF24_DRIVER} not found.")
        endif()
    elseif(NOT "${LibPIGPIO}" STREQUAL "LibPIGPIO-NOTFOUND" AND NOT DEFINED RF24_NO_INTERRUPT)
        if(NOT "${RF24_DRIVER}" STREQUAL "pigpio")
            message(STATUS "linking to ${LibPIGPIO} for interrupt support")
        else()
            message(STATUS "linking to ${LibPIGPIO}")
        endif()
        list(APPEND rf24_linked_libs ${LibPIGPIO})
    else()
        message(STATUS "Disabling IRQ pin support")
    endif()

    target_link_libraries(RF24GatewayController PUBLIC ${rf24_linked_libs})

    target_compile_definitions(RF24GatewayController PUBLIC "CM_UNIX")
elseif(WIN32)
    target_compile_definitions(RF24GatewayController PUBLIC "CM_WIN32")
endif()

# spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(RF24GatewayController PUBLIC spdlog::spdlog)

# jsoncpp
# find_package(jsoncpp REQUIRED)
# target_link_libraries(RF24GatewayController PUBLIC jsoncpp_lib)

# Boost
set(Boost_USE_STATIC_LIBS ON)
find_package( Boost 1.74 REQUIRED COMPONENTS program_options)
target_include_directories(RF24GatewayController PRIVATE ${Boost_INCLUDE_DIR} )
target_link_libraries(RF24GatewayController PRIVATE ${Boost_LIBRARIES} )



