#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cmath>

#include "GLCommon.hpp"
#include "GL/glfw.h"
#include "stb_image/stb_image.h"
#include "imgui.h"
#include "imguiRenderGL.h"

#include "FramebufferGL.hpp"
#include "ShaderGLSL.hpp"
#include "Camera.hpp"
#include "Transform.hpp"
#include "LinearAlgebra.hpp"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif


struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;


bool DoTheImportThing(const std::string& pFile, int *& obj_FacesIndices, float *& obj_Vertices, float *& obj_Normals, float *& obj_UVs, 
                      unsigned int &obj_NumFaces,  unsigned int &obj_NumVertices, int numMesh, float proportion);

void BindingInstanceLike(int *& obj_FacesIndices, float *& obj_Vertices, float *& obj_Normals, float *& obj_UVs, 
                      unsigned int &obj_NumFaces,  unsigned int &obj_NumVertices, GLuint & vao, GLuint * vbo, int startVBO);

bool importSpline(const std::string& pFile, float *& obj_Vertices, unsigned int &obj_NumVertices, int numMesh, float proportion);

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}

int main( int argc, char **argv )
{
 
   bool cameraAuto = true;
   if(argc > 1){
      if(strcmp(argv[1], "-c") == 0){
        if(strcmp(argv[2],"oui") == 0)
            cameraAuto = true;
        else
            cameraAuto = false;
      }
   }

  ///////////////////////////////////// ID TRANSFORMATIONS //////////////////////////////
    float IdTranslation[3] = {0,0,0};
    float IdRotation[16] = {1,0,0,0,
                            0,1,0,0,
                            0,0,1,0,
                            0,0,0,1};

    int width = 1024, height=768;
    double t;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    
#ifdef __APPLE__
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    // Open a window and create its OpenGL context
    if( !glfwOpenWindow( width, height, 0,0,0,0, 24,0, GLFW_WINDOW ) )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );

        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwSetWindowTitle( "td3" );
    
#ifdef __APPLE__
    glewExperimental = GL_TRUE;
