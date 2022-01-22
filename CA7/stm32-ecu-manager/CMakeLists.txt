project(stm32-ecu-manager)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(SYSTEM /home/jordan/Documents/2021/stm32-ecu/CA7/linux-5.10.10/build/install_artifact/include)

add_compile_options(-DgPRC_SSL_PROVIDER=package)

add_executable(stm32-ecu-manager main.cpp)

add_subdirectory(grpc)
