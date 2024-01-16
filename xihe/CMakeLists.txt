project(xihe LANGUAGES C CXX)

add_executable (xihe "xihe.cpp" "xihe.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET xihe PROPERTY CXX_STANDARD 20)
endif()

# Link third party libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    volk
)