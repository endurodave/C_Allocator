# Collect all .c files in this subdirectory
file(GLOB SUBDIR_SOURCES "*.c")

# Collect all .h files in this subdirectory
file(GLOB SUBDIR_HEADERS "*.h")

# Create a library target named DelegateLibrary
add_library(ExamplesLib STATIC ${SUBDIR_SOURCES} ${SUBDIR_HEADERS})

# Include directories for the library
target_include_directories(ExamplesLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(ExamplesLib LINK_PUBLIC 
	PortLib
	AllocatorLib
)