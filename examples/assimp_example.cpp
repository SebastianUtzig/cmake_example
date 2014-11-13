#include "utils.hpp"

#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define GLM_FORCE_RADIANS
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <stdexcept>


// Information to render each assimp node
struct MyMesh{

    GLuint vao;
    int numFaces;
};

// This is for a shader uniform block
struct MyMaterial{

    float diffuse[4];
    float ambient[4];
    float specular[4];
    float emissive[4];
    float shininess;
    int texCount;
};

std::vector<struct MyMesh> myMeshes;

// Vertex Attribute Locations
GLuint vertexLoc=0, normalLoc=1, texCoordLoc=2;


//forward declaration
void genVAOsAndUniformBuffer(const aiScene*);

const std::string strVertexShader(
    "#version 330\n"
    /*"layout(location = 0) in vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}\n"*/

    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec3 normal;\n"
 
    "uniform mat4 projMatrix;\n"
    "uniform mat4 viewMatrix;\n"
    "uniform mat4 modelMatrix;\n"
     
    // "in vec2 texCoord;\n"
     
    "out vec4 vertexPos;\n"
    /*"out vec2 TexCoord;\n"*/
    "out vec3 Normal;\n"
     
    "void main()\n"
    "{\n"
    "    Normal = normalize(vec3( (viewMatrix * modelMatrix) * vec4(normal,0.0)));\n"
    /*"    TexCoord = vec2(texCoord);\n"*/
    "    gl_Position = (projMatrix * viewMatrix * modelMatrix) * vec4(position,1.0);\n"
    "}\n"
);

const std::string strFragmentShader(
    "#version 330\n"
    "layout(location = 0) out vec4 outputColor;\n"
    "in vec3 Normal;\n"
    "void main()\n"
    "{\n"
    "   outputColor = vec4(abs(Normal.x),abs(Normal.y),abs(Normal.z), 1.0f);\n"
    // "   outputColor = vec4(0.7, 0.0f, 0.0f, 1.0f);\n"
    "}\n"

);

int main(int argc, char *argv[])
{
    Assimp::Importer importer;
    std::string pFile = "boblampclean.md5mesh" ;
    //check if file exists
    std::ifstream fin(pFile.c_str());
    if(!fin.fail()) {
        fin.close();
    }
    else{
        std::cout << "Couldn't open file: " << pFile.c_str() << std::endl;
        std::cout << importer.GetErrorString();
    }


    // the global Assimp scene object
    const aiScene* scene = importer.ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);
 
    // If the import failed, report it
    if( !scene)
    {
        std::cout << importer.GetErrorString()  << std::endl;
    }

    // Now we can access the file's contents.
    std::cout << "Import of scene " << pFile.c_str() << " succeeded." << std::endl;

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(1024, 800, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    std::cout << "Import of scene " << pFile.c_str() << " succeeded." << std::endl;
    
    /* Make the window's context current */ 
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();

    glClearColor(0.4,0.4,0.4,1.0);
    glEnable(GL_DEPTH_TEST);

    GLuint program = 0;
    try {
        program = createProgram(strVertexShader, strFragmentShader);
    } catch (std::logic_error& e) {
        std::cerr << e.what() << std::endl;
    }

    // get uniform locations
    glUseProgram(program);
    auto modelMatrixUniformLocation = glGetUniformLocation(program, "modelMatrix");
    auto viewMatrixUniformLocation  = glGetUniformLocation(program, "viewMatrix");
    auto projMatrixUniformLocation  = glGetUniformLocation(program, "projMatrix");


    // generating view / projection / model  matrix
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 20.0, -30));
    modelMatrix = modelMatrix * glm::rotate(glm::mat4(1.0), 90.0f, glm::vec3(1.0f, 0.0f, 0.0f) );
    modelMatrix = modelMatrix * glm::scale(glm::mat4(1.0), glm::vec3(0.4f) );

    glm::mat4 viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 0.0)));
    glm::mat4 projMatrix = glm::perspective(30.0f, 1024.0f/800.0f, 1.0f, 100.0f);

    // upload Uniform matrices
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix) );
    glUniformMatrix4fv(viewMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(viewMatrix) );
    glUniformMatrix4fv(projMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(projMatrix) );
 

    genVAOsAndUniformBuffer(scene);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      
        glUseProgram(program);
        
        float time = static_cast<float>(glfwGetTime());
        
        glm::mat4 newModelMatrix = modelMatrix * glm::rotate(glm::mat4(1.0), time*0.3f, glm::vec3(0.0f, 0.0f, 1.0f) );
        glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(newModelMatrix) );


        for (auto mesh: myMeshes)
        {
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.numFaces*3,GL_UNSIGNED_INT,0);
        }

        glUseProgram(0);
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}



void genVAOsAndUniformBuffer(const aiScene *sc) {
 
    struct MyMesh aMesh;
    GLuint buffer;

    // For each mesh
    for (unsigned int n = 0; n < sc->mNumMeshes; ++n)
    {
        const aiMesh* mesh = sc->mMeshes[n];
 
        // create array with faces
        // have to convert from Assimp format to array
        unsigned int *faceArray;
        faceArray = (unsigned int *)malloc(sizeof(unsigned int) * mesh->mNumFaces * 3);
        unsigned int faceIndex = 0;
 
        for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
            const aiFace* face = &mesh->mFaces[t];
            
            memcpy(&faceArray[faceIndex], face->mIndices,3 * sizeof(unsigned int));
            faceIndex += 3;
        }
        aMesh.numFaces = sc->mMeshes[n]->mNumFaces;
 
        // generate Vertex Array for mesh
        glGenVertexArrays(1,&(aMesh.vao));
        glBindVertexArray(aMesh.vao);
 
        // buffer for faces
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->mNumFaces * 3, &faceArray[0], GL_STATIC_DRAW);
 
        // buffer for vertex positions
        if (mesh->HasPositions()) {
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(vertexLoc);
            glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, 0, 0, 0);
        }
 
        // buffer for vertex normals
        if (mesh->HasNormals()) {
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
            glEnableVertexAttribArray(normalLoc);
            glVertexAttribPointer(normalLoc, 3, GL_FLOAT, 0, 0, 0);
        }
 
        // unbind buffers
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
 
        myMeshes.push_back(aMesh);
    }
}
