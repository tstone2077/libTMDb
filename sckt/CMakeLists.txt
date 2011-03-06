CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (sckt)

SET (CLIENT_BINARY_NAME "sckt")

SET ( SOURCES
        "sckt.cpp"
    )

# set the generated executable path
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")

# add our target
ADD_LIBRARY (${CLIENT_BINARY_NAME} ${SOURCES} ) 

# link
  TARGET_LINK_LIBRARIES (${CLIENT_BINARY_NAME})