#endif
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwEnable( GLFW_STICKY_KEYS );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );

    // Init UI
    if (!imguiRenderGLInit("DroidSans.ttf"))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    // Init viewer structures
    Camera camera;
    GUIStates guiStates;
    init_gui_states(guiStates);

    // Load images and upload textures

    GLuint textures[11];
    glGenTextures(11, textures);
    int x;
    int y;
    int comp; 
    unsigned char * diffuse = stbi_load("textures/ishiza.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    unsigned char * spec = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, spec);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    unsigned char * box = stbi_load("textures/skybox2.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, textures[4]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, box);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char * grass = stbi_load("textures/grass2.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, textures[5]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, grass);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char * specgrass = stbi_load("textures/grass2.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, textures[6]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, specgrass);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char * diffuse2 = stbi_load("textures/gigul.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, textures[7]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE,  diffuse2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char * diffuse3 = stbi_load("textures/jeflum.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, textures[8]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE,  diffuse3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char * transp = stbi_load("textures/trans.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, textures[9]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE,  transp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char * transpec = stbi_load("textures/transSpec.tga", &x, &y, &comp, 1);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, textures[10]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE,  transp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Try to load and compile shader
    int status;
    ShaderGLSL gbuffer_shader;
    status = load_shader_from_file(gbuffer_shader, "td3/3_gbuffer.glsl", ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  td3/3_gbuffer.glsl\n");
        exit( EXIT_FAILURE );
    }
    
    // Compute locations for gbuffer_shader
    GLuint gbuffer_projectionLocation = glGetUniformLocation(gbuffer_shader.program, "Projection");
    GLuint gbuffer_viewLocation = glGetUniformLocation(gbuffer_shader.program, "View");
    GLuint gbuffer_objectLocation = glGetUniformLocation(gbuffer_shader.program, "Object");
    GLuint gbuffer_timeLocation = glGetUniformLocation(gbuffer_shader.program, "Time");
    GLuint gbuffer_diffuseLocation = glGetUniformLocation(gbuffer_shader.program, "Diffuse");
    GLuint gbuffer_specLocation = glGetUniformLocation(gbuffer_shader.program, "Spec");
    GLuint gbuffer_translationLocation = glGetUniformLocation(gbuffer_shader.program, "translation");
    GLuint gbuffer_rotationLocation = glGetUniformLocation(gbuffer_shader.program, "rotation");


    // SKYBOX
    ShaderGLSL skybox_shader;
    status = load_shader_from_file(skybox_shader, "td3/3_skybox.glsl", ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  td3/3_skybox.glsl\n");
        exit( EXIT_FAILURE );
    }
    // Compute locations for skybox_shader
    GLuint skybox_projectionLocation = glGetUniformLocation(skybox_shader.program, "Projection");
    GLuint skybox_viewLocation = glGetUniformLocation(skybox_shader.program, "View");
    GLuint skybox_objectLocation = glGetUniformLocation(skybox_shader.program, "Object");
    GLuint skybox_translationLocation = glGetUniformLocation(skybox_shader.program, "Translation");
    GLuint skybox_boxLocation = glGetUniformLocation(skybox_shader.program, "Box");

   

    // Load light accumulation shader
    ShaderGLSL laccum_shader;
    status = load_shader_from_file(laccum_shader, "td3/3_laccum_spot_corrected.glsl", ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  td3/3_laccum_spot_corrected.glsl\n");
        exit( EXIT_FAILURE );
    }
    // Compute locations for light accumulation shader
    GLuint laccum_projectionLocation = glGetUniformLocation(laccum_shader.program, "Projection");
    GLuint laccum_materialLocation = glGetUniformLocation(laccum_shader.program, "Material");
    GLuint laccum_normalLocation = glGetUniformLocation(laccum_shader.program, "Normal");
    GLuint laccum_depthLocation = glGetUniformLocation(laccum_shader.program, "Depth");
    GLuint laccum_shadowLocation = glGetUniformLocation(laccum_shader.program, "Shadow");
    GLuint laccum_timeLocation = glGetUniformLocation(laccum_shader.program, "Time");

    GLuint laccum_inverseViewProjectionLocation = glGetUniformLocation(laccum_shader.program, "InverseViewProjection");
    GLuint laccum_projectionLightLocation = glGetUniformLocation(laccum_shader.program, "ProjectionLight");
    GLuint laccum_cameraPositionLocation = glGetUniformLocation(laccum_shader.program, "CameraPosition");
    GLuint laccum_lightPositionLocation = glGetUniformLocation(laccum_shader.program, "LightPosition");
    GLuint laccum_lightDirectionLocation = glGetUniformLocation(laccum_shader.program, "LightDirection");
    GLuint laccum_lightColorLocation = glGetUniformLocation(laccum_shader.program, "LightColor");
    GLuint laccum_lightIntensityLocation = glGetUniformLocation(laccum_shader.program, "LightIntensity");

    GLuint laccum_shadowBiasLocation = glGetUniformLocation(laccum_shader.program, "ShadowBias");
    GLuint laccum_shadowSamples = glGetUniformLocation(laccum_shader.program, "ShadowSamples");
    GLuint laccum_shadowSampleSpread = glGetUniformLocation(laccum_shader.program, "ShadowSampleSpread");

     float shadowBias = 0.002f;
    float shadowSamples = 6.0;
    float shadowSampleSpread = 1000.0;
    
    // Load shadow generation shader
    ShaderGLSL shadowgen_shader;
    status = load_shader_from_file(shadowgen_shader, "td3/3_shadowgen.glsl", ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  td3/3_shadowgen.glsl\n");
        exit( EXIT_FAILURE );
    }
    
    // Compute locations for shadow generation shader
    GLuint shadowgen_projectionLocation = glGetUniformLocation(shadowgen_shader.program, "Projection");
    GLuint shadowgen_viewLocation = glGetUniformLocation(shadowgen_shader.program, "View");
    GLuint shadowgen_objectLocation = glGetUniformLocation(shadowgen_shader.program, "Object");
    GLuint shadowgen_timeLocation = glGetUniformLocation(shadowgen_shader.program, "Time");
    GLuint shadowgen_translationLocation = glGetUniformLocation(shadowgen_shader.program, "translation");


    // --------------------------------------------------------------------
    //------------------------- Let's Go ----------------------------------
    //---------------------------------------------------------------------
    
    // Init big model
    int *   model3_FacesIndices;
    uint    model3_NumFaces;
    uint    model3_NumVertices;
    float * model3_Vertices;
    float * model3_Normals;
    float * model3_UVs;

    // Init medium model
    int *   model2_FacesIndices;
    uint    model2_NumFaces;
    uint    model2_NumVertices;
    float * model2_Vertices;
    float * model2_Normals;
    float * model2_UVs;

    // Init little model
    int *   model1_FacesIndices;
    uint    model1_NumFaces;
    uint    model1_NumVertices;
    float * model1_Vertices;
    float * model1_Normals;
    float * model1_UVs;

      // Init little model
    int *   lampe_FacesIndices;
    uint    lampe_NumFaces;
    uint    lampe_NumVertices;
    float * lampe_Vertices;
    float * lampe_Normals;
    float * lampe_UVs;

      // Init little model
    int *   skybox_FacesIndices;
    uint    skybox_NumFaces;
    uint    skybox_NumVertices;
    float * skybox_Vertices;
    float * skybox_Normals;
    float * skybox_UVs;
    
    float * spline_Vertices;
    uint    spline_NumVertices;


    // Init a cube
    int   cube_triangleCount = 12;
    int   cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.01, -0.01, 0.01, 0.01, -0.01, 0.01, -0.01, 0.01, 0.01, 0.01, 0.01, 0.01, -0.01, 0.01, 0.01, 0.01, 0.01, 0.01, -0.01, 0.01, -0.01, 0.01, 0.01, -0.01, -0.01, 0.01, -0.01, 0.01, 0.01, -0.01, -0.01, -0.01, -0.01, 0.01, -0.01, -0.01, -0.01, -0.01, -0.01, 0.01, -0.01, -0.01, -0.01, -0.01, 0.01, 0.01, -0.01, 0.01, 0.01, -0.01, 0.01, 0.01, -0.01, -0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, -0.01, -0.01, -0.01, -0.01, -0.01, -0.01, 0.01, -0.01, 0.01, -0.01, -0.01, 0.01, -0.01, -0.01, -0.01, 0.01, -0.01, 0.01, 0.01 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };

   

    // Init a plane
    int   plane_triangleCount = 2;
    int   plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 100.f, 100.f, 0.f, 100.f, 100.f};
    float plane_vertices[] = {-15.0, -1.0, 15.0, 15.0, -1.0, 15.0, -15.0, -1.0, -15.0, 15.0, -1.0, -15.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
    
    // Init a quad
    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_uvs[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f};
    float quad_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5,};
    float quad_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};

   
