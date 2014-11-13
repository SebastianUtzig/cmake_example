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
#include <map>

//helpers from util.h out of ogldev tutorial 38
//utility function to count elements in array
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define SNPRINTF snprintf

//animation
#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

struct BoneInfo
{
    aiMatrix4x4 BoneOffset;
    aiMatrix4x4 FinalTransformation;        

    BoneInfo()
    {
        //???
        //BoneOffset.SetZero();
        //FinalTransformation.SetZero();            
    }
};

struct MeshEntry {
        MeshEntry()
        {
            NumIndices    = 0;
            BaseVertex    = 0;
            BaseIndex     = 0;
        }
        
        unsigned int NumIndices;
        unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };
    
std::vector<MeshEntry> m_Entries;

const uint NUM_BONES_PER_VEREX = 4;
static const uint MAX_BONES = 100;

GLuint m_boneLocation[MAX_BONES];

std::vector<aiMatrix4x4> Transforms;

std::map<std::string,uint> m_BoneMapping; // maps a bone name to its index
uint m_NumBones;
std::vector<BoneInfo> m_BoneInfo;
aiMatrix4x4 m_GlobalInverseTransform;

bool hasAnimations = false;

enum VB_TYPES {
    INDEX_BUFFER,
    POS_VB,
    NORMAL_VB,
    TEXCOORD_VB,
    BONE_VB,
    NUM_VBs            
};

struct VertexBoneData
{ 
    uint IDs[NUM_BONES_PER_VEREX];
    float Weights[NUM_BONES_PER_VEREX];

    void AddBoneData(uint BoneID, float Weight)
    {
        for (uint i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(IDs) ; i++) {
            if (Weights[i] == 0.0) {
                IDs[i] = BoneID;
                Weights[i] = Weight;
                return;
            } 
        }

        // should never get here - more bones than we have space for
        assert(0);
    }
};


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

const aiScene* scene;


//forward declaration
void genVAOsAndUniformBuffer(const aiScene*);

void LoadBones(uint, const aiMesh*, std::vector<VertexBoneData>&);
aiMatrix4x4 BoneTransform(float, std::vector<aiMatrix4x4>&);
void ReadNodeHeirarchy(float, const aiNode*, const aiMatrix4x4&);
void CalcInterpolatedRotation(aiQuaternion&, float, const aiNodeAnim*);
void CalcInterpolatedPosition(aiVector3D&, float, const aiNodeAnim*);
void CalcInterpolatedScaling(aiVector3D&, float, const aiNodeAnim*);
uint FindPosition(float, const aiNodeAnim*);
uint FindRotation(float, const aiNodeAnim*);
uint FindScaling(float, const aiNodeAnim*);
const aiNodeAnim* FindNodeAnim(const aiAnimation*, const std::string);

const std::string strVertexShader(
    "#version 440\n"
    /*"layout(location = 0) in vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}\n"*/

    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec3 normal;\n"
    
    "layout(location = 3) in ivec4 BoneIDs;\n"
    "layout(location = 4) in vec4 Weights;\n"
 
    "uniform mat4 projMatrix;\n"
    "uniform mat4 viewMatrix;\n"
    "uniform mat4 modelMatrix;\n"

    "uniform mat4 gBones[MAX_BONES];\n"
     
    // "in vec2 texCoord;\n"
     
    "out vec4 vertexPos;\n"
    /*"out vec2 TexCoord;\n"*/
    "out vec3 Normal;\n"
     
    "void main()\n"
    "{\n"
        "mat4 BoneTransform = gBones[BoneIDs[0]] * Weights[0];\n"
        "BoneTransform += gBones[BoneIDs[1]] * Weights[1];\n"
        "BoneTransform += gBones[BoneIDs[2]] * Weights[2];\n"
        "BoneTransform += gBones[BoneIDs[3]] * Weights[3];\n"


    "    Normal = normalize(vec3(viewMatrix * modelMatrix * BoneTransform * vec4(normal,0.0)));\n"
    /*"    TexCoord = vec2(texCoord);\n"*/
    "    gl_Position = projMatrix * viewMatrix * modelMatrix * BoneTransform * vec4(position,1.0);\n"
    "}\n"
);

const std::string strFragmentShader(
    "#version 440\n"
    "layout(location = 0) out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "   outputColor = vec4(0.7, 0.0f, 0.0f, 1.0f);\n"
    "}\n"

);

