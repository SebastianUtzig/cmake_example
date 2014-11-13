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
#include <sstream>


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

//handle for shader
GLuint program = 0;
GLuint modelMatrixUniformLocation = 0;
GLuint viewMatrixUniformLocation = 0;
GLuint projMatrixUniformLocation = 0;

glm::mat4 modelMatrix;

GLFWwindow* window;


//forward declaration
void genVAOsAndUniformBuffer(const aiScene*);
void setUpShader();
bool setUpWindow();

int main(int argc, char *argv[])
{
//read model and set up scene
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

    if(!setUpWindow())
    {
        return -1;
    }

    setUpShader();

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

bool setUpWindow()
{
    window;

    /* Initialize the library */
    if (!glfwInit())
        return false;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(1024, 800, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return false;
    }
    
    /* Make the window's context current */ 
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();

    glClearColor(0.4,0.4,0.4,1.0);
    glEnable(GL_DEPTH_TEST);

    return true;
}

void setUpShader()
{
    std::ifstream inFile;
    inFile.open("../shaders/vertexShader.vs");//open the input file
    std::stringstream vShaderStream;
    vShaderStream << inFile.rdbuf();//read the file
    inFile.close();
    const std::string strVertexShader = vShaderStream.str();//str holds the content of the file

    inFile.open("../shaders/fragmentShader.fs");//open the input file
    std::stringstream fShaderStream;
    fShaderStream << inFile.rdbuf();//read the file
    const std::string strFragmentShader = fShaderStream.str();//str holds the content of the file


    try {
        program = createProgram(strVertexShader, strFragmentShader);
    } catch (std::logic_error& e) {
        std::cerr << e.what() << std::endl;
    }

    // get uniform locations
    glUseProgram(program);
    modelMatrixUniformLocation = glGetUniformLocation(program, "modelMatrix");
    viewMatrixUniformLocation  = glGetUniformLocation(program, "viewMatrix");
    projMatrixUniformLocation  = glGetUniformLocation(program, "projMatrix");


    // generating view / projection / model  matrix
    modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 20.0, -30));
    modelMatrix = modelMatrix * glm::rotate(glm::mat4(1.0), 90.0f, glm::vec3(1.0f, 0.0f, 0.0f) );
    modelMatrix = modelMatrix * glm::scale(glm::mat4(1.0), glm::vec3(0.4f) );

    glm::mat4 viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 0.0)));
    glm::mat4 projMatrix = glm::perspective(30.0f, 1024.0f/800.0f, 1.0f, 100.0f);

    // upload Uniform matrices
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix) );
    glUniformMatrix4fv(viewMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(viewMatrix) );
    glUniformMatrix4fv(projMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(projMatrix) );   
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