/////////////// ----------------------------- Init Camera -------------------- ////////////////////
    float center[4] = {0.996354, -0.527525, 3.66418, 0};
    float dist =  16.0461;
    //camera.init(center, dist, 0.05501, 7.86);
///////////////////////////////////////////////////////////////////////////////////////////////////

    

/////////////// ----------------------------- Init Buffer -------------------- ////////////////////
    // Vertex Array Objects
    GLuint vao[8];
    glGenVertexArrays(8, vao);
    // Vertex Buffer Objects
    GLuint vbo[32];
    glGenBuffers(32, vbo);
///////////////////////////////////////////////////////////////////////////////////////////////////




    // ---------------------------------------------------------------------------
    //------------------------- Bind the Buffers ---------------------------------
    //----------------------------------------------------------------------------

    
    ////////////////////// Plane
    glBindVertexArray(vao[0]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);


    ////////////////////// Quad
    glBindVertexArray(vao[1]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_normals), quad_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_uvs), quad_uvs, GL_STATIC_DRAW);




    ////////////////////// Big Model
    // nb:    3
    DoTheImportThing("obj/modele3.obj", model3_FacesIndices, model3_Vertices, model3_Normals, model3_UVs, model3_NumFaces, model3_NumVertices,  0, 4);
    float model3_translation1[3] = {0,-0.85,4.2};
    float model3_translation2[3] = {9,-0.85,2.5};
    float model3_translation3[3] = {-4.5,-0.85,1};
    float * model3_translation[3] = {model3_translation1, model3_translation2, model3_translation3};
    int i = 2, j = 8;
        BindingInstanceLike(model3_FacesIndices, model3_Vertices, model3_Normals, model3_UVs, model3_NumFaces, model3_NumVertices, vao[2], &vbo[0], 8);

    ////////////////////// Medium Model
    // nb:  14
    DoTheImportThing("obj/modele2.obj", model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, 0, 7.5);
    float model2_translation1[3]  = {3.   ,-1  ,0};
    float model2_translation2[3]  = {-3.5 ,-1  ,6.5};
    float model2_translation3[3]  = {4.3  ,-1  ,2.};
    float model2_translation4[3]  = {-4.1 ,-1  ,2.};
    float model2_translation5[3]  = {-3.1 ,-1  ,-0.1};
    float model2_translation6[3]  = {-4.1 ,-1  ,4.3};
    float model2_translation7[3]  = {4.6  ,-1  ,4.3};
    float model2_translation8[3]  = {4.2  ,-1  ,6.3};
    float model2_translation9[3]  = {2.8  ,-1  ,7.9};
    float model2_translation10[3] = {0.4  ,-1  ,8.2};
    float model2_translation11[3] = {-1.9  ,-1  ,7.8};
    float model2_translation12[3] = {0,-1,-1.25};

    float model2_translation13[3] = {-2.5,-1,-2};
    float model2_translation14[3] = {6.5,-1,3.5};
    float * model2_translation[14] = {model2_translation1,model2_translation2,model2_translation3,model2_translation4,model2_translation5,model2_translation6,model2_translation7,model2_translation8,model2_translation9,model2_translation10,model2_translation11,model2_translation12,model2_translation13,model2_translation14};
    float Model2_angle[14] = {-15+3.14/2, 16.3+3.14/2 , -15.5+3.14/2 , 15.5+3.14/2 , 15+3.14/2 , 16+3.14/2 , -16+3.14/2 , -16.3+3.14/2 , -16.8+3.14/2 , -17.3+3.14/2 , -17.8+3.14/2 , 0 , 0 , 0};
        BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[3], &vbo[0], 12);
    
    
    ////////////////////// Little Model
    // nb:  40
        int Nbmodle = 58;
    DoTheImportThing("obj/modele1.obj", model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, 0, 16);
    float model1_translation1[3] = {2  ,-1  ,-5};
    float model1_translation2[3] = {3  ,-1  ,-5};
    float model1_translation3[3] = {4  ,-1  ,-5};
    float model1_translation4[3] = {5  ,-1  ,-4};
    float model1_translation5[3] = {6  ,-1  ,-3};
    float model1_translation6[3] = {7  ,-1  ,-2};
    float model1_translation7[3] = {8  ,-1  ,-1};
    float model1_translation8[3] = {8  ,-1  ,0};
    float model1_translation9[3] = {8  ,-1  ,1};
    float model1_translation10[3] = {8  ,-1  ,2};
    float model1_translation11[3] = {8  ,-1  ,3};
    float model1_translation12[3] = {8  ,-1  ,4};
    float model1_translation13[3] = {8  ,-1  ,5};
    float model1_translation14[3] = {8  ,-1  ,6};
    float model1_translation15[3] = {8  ,-1  ,7};
    float model1_translation16[3] = {7  ,-1  ,8};
    float model1_translation17[3] = {6  ,-1  ,9};
    float model1_translation18[3] = {5  ,-1  ,10};
    float model1_translation19[3] = {4  ,-1  ,11};
    float model1_translation20[3] = {3  ,-1  ,12};
    float model1_translation21[3] = {2  ,-1  ,12};
    float model1_translation22[3] = {1  ,-1  ,12};
    float model1_translation23[3] = {0  ,-1  ,12};
    float model1_translation24[3] = {-1  ,-1  ,12};
    float model1_translation25[3] = {-2  ,-1  ,12};
    float model1_translation26[3] = {0  ,-1  ,11};
    float model1_translation27[3] = {0  ,-1  ,10};
    float model1_translation28[3] = {1  ,-1  ,10};
    float model1_translation29[3] = {2  ,-1  ,10};
    float model1_translation30[3] = {3  ,-1  ,11};

    float model1_translation31[3] = {-2  ,-1  ,-5};
    float model1_translation32[3] = {-3  ,-1  ,-5};
    float model1_translation33[3] = {-4  ,-1  ,-5};
    float model1_translation34[3] = {-5  ,-1  ,-4};
    float model1_translation35[3] = {-6  ,-1  ,-3};
    float model1_translation36[3] = {-7  ,-1  ,-2};
    float model1_translation37[3] = {-8  ,-1  ,-1};
    float model1_translation38[3] = {-8  ,-1  ,0};
    float model1_translation39[3] = {-8  ,-1  ,1};
    float model1_translation40[3] = {-8  ,-1  ,2};
    float model1_translation41[3] = {-8  ,-1  ,3};
    float model1_translation42[3] = {-8  ,-1  ,4};
    float model1_translation43[3] = {-8  ,-1  ,5};
    float model1_translation44[3] = {-8  ,-1  ,6};
    float model1_translation45[3] = {-8  ,-1  ,7};
    float model1_translation46[3] = {-7  ,-1  ,8};
    float model1_translation47[3] = {-6  ,-1  ,9};
    float model1_translation48[3] = {-5  ,-1  ,10};
    float model1_translation49[3] = {-4  ,-1  ,11};
    float model1_translation50[3] = {-3  ,-1  ,12};
    float model1_translation51[3] = {-2  ,-1  ,12};
    float model1_translation52[3] = {-1  ,-1  ,12};
    float model1_translation53[3] = {-0  ,-1  ,12};
    float model1_translation54[3] = {1  ,-1  ,12};
    float model1_translation55[3] = {2  ,-1  ,12};
    float model1_translation56[3] = {-1  ,-1  ,10};
    float model1_translation57[3] = {-2  ,-1  ,10};
    float model1_translation58[3] = {-3  ,-1  ,11};
   
    float * model1_translation[58] = {model1_translation1,model1_translation2,model1_translation3,model1_translation4,model1_translation5,model1_translation6,model1_translation7,model1_translation8,model1_translation9,model1_translation10,
        model1_translation11,model1_translation12,model1_translation13,model1_translation14,model1_translation15,model1_translation16,model1_translation17,model1_translation18,model1_translation19,model1_translation20,
        model1_translation21,model1_translation22,model1_translation23,model1_translation24,model1_translation25,model1_translation26,model1_translation27,model1_translation28,model1_translation29,model1_translation30,
        model1_translation31,model1_translation32,model1_translation33,model1_translation34,model1_translation35,model1_translation36,model1_translation37,model1_translation38,model1_translation39,model1_translation40,
        model1_translation41,model1_translation42,model1_translation43,model1_translation44,model1_translation45,model1_translation46,model1_translation47,model1_translation48,model1_translation49,model1_translation50,
        model1_translation51,model1_translation52,model1_translation55,model1_translation54,model1_translation55,model1_translation56,model1_translation57,model1_translation58};
   
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[4], &vbo[0], 16);


    ////////////////////// Lamps
    // nb:  15
    DoTheImportThing("obj/lampe_asura.obj", lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, 0, 20);
    float lampe_translation1[3]  = {3.   ,0  ,0};
    float lampe_translation2[3]  = {-3.5 ,0  ,6.5};
    float lampe_translation3[3]  = {4.3  ,0  ,2.};
    float lampe_translation4[3]  = {-4.1 ,0  ,2.};
    float lampe_translation5[3]  = {-3.1 ,0  ,-0.1};
    float lampe_translation6[3]  = {-4.1 ,0  ,4.3};
    float lampe_translation7[3]  = {4.6  ,0  ,4.3};
    float lampe_translation8[3]  = {4.2  ,0  ,6.3};
    float lampe_translation9[3]  = {2.8  ,0  ,7.9};
    float lampe_translation10[3] = {0.4  ,0  ,8.2};
    float lampe_translation11[3] = {-1.9  ,0  ,7.8};
    float lampe_translation12[3] = {0,0,-1.25};

    float lampe_translation13[3]  = {0, 0, 3};
    float lampe_translation14[3]  = {0, 0, 3};
    float lampe_translation15[3]  = {-5, 0, -1.75};
    float * lampe_translation[15] = {lampe_translation1,lampe_translation2,lampe_translation3,lampe_translation4,lampe_translation5,lampe_translation6,lampe_translation7,lampe_translation8,lampe_translation9,lampe_translation10,lampe_translation11,lampe_translation12,lampe_translation13,lampe_translation14,lampe_translation15};
    
   BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[5], &vbo[0], 20);

     ////////////////////// Skybox
    // nb:  
    DoTheImportThing("obj/cube_reversed.obj", skybox_FacesIndices, skybox_Vertices, skybox_Normals, skybox_UVs, skybox_NumFaces, skybox_NumVertices, 0, 0.2);
    float skybox_translation[3] = {00., -2, 0.};
    BindingInstanceLike(skybox_FacesIndices, skybox_Vertices, skybox_Normals, skybox_UVs, skybox_NumFaces, skybox_NumVertices, 
                        vao[6], &vbo[0], 24);

    
    ////////////////////// Quad
    glBindVertexArray(vao[7]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[28]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[29]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[30]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[31]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);



    importSpline("obj/spline.obj", spline_Vertices, spline_NumVertices, 0, 20);



    // Unbind everything. Potentially illegal on some implementations
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // Init frame buffers
    FramebufferGL gbuffer;
    status = build_framebuffer(gbuffer, width, height, 2);
    if (status == -1)
    {
      fprintf(stderr, "Error on building framebuffer\n");
      exit( EXIT_FAILURE );
    }
    
    // Init shadow buffers
    FramebufferGL shadowBuffer;
    status = build_framebuffer(shadowBuffer, 512, 512, 1);
    if (status == -1)
    {
      fprintf(stderr, "Error on building framebuffer\n");
      exit( EXIT_FAILURE );
    }

    FramebufferGL deferredShadingFB;
    status = build_framebuffer(deferredShadingFB, width, height, 1);
    
    if (status == -1)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////// THE LOOP !!!!! ////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool printInfos = true;
  
    float before[3] = {camera.getEye()[0], 0, camera.getEye()[2]};

 
    int index = 0;
    do
    {
      t = glfwGetTime();
      if(cameraAuto){
          if(index > spline_NumVertices)
            index = 0;
          // std::cout << t << std::endl;
          float after[3] = {spline_Vertices[3*index], spline_Vertices[3*index + 1], spline_Vertices[3*index + 2]};
          camera.updateCamera(before, after);
          for(int i = 0 ; i < 3 ; ++i)
            before[i] = after[i];
      }
      //index++;    
      // Mouse states
      int leftButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_LEFT );
      int rightButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_RIGHT );
      int middleButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_MIDDLE );

      if( leftButton == GLFW_PRESS )
        guiStates.turnLock = true;
      else
        guiStates.turnLock = false;

      if( rightButton == GLFW_PRESS )
        guiStates.zoomLock = true;
      else
        guiStates.zoomLock = false;

      if( middleButton == GLFW_PRESS )
        guiStates.panLock = true;
      else
        guiStates.panLock = false;

      // Camera movements
      int altPressed = glfwGetKey(GLFW_KEY_LCTRL);
      int cPressed = glfwGetKey('C');
      int mPressed = glfwGetKey('M');
        

      if(mPressed == GLFW_PRESS && printInfos) {
        int x; int y;
        glfwGetMousePos(&x, &y);       
        std::cout << "Mouse (x,y) : (" << x << ", " << y << ")" << std::endl;
        printInfos = false;
      }

      if(mPressed == GLFW_RELEASE && cPressed == GLFW_RELEASE) {
        printInfos = true;
      }

      if(cPressed == GLFW_PRESS && printInfos) {
        float * cameraCenter = camera.getCenter();
        
        std::cout << " Camera center : (" << cameraCenter[0] << ", " << cameraCenter[1] << ", " << cameraCenter[2] << ", " << cameraCenter[3] << ")" << std::endl;
        std::cout << " Camera radius : " << camera.getRadius() << std::endl;
        std::cout << " Camera phi : " << camera.getPhi() << std::endl;
        std::cout << " Camera theta : " << camera.getTheta() << std::endl;
        printInfos = false;
      }
        
      if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
      {
          int x; int y;
          glfwGetMousePos(&x, &y);
          guiStates.lockPositionX = x;
          guiStates.lockPositionY = y;
      }
      
      if (altPressed == GLFW_PRESS)
      {
        int mousex; int mousey;
        glfwGetMousePos(&mousex, &mousey);
        int diffLockPositionX = mousex - guiStates.lockPositionX;
        int diffLockPositionY = mousey - guiStates.lockPositionY;
        if (guiStates.zoomLock)
        {
          float zoomDir = 0.0;
          if (diffLockPositionX > 0)
            zoomDir = -1.f;
          else if (diffLockPositionX < 0 )
            zoomDir = 1.f;
          camera.zoom(zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
        }
        else if (guiStates.turnLock)
        {
          camera.turn(diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                      diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

        }
        else if (guiStates.panLock)
        {
          camera.pan(diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                     diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
        }
        guiStates.lockPositionX = mousex;
        guiStates.lockPositionY = mousey;
      }
  
      
      // Get camera matrices
      float projection[16];
      float worldToView[16];
      float objectToWorld[16];
      float cameraPosition[4];
      float orthoProj[16];
      ortho(-0.5, 0.5, -0.5, 0.5, -1.0, 1.0, orthoProj);
      mat4fCopy(projection, camera.perspectiveProjection());
      mat4fCopy(worldToView, camera.worldToView());
      mat4fToIdentity(objectToWorld);
      vec4fCopy(cameraPosition, camera.position());

      float viewProjection[16];     
      float iviewProjection[16];       

      mat4fMul( worldToView, projection, viewProjection);
      mat4fInverse(viewProjection, iviewProjection);

      glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);

          glDrawBuffers(gbuffer.outCount, gbuffer.drawBuffers);

          // Viewport 
          glViewport( 0, 0, width, height  );
          camera.setViewport(0, 0, width, height);

          // Default states
          glEnable(GL_DEPTH_TEST);

          // Clear the front buffer
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


          glDisable(GL_DEPTH_TEST);
          // Bind gbuffer shader
          glUseProgram(skybox_shader.program);

          
          // Upload uniforms
          glUniformMatrix4fv(skybox_projectionLocation, 1, 0, projection);
          glUniformMatrix4fv(skybox_viewLocation, 1, 0, worldToView);
          glUniformMatrix4fv(skybox_objectLocation, 1, 0, objectToWorld);
          glUniform1i(skybox_boxLocation, 4);
          glUniform3f(skybox_translationLocation, skybox_translation[0], skybox_translation[1], skybox_translation[2]);

          // Bind textures
          glActiveTexture(GL_TEXTURE4);
          glBindTexture(GL_TEXTURE_2D, textures[4]);


          glBindVertexArray(vao[6]);
          glDrawElements(GL_TRIANGLES, skybox_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          
          glEnable(GL_DEPTH_TEST);



          // Bind gbuffer shader
          glUseProgram(gbuffer_shader.program);
          
          // Upload uniforms
          glUniformMatrix4fv(gbuffer_projectionLocation, 1, 0, projection);
          glUniformMatrix4fv(gbuffer_viewLocation, 1, 0, worldToView);
          glUniformMatrix4fv(gbuffer_objectLocation, 1, 0, objectToWorld);
          

          // --------------------------------------------------------------------
          //------------------------- ON AFFICHE LA --------------------------
          //---------------------------------------------------------------------

          glUniform1i(gbuffer_diffuseLocation, 5);
          glUniform1i(gbuffer_specLocation, 6);
          glUniform1f(gbuffer_rotationLocation, 0);

          glActiveTexture(GL_TEXTURE5);
          glBindTexture(GL_TEXTURE_2D, textures[5]);
          glActiveTexture(GL_TEXTURE6);
          glBindTexture(GL_TEXTURE_2D, textures[6]);

          // Render vaos  
          glBindVertexArray(vao[0]);
          glUniform1f(gbuffer_timeLocation, 0);
          glUniform3f(gbuffer_translationLocation, IdTranslation[0], IdTranslation[1], IdTranslation[2]);
          glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

          glUniform1i(gbuffer_diffuseLocation,0);
          glUniform1i(gbuffer_specLocation, 1);
           // Bind textures
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, textures[0]);
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, textures[1]);

          for(int i = 0; i < Nbmodle; ++i){// 40
              glBindVertexArray(vao[4]);
              glUniform3f(gbuffer_translationLocation, model1_translation[i][0], model1_translation[i][1], model1_translation[i][2]);
              glDrawElements(GL_TRIANGLES, model1_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }

          glUniform1f(gbuffer_timeLocation, t);
          for(int i = 0; i < 15; ++i){// 15
              glBindVertexArray(vao[5]);
              glUniform3f(gbuffer_translationLocation, lampe_translation[i][0], lampe_translation[i][1], lampe_translation[i][2]);
              glDrawElements(GL_TRIANGLES, lampe_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }

          glUniform1i(gbuffer_diffuseLocation, 7);
           // Bind textures
          glActiveTexture(GL_TEXTURE7);
          glBindTexture(GL_TEXTURE_2D, textures[7]);

          glUniform1f(gbuffer_timeLocation, 0);
          for(int i = 0; i < 1; ++i){ // 3
              glBindVertexArray(vao[2]);
              glUniform3f(gbuffer_translationLocation, model3_translation[i][0], model3_translation[i][1], model3_translation[i][2]);
              glDrawElements(GL_TRIANGLES, model3_NumFaces * 3, GL_UNSIGNED_INT, (void*)0); 
          }

          glUniform1i(gbuffer_diffuseLocation, 8);
          // Bind textures
          glActiveTexture(GL_TEXTURE8);
          glBindTexture(GL_TEXTURE_2D, textures[8]);

          for(int i = 0; i < 12; ++i){// 14
              glBindVertexArray(vao[3]);
              glUniform1f(gbuffer_rotationLocation, Model2_angle[i]);
              glUniform3f(gbuffer_translationLocation, model2_translation[i][0], model2_translation[i][1], model2_translation[i][2]);
              glDrawElements(GL_TRIANGLES, model2_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }


         
          glUniform1i(gbuffer_diffuseLocation, 9);
          glUniform1i(gbuffer_specLocation, 10);

          glActiveTexture(GL_TEXTURE9);
          glBindTexture(GL_TEXTURE_2D, textures[9]);
          glActiveTexture(GL_TEXTURE10);
          glBindTexture(GL_TEXTURE_2D, textures[10]);

          glEnable(GL_BLEND);
          glBlendFunc(GL_ONE, GL_ONE);
          glUniform1f(gbuffer_timeLocation, t);
          for(int i = 0 ; i<15 ; ++i) {
            glBindVertexArray(vao[7]);
            glUniform3f(gbuffer_translationLocation, lampe_translation[i][0], lampe_translation[i][1], lampe_translation[i][2]);
            glDrawElements(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
          }
          glUniform1f(gbuffer_timeLocation, 0);
           
          glDisable(GL_BLEND);
            
        
          // Compute light positions
          //float lightPosition[3] = { -4.0, 500.0, -4.0};
          float lightPosition[3] = { 5.0, 5.0, 5.0};
          // float lightPosition[3] = { sin(t/10) * 5.0, 50.0, cos(t/10) * 5.0};
          float lightTarget[3] = { 0.0, 0.0, 0.0};
          float lightDirection[3];
          float lightUp[3] = { 0.0, 1.0, 0.0};
          vec3fSub(lightTarget, lightPosition, lightDirection);
          vec3fNormalize(lightDirection, vec3fNorm(lightDirection));
          float lightColor[3] = {1.0, 1.0, 1.0};
          float lightIntensity = 1.0;

          // Build shadow matrices
          float shadowProjection[16];
          float worldToLight[16];
          lookAt(lightPosition, lightTarget, lightUp, worldToLight);
          perspective(60.f, 1.f, 1.0f, 1000.f, shadowProjection );
          
          
          // Buil projection matrice
          float projectionLight[16];     
          float projectionLightBias[16];     
          mat4fMul( worldToLight, shadowProjection,  projectionLight);
          mat4fMul( projectionLight, MAT4F_M1_P1_TO_P0_P1, projectionLightBias);


      glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);

          // Clear the front buffer
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          glViewport( 0, 0, 512, 512);
         
         
          // Bind shadow shader
          glUseProgram(shadowgen_shader.program);
          // Upload uniforms for shaddws shader
          glUniformMatrix4fv(shadowgen_projectionLocation, 1, 0, shadowProjection);
          glUniformMatrix4fv(shadowgen_viewLocation, 1, 0, worldToLight);
          glUniformMatrix4fv(shadowgen_objectLocation, 1, 0, objectToWorld);
          glUniform1f(shadowgen_timeLocation, t);

          // // Render vaos  
         
          glUniform1f(shadowgen_timeLocation, 0);

          for(int i = 0; i < Nbmodle; ++i){// 40
              glBindVertexArray(vao[4]);
              glUniform3f(shadowgen_translationLocation, model1_translation[i][0], model1_translation[i][1], model1_translation[i][2]);
              glDrawElements(GL_TRIANGLES, model1_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }
          glUniform1f(shadowgen_timeLocation, t);
          for(int i = 0; i < 15; ++i){// 15
              glBindVertexArray(vao[5]);
              glUniform3f(shadowgen_translationLocation, lampe_translation[i][0], lampe_translation[i][1], lampe_translation[i][2]);
              glDrawElements(GL_TRIANGLES, lampe_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }
          glUniform1f(shadowgen_timeLocation, 0);
          for(int i = 0; i < 1; ++i){ // 3
              glBindVertexArray(vao[2]);
              glUniform3f(shadowgen_translationLocation, model3_translation[i][0], model3_translation[i][1], model3_translation[i][2]);
              glDrawElements(GL_TRIANGLES, model3_NumFaces * 3, GL_UNSIGNED_INT, (void*)0); 
          }
          for(int i = 0; i < 12; ++i){// 14
              glBindVertexArray(vao[3]);
              glUniform3f(shadowgen_translationLocation, model2_translation[i][0], model2_translation[i][1], model2_translation[i][2]);
              glDrawElements(GL_TRIANGLES, model2_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }

        
      // Unbind bufferx 
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      glDisable(GL_BLEND);
      // Clear the front buffer
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glViewport( 0, 0, width, height);

      // Bind laccum shader
        glUseProgram(laccum_shader.program);
        // Upload uniforms
        glUniformMatrix4fv(laccum_projectionLocation, 1, 0, orthoProj);
        glUniform1i(laccum_materialLocation, 0);
        glUniform1i(laccum_normalLocation, 1);
        glUniform1i(laccum_depthLocation, 2);
        glUniform1i(laccum_shadowLocation, 3);
        glUniform3fv(laccum_cameraPositionLocation, 1, cameraPosition);
        glUniformMatrix4fv(laccum_inverseViewProjectionLocation, 1, 0, iviewProjection);
        glUniformMatrix4fv(laccum_projectionLightLocation, 1, 0, projectionLightBias);
        glUniform1f(laccum_shadowBiasLocation, shadowBias);
        glUniform1f(laccum_shadowSamples, shadowSamples);
        glUniform1f(laccum_shadowSampleSpread, shadowSampleSpread);

        // Bind color to unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbuffer.colorTexId[0]);        
        // Bind normal to unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbuffer.colorTexId[1]);    
        // Bind depth to unit 2
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbuffer.depthTexId);        
        // Bind shadow map to unit 3
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthTexId);        

        // Blit above the rest
        glDisable(GL_DEPTH_TEST);

        
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_BLEND);


        // Light uniforms
        glUniform3fv(laccum_lightDirectionLocation, 1, lightDirection);
        glUniform3fv(laccum_lightPositionLocation, 1, lightPosition);
        glUniform3fv(laccum_lightColorLocation, 1, lightColor);
        glUniform1f(laccum_lightIntensityLocation, lightIntensity);

      // Draw quad
      glBindVertexArray(vao[1]);
      glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

    


      // // Draw UI
      // glActiveTexture(GL_TEXTURE0);
      // glEnable(GL_BLEND);
      // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      // glViewport(0, 0, width, height);
      // glDisable(GL_DEPTH_TEST);
      // glMatrixMode(GL_PROJECTION);
      // glLoadIdentity();
      // float orthoUI[16];
      // ortho(0, width, 0, height, 0.0, 1.0, orthoUI);
      // glLoadMatrixf(orthoUI);
      // glMatrixMode(GL_MODELVIEW);
      // glLoadIdentity();
      // glUseProgram(0);

      // unsigned char mbut = 0;
      // int mscroll = 0;
      // int mousex; int mousey;
      // glfwGetMousePos(&mousex, &mousey);
      // mousey = height - mousey;

      // if( leftButton == GLFW_PRESS )
      //     mbut |= IMGUI_MBUT_LEFT;
  
      // imguiBeginFrame(mousex, mousey, mbut, mscroll);
      // const char msg[] = "UI Test";
      // int logScroll = 0;
      // imguiBeginScrollArea("Settings", width - 210, height - 310, 200, 300, &logScroll);
      // imguiSlider("bias", &shadowBias, 0.0000, 0.1, 0.0005);
      // imguiSlider("samples", &shadowSamples, 1, 16, 1);
      // imguiSlider("spread", &shadowSampleSpread, 1, 1000, 1);
      // imguiEndScrollArea();
      // imguiEndFrame();


      // imguiRenderGLDraw(); 
      glDisable(GL_BLEND);

      // Check for errors
      GLenum err = glGetError();
      if(err != GL_NO_ERROR)
      {
          fprintf(stderr, "OpenGL Error : %s\n", gluErrorString(err));
      }

      // Swap buffers
      glfwSwapBuffers();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
           glfwGetWindowParam( GLFW_OPENED ) );

    // Clean UI
    imguiRenderGLDestroy();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}


// --------------------------------------------------------------------
//----------- LA BELLE FONCTION DE CHARGEMENT DE MESHES ---------------
//---------------------------------------------------------------------

bool DoTheImportThing(const std::string& pFile, int *& obj_FacesIndices, float *& obj_Vertices, float *& obj_Normals, float *& obj_UVs, 
                      unsigned int &obj_NumFaces,  unsigned int &obj_NumVertices, int numMesh, float proportion)
{
  // Create an instance of the Importer class
  Assimp::Importer importer;

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll 
  // propably to request more postprocessing than we do in this example.
  const aiScene* scene = importer.ReadFile( pFile, 
        aiProcess_CalcTangentSpace       | 
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);
    
    
    obj_NumFaces = scene->mMeshes[numMesh]->mNumFaces;
    obj_FacesIndices = new int[obj_NumFaces*3];
    proportion *= 5;
    
    // Create faces indices tab
    for(size_t j = 0 ; j < obj_NumFaces ; ++j) {
      for(unsigned int k = 0 ; k < 3 ; ++k){
        obj_FacesIndices[j*3+k] = scene->mMeshes[numMesh]->mFaces[j].mIndices[k];
      }
    }
    
    obj_NumVertices = scene->mMeshes[numMesh]->mNumVertices;
    obj_Vertices = new float[3*obj_NumVertices];
    for(size_t j = 0 ; j < obj_NumVertices ; ++j) {
        obj_Vertices[j*3] = (scene->mMeshes[numMesh]->mVertices[j])[0]/proportion;
        obj_Vertices[j*3+1] = (scene->mMeshes[numMesh]->mVertices[j])[1]/proportion;
        obj_Vertices[j*3+2] = (scene->mMeshes[numMesh]->mVertices[j])[2]/proportion;
    }

    obj_Normals = new float[3*obj_NumVertices];
    if(scene->mMeshes[0]->HasNormals()){
      for(size_t j = 0 ; j < obj_NumVertices ; ++j) {
        for(unsigned int k = 0 ; k < 3 ; ++k){
         obj_Normals[j*3+k] = (scene->mMeshes[numMesh]->mNormals[j])[k];
        }
      }
    } 


    obj_UVs = new float[2*obj_NumVertices];
    if(scene->mMeshes[0]->HasTextureCoords(0)){
        for(size_t k = 0 ; k < obj_NumVertices ; ++k) {
            obj_UVs[2*k] = (scene->mMeshes[0]->mTextureCoords[0][k].x);
            obj_UVs[2*k+1] = (scene->mMeshes[0]->mTextureCoords[0][k].y);
        }
    } 

 

 // We're done. Everything will be cleaned up by the importer destructor
  return true;
}

bool importSpline(const std::string& pFile, float *& obj_Vertices, unsigned int &obj_NumVertices, int numMesh, float proportion) {
  // Create an instance of the Importer class
  Assimp::Importer importer;

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll 
  // propably to request more postprocessing than we do in this example.
  const aiScene* scene = importer.ReadFile( pFile, 
        aiProcess_CalcTangentSpace       | 
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);
    
    
    proportion *= 5;
    obj_NumVertices = scene->mMeshes[numMesh]->mNumVertices;
    obj_Vertices = new float[3*obj_NumVertices];
    for(size_t j = 0 ; j < obj_NumVertices ; ++j) {
        obj_Vertices[j*3] = (scene->mMeshes[numMesh]->mVertices[j])[0]/proportion;
        obj_Vertices[j*3+1] = (scene->mMeshes[numMesh]->mVertices[j])[1]/proportion;
        obj_Vertices[j*3+2] = (scene->mMeshes[numMesh]->mVertices[j])[2]/proportion;
    }

    return true;
}


void BindingInstanceLike(int *& obj_FacesIndices, float *& obj_Vertices, float *& obj_Normals, float *& obj_UVs, 
                      unsigned int &obj_NumFaces,  unsigned int &obj_NumVertices, GLuint & vao, GLuint * vbo, int startVBO){
    
    

     // Big Model
    glBindVertexArray(vao);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[startVBO]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj_FacesIndices)*obj_NumFaces*3, obj_FacesIndices, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[startVBO+1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(obj_Vertices)*obj_NumVertices*3, obj_Vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[startVBO+2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(obj_Normals)*obj_NumVertices*3, obj_Normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[startVBO+3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(obj_UVs)*obj_NumVertices*3, obj_UVs, GL_STATIC_DRAW);
}