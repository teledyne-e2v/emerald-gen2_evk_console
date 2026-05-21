cmake_minimum_required (VERSION 3.10)

# Project name and language
project(Emerald_Gen2-console LANGUAGES CXX)

# Fix header path
add_library(PigentlHeaders_API INTERFACE)
target_include_directories(PigentlHeaders_API INTERFACE
    "C:/Program Files/Teledyne e2v/Evalkit-Emeraldgen2/1.0/pigentl/include" #Adjust this path
)
add_library(PigentlHeaders::API ALIAS PigentlHeaders_API)

# Load SDK
set(pigentl_DIR "C:/Program Files/Teledyne e2v/Evalkit-Emeraldgen2/1.0/pigentl/lib/cmake/pigentl") # Adjust this path
#find_package(PigentlHeaders REQUIRED COMPONENTS API)
find_package(pigentl REQUIRED COMPONENTS sdk)
if(NOT TARGET pigentl::sdk)
    message(FATAL_ERROR "pigentl::sdk not found!")
endif()


# load OpenCV
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib") # Adjust this path
find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc)

# create executable
add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
)

# link libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE pigentl::sdk)

if(OpenCV_FOUND)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE WITH_CV)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
endif()
