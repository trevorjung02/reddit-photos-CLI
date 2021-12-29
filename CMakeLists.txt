cmake_minimum_required (VERSION 3.5)
project (reddit-photomosaics VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(reddit-photomosaics
    main.cpp
)

add_custom_target(copy_scraper
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Scraper ${CMAKE_CURRENT_BINARY_DIR}/Scraper
)
add_dependencies(reddit-photomosaics copy_scraper)
    
target_link_libraries( reddit-photomosaics ${OpenCV_LIBS} )
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT reddit-photomosaics)
