add_executable (Tutorial tutorial.cpp)

# add the executable
target_link_libraries (Tutorial  ${EXTRA_LIBS})
add_dependencies(Tutorial MathFunctions)




add_executable (glfw_example glfw_example.cpp utils.cpp)
target_link_libraries (glfw_example glfw GLEW ${EXTRA_LIBS} ${GLFW_LIBRARIES} assimp)
add_dependencies(glfw_example glfw ${GLFW_LIBRARIES})


add_executable (assimp_example assimp_example.cpp utils.cpp)
target_link_libraries (assimp_example glfw GLEW ${EXTRA_LIBS} ${GLFW_LIBRARIES} assimp)
add_dependencies(assimp_example glfw ${GLFW_LIBRARIES})