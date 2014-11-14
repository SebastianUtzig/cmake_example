#include "utils.hpp"

#include <GLFW/glfw3.h>
#include "scene.h"

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

///////////////////////////////////////////////////////////////////////////////////////
//helpers from util.h out of ogldev tutorial 38
//utility function to count elements in array
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define SNPRINTF snprintf
#define MAX_BONES 1000

bool hasAnimations = false;
void updateBoneTransforms();
///////////////////////////////////////////////////////////////////////////////////////

//std::vector<struct MyMesh> myMeshes;

// Vertex Attribute Locations
GLuint vertexLoc=0, normalLoc=1, texCoordLoc=2;

//handles for shader
GLuint program = 0;
GLuint modelMatrixUniformLocation = 0;
GLuint viewMatrixUniformLocation = 0;
GLuint projMatrixUniformLocation = 0;

glm::mat4 modelMatrix;

GLFWwindow* window;

Scene scene;

const std::string vertShaderPath = "../shaders/vertexShader.vs";
const std::string fragShaderPath = "../shaders/fragmentShader.fs";

GLuint m_boneLocation[MAX_BONES];
//forward declaration
//void genVAOsAndUniformBuffer(const aiScene*);
bool setUpShader();
bool setUpWindow();
void render();

int main(int argc, char *argv[])
{
    std::string fileName = "boblampclean.md5mesh";
    if (argc == 2)
    {
        fileName = argv[1];
    }

    if(!setUpWindow())
    {
        return -1;
    }

    if(!setUpShader())
    {
        return -1;
    }
    //scene = Scene();
    if (!scene.LoadMesh(fileName)) {
        printf("Mesh load failed\n");
        return -1;            
    }
    //Now we can access the file's contents.
   //  std::cout << "Import of scene " << pFile.c_str() << " succeeded." << std::endl;
   //  std::cout << " contains " << scene->mNumMeshes << " meshes" << std::endl;
   // std::cout << " contains " << scene->mNumAnimations << " animations" << std::endl;
    for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_boneLocation) ; i++) {
        char Name[128];
        memset(Name, 0, sizeof(Name));
        SNPRINTF(Name, sizeof(Name), "gBones[%d]", i);
        m_boneLocation[i] = glGetUniformLocation(program,Name);
    }


    //genVAOsAndUniformBuffer(scene);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        render();
    }

    glfwTerminate();

    return 0;
    //bulshit
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  
    glUseProgram(program);
    
    float time = static_cast<float>(glfwGetTime());
    
    glm::mat4 newModelMatrix = glm::rotate(modelMatrix, time*0.3f, glm::vec3(0.0f, 1.0f, 0.0f) );
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(newModelMatrix) );

    updateBoneTransforms();

    scene.Render();

    glUseProgram(0);
    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
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
    
    window = glfwCreateWindow(1024, 800, "Animation with Assimp", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return false;
    }
    
    /* Make the window's context current */ 
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    std::cout << glGetError() << ", glew initeted - but createted GL_INVALID_ENUM, it works not" << std::endl;

    glClearColor(0.4,0.4,0.4,1.0);
    glEnable(GL_DEPTH_TEST);

    return true;
}

bool setUpShader()
{
    std::ifstream inFile;
    inFile.open(vertShaderPath);
    if(inFile.fail())
    {
        std::cout << "Couldn't open file: " << vertShaderPath << std::endl;
        return false;
    }
    std::stringstream vShaderStream;
    vShaderStream << inFile.rdbuf();//read the file
    inFile.close();
    const std::string strVertexShader = vShaderStream.str();//str holds the content of the file

    inFile.open(fragShaderPath);
    if(inFile.fail())
    {
        std::cout << "Couldn't open file: " << fragShaderPath << std::endl;
        return false;
    }
    std::stringstream fShaderStream;
    fShaderStream << inFile.rdbuf();//read the file
    inFile.close();
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
    //modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(0.4f) );
    //modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f) );

    glm::mat4 cameraMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 30.0, 150.0));
    glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
    glm::mat4 projMatrix = glm::perspectiveFov(glm::radians(60.0f), 1024.0f, 800.0f, 1.0f, 500.0f);

    // upload Uniform matrices
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix) );
    glUniformMatrix4fv(viewMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(viewMatrix) );
    glUniformMatrix4fv(projMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(projMatrix) );   

    return true;
}

/*void genVAOsAndUniformBuffer(const aiScene *sc) {
 
    struct MyMesh aMesh;
    GLuint buffer;

    // For each mesh
    for (unsigned int n = 0; n < sc->mNumMeshes; ++n)
    {
        const aiMesh* mesh = sc->mMeshes[n];

        std::cout << "mesh " << mesh->mName.data << " has " << mesh->mNumBones << " bones" << std::endl;

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
}*/

//animation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void updateBoneTransforms()
{
    vector<aiMatrix4x4> Transforms;

    scene.BoneTransform(static_cast<float>(glfwGetTime()), Transforms);

    for (uint i = 0 ; i < Transforms.size() ; i++) {
        assert(i < MAX_BONES);
        float currentTranform[4][4] = 
        {
            {Transforms[i].a1, Transforms[i].a2, Transforms[i].a3, Transforms[i].a4},
            {Transforms[i].b1, Transforms[i].b2, Transforms[i].b3, Transforms[i].b4},
            {Transforms[i].c1, Transforms[i].c2, Transforms[i].c3, Transforms[i].c4},
            {Transforms[i].d1, Transforms[i].d2, Transforms[i].d3, Transforms[i].d4},
        };
        //needs pointer to 4x4 array as last parameter
        glUniformMatrix4fv(m_boneLocation[i], 1, GL_TRUE, (const GLfloat*)currentTranform);
    }
}