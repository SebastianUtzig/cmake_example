#include "utils.hpp"
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
#include <stdexcept>

const std::string strVertexShader(
    "#version 440\n"
    /*"layout(location = 0) in vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}\n"*/

    "layout (std140) uniform Matrices {\n"
 
        "mat4 projMatrix;\n"
        "mat4 viewMatrix;\n"
        "mat4 modelMatrix;\n"
    "};\n"
     
    "in vec3 position;\n"
    "in vec3 normal;\n"
    "in vec2 texCoord;\n"
     
    "out vec4 vertexPos;\n"
    /*"out vec2 TexCoord;\n"*/
    "out vec3 Normal;\n"
     
    "void main()\n"
    "{\n"
    "    Normal = normalize(vec3(viewMatrix * modelMatrix * vec4(normal,0.0)));\n"
    /*"    TexCoord = vec2(texCoord);\n"*/
    "    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position,1.0);\n"
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

    /*"layout (std140) uniform Material {\n"
    "    vec4 diffuse;\n"
    "    vec4 ambient;\n"
    "    vec4 specular;\n"
    "    vec4 emissive;\n"
    "    float shininess;\n"
    "    int texCount;\n"
    "};\n"
     
    "uniform sampler2D texUnit;\n"
     
    "in vec3 Normal;\n"
    "in vec2 TexCoord;\n"
    "out vec4 output;\n"
     
    "void main()\n"
    "{\n"
    "    vec4 color;\n"
    "    vec4 amb;\n"
    "    float intensity;\n"
    "    vec3 lightDir;\n"
    "    vec3 n;\n"
     
    "    lightDir = normalize(vec3(1.0,1.0,1.0));\n"
    "    n = normalize(Normal);\n"
    "    intensity = max(dot(lightDir,n),0.0);\n"
     
    "    if (texCount == 0) {\n"
    "        color = diffuse;\n"
    "        amb = ambient;\n"
    "    }\n"
    "    else {\n"
    "        color = texture2D(texUnit, TexCoord);\n"
    "        amb = color * 0.33;\n"
    "    }\n"
    "    output = (color * intensity) + amb;\n"
    "}\n"*/
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

        /*float vertices[] = {-0.5f,-0.5f,0.0f,1.0f,
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
        glBindVertexArray(0);*/

        //new:
        genVAOsAndUniformBuffer(scene);



        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            float time = static_cast<float>(glfwGetTime());
          
            glUseProgram(program);
            
            glUniform1f(timeLoc, time);

            /*glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);*/
            glUseProgram(0);
            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }

        glfwTerminate();

    }

    return 0;
}

/*void recursive_render (const aiScene *sc, const aiNode* nd)
{
 
    // Get node transformation matrix
    aiMatrix4x4 m = nd->mTransformation;
    // OpenGL matrices are column major
    m.Transpose();
 
    // save model matrix and apply node transformation
    pushMatrix();
 
    float aux[16];
    memcpy(aux,&m,sizeof(float) * 16);
    multMatrix(modelMatrix, aux);
    setModelMatrix();
 
    // draw all meshes assigned to this node
    for (unsigned int n=0; n < nd->mNumMeshes; ++n){
        // bind material uniform
        glBindBufferRange(GL_UNIFORM_BUFFER, materialUniLoc, myMeshes[nd->mMeshes[n]].uniformBlockIndex, 0, sizeof(struct MyMaterial));  
        // bind texture
        glBindTexture(GL_TEXTURE_2D, myMeshes[nd->mMeshes[n]].texIndex);
        // bind VAO
        glBindVertexArray(myMeshes[nd->mMeshes[n]].vao);
        // draw
        glDrawElements(GL_TRIANGLES,myMeshes[nd->mMeshes[n]].numFaces*3,GL_UNSIGNED_INT,0);
 
    }
 
    // draw all children
    for (unsigned int n=0; n < nd->mNumChildren; ++n){
        recursive_render(sc, nd->mChildren[n]);
    }
    popMatrix();
}*/


void genVAOsAndUniformBuffer(const aiScene *sc) {
 
    struct MyMesh aMesh;
    struct MyMaterial aMat; 
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);
 
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
 
        // buffer for vertex texture coordinates
        if (mesh->HasTextureCoords(0)) {
            float *texCoords = (float *)malloc(sizeof(float)*2*mesh->mNumVertices);
            for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
 
                texCoords[k*2]   = mesh->mTextureCoords[0][k].x;
                texCoords[k*2+1] = mesh->mTextureCoords[0][k].y; 
 
            }
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
            glEnableVertexAttribArray(texCoordLoc);
            glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, 0, 0, 0);
        }
 
        // unbind buffers
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
 
        // create material uniform buffer
        aiMaterial *mtl = sc->mMaterials[mesh->mMaterialIndex];
 
        aiString texPath;   //contains filename of texture
        if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)){
                //bind texture
                unsigned int texId = textureIdMap[texPath.data];
                aMesh.texIndex = texId;
                aMat.texCount = 1;
            }
        else
            aMat.texCount = 0;
 
        float c[4];
        set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
        aiColor4D diffuse;
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
            color4_to_float4(&diffuse, c);
        memcpy(aMat.diffuse, c, sizeof(c));
 
        set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
        aiColor4D ambient;
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
            color4_to_float4(&ambient, c);
        memcpy(aMat.ambient, c, sizeof(c));
 
        set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
        aiColor4D specular;
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
            color4_to_float4(&specular, c);
        memcpy(aMat.specular, c, sizeof(c));
 
        set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
        aiColor4D emission;
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
            color4_to_float4(&emission, c);
        memcpy(aMat.emissive, c, sizeof(c));
 
        float shininess = 0.0;
        unsigned int max;
        aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
        aMat.shininess = shininess;
 
        glGenBuffers(1,&(aMesh.uniformBlockIndex));
        glBindBuffer(GL_UNIFORM_BUFFER,aMesh.uniformBlockIndex);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(aMat), (void *)(&aMat), GL_STATIC_DRAW);
 
        myMeshes.push_back(aMesh);
    }
}
