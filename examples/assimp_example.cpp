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

    GLuint vao = 0;
    int numFaces = 0;
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

///////////////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////////////

std::vector<struct MyMesh> myMeshes;

// Vertex Attribute Locations
GLuint vertexLoc=0, normalLoc=1, texCoordLoc=2;

//handles for shader
GLuint program = 0;
GLuint modelMatrixUniformLocation = 0;
GLuint viewMatrixUniformLocation = 0;
GLuint projMatrixUniformLocation = 0;

glm::mat4 modelMatrix;

GLFWwindow* window;

const aiScene* scene;

const std::string vertShaderPath = "../shaders/vertexShader.vs";
const std::string fragShaderPath = "../shaders/fragmentShader.fs";


//forward declaration
void genVAOsAndUniformBuffer(const aiScene*);
bool setUpShader();
bool setUpWindow();
void render();

int main(int argc, char *argv[])
{
//read model and set up scene
    Assimp::Importer importer;
    std::string pFile = "boblampclean.md5mesh";
    //check if file exists
    std::ifstream fin(pFile.c_str());
    if(!fin.fail()) {
        fin.close();
    }
    else{
        std::cout << "Couldn't open file: " << pFile.c_str() << std::endl;
        std::cout << importer.GetErrorString();
        return -1;
    }

    // the global Assimp scene object
    scene = importer.ReadFile( pFile, aiProcessPreset_TargetRealtime_Quality);
    // If the import failed, report it
    if(!scene)
    {
        std::cout << importer.GetErrorString()  << std::endl;
        return -1;
    }

    // Now we can access the file's contents.
    std::cout << "Import of scene " << pFile.c_str() << " succeeded." << std::endl;
    std::cout << " contains " << scene->mNumMeshes << " meshes" << std::endl;
    std::cout << " contains " << scene->mNumAnimations << " animations" << std::endl;

    //bone locations
    for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_boneLocation) ; i++) {
        char Name[128];
        memset(Name, 0, sizeof(Name));
        SNPRINTF(Name, sizeof(Name), "gBones[%d]", i);
        m_boneLocation[i] = glGetUniformLocation(program,Name);
    }

    if(!setUpWindow())
    {
        return -1;
    }

    if(!setUpShader())
    {
        return -1;
    }

    genVAOsAndUniformBuffer(scene);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        render();
    }

    glfwTerminate();

    return 0;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  
    glUseProgram(program);
    
    float time = static_cast<float>(glfwGetTime());
    
    glm::mat4 newModelMatrix = glm::rotate(modelMatrix, time*0.3f, glm::vec3(0.0f, 0.0f, 1.0f) );
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(newModelMatrix) );

    updateBoneTransforms();

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
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f) );

    glm::mat4 cameraMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 30.0, 100.0));
    glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
    glm::mat4 projMatrix = glm::perspectiveFov(glm::radians(60.0f), 1024.0f, 800.0f, 1.0f, 200.0f);

    // upload Uniform matrices
    glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix) );
    glUniformMatrix4fv(viewMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(viewMatrix) );
    glUniformMatrix4fv(projMatrixUniformLocation , 1, GL_FALSE, glm::value_ptr(projMatrix) );   

    return true;
}

void genVAOsAndUniformBuffer(const aiScene *sc) {
 
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