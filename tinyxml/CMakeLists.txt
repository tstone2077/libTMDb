CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (tinyxml)

SET (CLIENT_BINARY_NAME "tinyxml")

SET ( SOURCES
        "tinystr.cpp"
        "tinyxml.cpp"
        "tinyxmlerror.cpp"
        "tinyxmlparser.cpp"
    )

# set the generated executable path
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")

# add our target
ADD_LIBRARY (${CLIENT_BINARY_NAME} ${SOURCES} ) 

# link
  TARGET_LINK_LIBRARIES (${CLIENT_BINARY_NAME})