int main(int argc, char *argv[])
{
    Assimp::Importer importer;
    std::string pFile = "bench.obj" ;
    //check if file exists
    std::ifstream fin(pFile.c_str());
    if(!fin.fail()) {
        fin.close();
    }
    else{
        std::cout << "Couldn't open file: " << pFile.c_str() << std::endl;
        std::cout << importer.GetErrorString();
        return 1;
    }


    // the global Assimp scene object
    //const aiScene* scene = importer.ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);
    scene = importer.ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);
 
    // If the import failed, report it
    if( !scene)
    {
        std::cout << importer.GetErrorString()  << std::endl;
    }

    // Now we can access the file's contents.
    std::cout << "Import of scene " << pFile.c_str() << " succeeded." << std::endl;

    if(ARRAY_SIZE_IN_ELEMENTS(scene->mAnimations[0]) > 0)
    {
        std::cout << ARRAY_SIZE_IN_ELEMENTS(scene->mAnimations[0]) << " animations in scene" << std::endl;
        hasAnimations = true;
    }
    else std::cout << "no animations in scene" << std::endl;

    //animation
    m_GlobalInverseTransform = scene->mRootNode->mTransformation;
    m_GlobalInverseTransform.Inverse();

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(1024, 800, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();

    glClearColor(0.4,0.4,0.4,1.0);
    
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
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 0.0));
    glm::mat4 viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 1.0)));
    glm::mat4 projMatrix = glm::perspective(60.0f, (float)1024/800, 1.0f, 100.0f);

    // upload Uniform matrices
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix) );
    glUniformMatrix4fv(viewMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(viewMatrix) );
    glUniformMatrix4fv(projMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(projMatrix) );
 

    std::cout << "generating vao ... ";

    genVAOsAndUniformBuffer(scene);

    std::cout << "done" << std::endl;

    std::cout << "Number of meshes " << myMeshes.size() << std::endl;
    for (auto mesh: myMeshes)
    {
        std::cout << "Number of faces " << mesh.numFaces << std::endl;
    }


    //bone locations
    for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_boneLocation) ; i++) {
        char Name[128];
        memset(Name, 0, sizeof(Name));
        SNPRINTF(Name, sizeof(Name), "gBones[%d]", i);
        m_boneLocation[i] = glGetUniformLocation(program,Name);
    }

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      
        glUseProgram(program);
        
        for (auto mesh: myMeshes)
        {
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.numFaces*3,GL_UNSIGNED_INT,0);
        }

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

    return 0;
}

void updateBoneTransforms()
{
    BoneTransform(static_cast<float>(glfwGetTime()), Transforms);

    for (uint i = 0 ; i < Transforms.size() ; i++) {
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

        if(mesh->HasBones()){

            std::vector<VertexBoneData> Bones;

            Bones.resize(mesh->mNumVertices);

            LoadBones(n,mesh,Bones);//???

            glGenBuffers(1, &buffer);//???

            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(BONE_ID_LOCATION);
            glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
            glEnableVertexAttribArray(BONE_WEIGHT_LOCATION); 
            glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);    
        }
 
        // unbind buffers
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
 
        myMeshes.push_back(aMesh);
    }
}

//animation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LoadBones(uint MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones)
{
    for (uint i = 0 ; i < pMesh->mNumBones ; i++) { 
        uint BoneIndex = 0; 
        std::string BoneName(pMesh->mBones[i]->mName.data);

        if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {
            BoneIndex = m_NumBones;
            m_NumBones++; 
            BoneInfo bi;    
            m_BoneInfo.push_back(bi);
        }
        else {
            BoneIndex = m_BoneMapping[BoneName];
        }

        m_BoneMapping[BoneName] = BoneIndex;
        m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

        for (uint j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) {
            uint VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
            float Weight = pMesh->mBones[i]->mWeights[j].mWeight; 
            Bones[VertexID].AddBoneData(BoneIndex, Weight);
        }
    } 
}


aiMatrix4x4 BoneTransform(float TimeInSeconds, std::vector<aiMatrix4x4>& Transforms)
{
    aiMatrix4x4 Identity;
    //Identity.InitIdentity();
    Identity = aiMatrix4x4();

    float TicksPerSecond = scene->mAnimations[0]->mTicksPerSecond != 0 ? 
                            scene->mAnimations[0]->mTicksPerSecond : 25.0f;
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float AnimationTime = fmod(TimeInTicks, scene->mAnimations[0]->mDuration);

    ReadNodeHeirarchy(AnimationTime, scene->mRootNode, Identity);

    Transforms.resize(m_NumBones);

    for (uint i = 0 ; i < m_NumBones ; i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}

void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform)
{ 
    std::string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = scene->mAnimations[0];

    aiMatrix4x4 NodeTransformation(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
        aiMatrix4x4 ScalingM;
        //ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);
        ScalingM.Scaling(Scaling, ScalingM);//???

        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim); 
        aiMatrix4x4 RotationM = aiMatrix4x4(RotationQ.GetMatrix());

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
        aiMatrix4x4 TranslationM;
        //TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);
        TranslationM.Translation(Translation,TranslationM);//???

        // Combine the above transformations
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
        uint BoneIndex = m_BoneMapping[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * 
                                                    m_BoneInfo[BoneIndex].BoneOffset;
    }

    for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
    }
}

void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    uint RotationIndex = FindRotation(AnimationTime, pNodeAnim);
    uint NextRotationIndex = (RotationIndex + 1);
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;
    float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out = Out.Normalize();
}

void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }
            
    uint PositionIndex = FindPosition(AnimationTime, pNodeAnim);
    uint NextPositionIndex = (PositionIndex + 1);
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}

void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
    uint NextScalingIndex = (ScalingIndex + 1);
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}

uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{    
    for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }
    
    assert(0);

    return 0;
}


uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }
    
    assert(0);

    return 0;
}


uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);
    
    for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }
    
    assert(0);

    return 0;
}

const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName)
{
    for (uint i = 0 ; i < pAnimation->mNumChannels ; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
        
        if (std::string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }
    
    return NULL;
}