#include "utils.hpp"
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
#include <stdexcept>

const std::string strVertexShader(
    "#version 440\n"
    "layout(location = 0) in vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}\n"
);

const std::string strFragmentShader(
    "#version 440\n"
    "layout(location = 0) out vec4 outputColor;\n"
    "uniform float time;"
    "void main()\n"
    "{\n"
    "   outputColor = vec4(0.5+0.5*sin(time), 0.0f, 0.0f, 1.0f);\n"
    "}\n"
);

int main(int argc, char *argv[])
{
    Assimp::Importer importer;
    if (argc == 2) {
        auto const* scene = importer.ReadFile(argv[1],
            aiProcessPreset_TargetRealtime_MaxQuality
        );

        if(!scene){
            std::cerr<<"Unable to load model!"<<std::endl;
            return -1;
        }
    }

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();

    glClearColor(1.0,0.0,0.0,1.0);
    
    GLuint program = 0;
    try {
        program = createProgram(strVertexShader, strFragmentShader);
    } catch (std::logic_error& e) {
        std::cerr << e.what() << std::endl;
    }

    glUseProgram(program);

    auto timeLoc = glGetUniformLocation(program, "time");

    float vertices[] = {-0.5f,-0.5f,0.0f,1.0f,
                        -0.5f,0.5f,0.0f,1.0f,
                        0.5f,0.5f,0.0f,1.0f,
                        -0.5f,-0.5f,0.0f,1.0f,
                        0.5f,0.5f,0.0f,1.0f,
                        0.5f,-0.5f,0.0f,1.0f};

    GLuint vao = 0;
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,6*4* sizeof(float),vertices,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,0,0);
    glBindVertexArray(0);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        float time = static_cast<float>(glfwGetTime());
      
        glUseProgram(program);
        
        glUniform1f(timeLoc, time);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
