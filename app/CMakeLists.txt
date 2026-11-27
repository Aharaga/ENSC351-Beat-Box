# Build the app, using the HAL

find_package(ALSA REQUIRED)

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(MS2 ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(MS2 LINK_PRIVATE hal)

# 3. Include directories so headers are found
target_include_directories(MS2 PRIVATE ${CMAKE_SOURCE_DIR})

# 4. Link the ALSA library (asound) to your executable
target_link_libraries(MS2 LINK_PRIVATE asound)
target_link_libraries(MS2 LINK_PRIVATE gpiod)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET MS2 POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:MS2>"
     "~/ensc351/public/myApps/MS2"
   COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_SOURCE_DIR}/tracks"
      "~/ensc351/public/myApps/tracks"
  COMMENT "Copying ARM executable to public NFS directory")
