# CMakeList.txt : CMake project for DtlsServerCpp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("DtlsServerCpp")

set(MBEDTLS_DIR "D:/Software/vcpkg/installed/x64-windows-static-md-v143")

# Add source to this project's executable.
add_executable (DtlsServerCpp "DtlsServerCpp.cpp" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DtlsServerCpp PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(DtlsServerCpp PUBLIC "${MBEDTLS_DIR}/include/")
target_link_libraries(DtlsServerCpp PUBLIC "${MBEDTLS_DIR}/lib/mbedcrypto.lib")
target_link_libraries(DtlsServerCpp PUBLIC "${MBEDTLS_DIR}/lib/mbedtls.lib")
target_link_libraries(DtlsServerCpp PUBLIC "${MBEDTLS_DIR}/lib/mbedx509.lib")
