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
                      unsigned int &obj_NumFaces,  unsigned int &obj_NumVertices, GLuint & vao, GLuint * vbo, int startVBO,
                      float translation[3], float matRotation[16]);

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

    GLuint textures[9];
    glGenTextures(9, textures);
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
    fprintf(stderr, "Diffuse %dx%d:%d\n", x, y, comp);
    unsigned char * spec = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, spec);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "Spec %dx%d:%d\n", x, y, comp);
    
    unsigned char * box = stbi_load("textures/testskyboxUV2.bmp", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, textures[4]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, box);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "Box %dx%d:%d\n", x, y, comp);

    unsigned char * grass = stbi_load("textures/grass2.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, textures[5]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, grass);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "grass %dx%d:%d\n", x, y, comp);

    unsigned char * specgrass = stbi_load("textures/grass2.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, textures[6]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, specgrass);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "grass %dx%d:%d\n", x, y, comp);

    unsigned char * diffuse2 = stbi_load("textures/gigul.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, textures[7]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE,  diffuse2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "grass %dx%d:%d\n", x, y, comp);

    unsigned char * diffuse3 = stbi_load("textures/jeflum.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, textures[8]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE,  diffuse3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "grass %dx%d:%d\n", x, y, comp);

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
    GLuint skybox_rotationTimeLocation = glGetUniformLocation(skybox_shader.program, "RationTime");
    GLuint skybox_rotationLocation = glGetUniformLocation(skybox_shader.program, "Rotation");
    GLuint skybox_boxLocation = glGetUniformLocation(skybox_shader.program, "Box");

   

    // Load light accumulation shader
    ShaderGLSL laccum_shader;
    status = load_shader_from_file(laccum_shader, "td3/3_laccum_spot.glsl", ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  td3/3_laccum_spot.glsl\n");
        exit( EXIT_FAILURE );
    }
    
    // Compute locations for light accumulation shader
    GLuint laccum_projectionLocation = glGetUniformLocation(laccum_shader.program, "Projection");
    GLuint laccum_materialLocation = glGetUniformLocation(laccum_shader.program, "Material");
    GLuint laccum_normalLocation = glGetUniformLocation(laccum_shader.program, "Normal");
    GLuint laccum_depthLocation = glGetUniformLocation(laccum_shader.program, "Depth");
    GLuint laccum_shadowMaplLocation = glGetUniformLocation(laccum_shader.program, "ShadowMap");
    GLuint laccum_inverseViewProjectionLocation = glGetUniformLocation(laccum_shader.program, "InverseViewProjection");
    GLuint laccum_cameraPositionLocation = glGetUniformLocation(laccum_shader.program, "CameraPosition");
    GLuint laccum_lightPositionLocation = glGetUniformLocation(laccum_shader.program, "LightPosition");
    GLuint laccum_lightDirectionLocation = glGetUniformLocation(laccum_shader.program, "LightDirection");
    GLuint laccum_lightColorLocation = glGetUniformLocation(laccum_shader.program, "LightColor");
    GLuint laccum_lightIntensityLocation = glGetUniformLocation(laccum_shader.program, "LightIntensity");
    GLuint laccum_ProjectionLightLocationBias = glGetUniformLocation(laccum_shader.program, "ProjectionLightBias");
    GLuint laccum_BiasLocation = glGetUniformLocation(laccum_shader.program, "Bias");
    GLuint laccum_SamplesLocation = glGetUniformLocation(laccum_shader.program, "Samples");
    GLuint laccum_SpreadLocation = glGetUniformLocation(laccum_shader.program, "Spread");

    float shadowBias = 0.004f;
    float Samples = 1.f;
    float Spread = 400.f;
    
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


    // Init a cube
    int   cube_triangleCount = 12;
    int   cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };

   

    // Init a plane
    int   plane_triangleCount = 2;
    int   plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 100.f, 100.f, 0.f, 100.f, 100.f};
    float plane_vertices[] = {-50.0, -1.0, 50.0, 50.0, -1.0, 50.0, -50.0, -1.0, -50.0, 50.0, -1.0, -50.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
    
    // Init a quad
    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_uvs[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f};
    float quad_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5,};
    float quad_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};

   
/////////////// ----------------------------- Init Camera -------------------- ////////////////////
    float center[4] = {0.21986, 3.55802, 1.11058, 0};
    float dist =  16.0695;
    //camera.init(center, dist, 0.02501, 1.585);
///////////////////////////////////////////////////////////////////////////////////////////////////

    

/////////////// ----------------------------- Init Buffer -------------------- ////////////////////
    // Vertex Array Objects
    GLuint vao[75];
    glGenVertexArrays(75, vao);
    // Vertex Buffer Objects
    GLuint vbo[300];
    glGenBuffers(300, vbo);
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
    float model3_translation1[3] = {0,-0.85,-1};
    float model3_translation2[3] = {5,-0.85,1.5};
    float model3_translation3[3] = {-4.5,-0.85,1};
    BindingInstanceLike(model3_FacesIndices, model3_Vertices, model3_Normals, model3_UVs, model3_NumFaces, model3_NumVertices, vao[2], &vbo[0], 8, model3_translation1, IdRotation);
    BindingInstanceLike(model3_FacesIndices, model3_Vertices, model3_Normals, model3_UVs, model3_NumFaces, model3_NumVertices, vao[3], &vbo[0], 12, model3_translation2, IdRotation);
    BindingInstanceLike(model3_FacesIndices, model3_Vertices, model3_Normals, model3_UVs, model3_NumFaces, model3_NumVertices, vao[4], &vbo[0], 16, model3_translation3, IdRotation);


    ////////////////////// Medium Model
    // nb:  14
    DoTheImportThing("obj/modele2.obj", model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, 0, 7.5);
    float model2_translation1[3] = {1.5,-1,-0.5};
    float model2_translation2[3] = {1.5,-1,-2.5};
    float model2_translation3[3] = {2.5,-1,1.5};
    float model2_translation4[3] = {-2,-1,1.5};
    float model2_translation5[3] = {-2.5,-1,4};
    float model2_translation6[3] = {-6,-1,4};
    float model2_translation7[3] = {4.5,-1,4};
    float model2_translation8[3] = {3.5,-1,-3.5};
    float model2_translation9[3] = {6.5,-1,-2};
    float model2_translation10[3] = {-0.5,-1,-3.5};
    float model2_translation11[3] = {-4,-1,-4};
    float model2_translation12[3] = {-1.5,-1,0.25};
    float model2_translation13[3] = {-2.5,-1,-2};
    float model2_translation14[3] = {6.5,-1,3.5};
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[5], &vbo[0], 20, model2_translation1, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[6], &vbo[0], 24, model2_translation2, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[7], &vbo[0], 28, model2_translation3, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[8], &vbo[0], 32, model2_translation4, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[9], &vbo[0], 36, model2_translation5, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[10], &vbo[0], 40, model2_translation6, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[11], &vbo[0], 44, model2_translation7, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[12], &vbo[0], 48, model2_translation8, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[13], &vbo[0], 52, model2_translation9, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[14], &vbo[0], 56, model2_translation10, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[15], &vbo[0], 60, model2_translation11, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[16], &vbo[0], 64, model2_translation12, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[17], &vbo[0], 68, model2_translation13, IdRotation);
    BindingInstanceLike(model2_FacesIndices, model2_Vertices, model2_Normals, model2_UVs, model2_NumFaces, model2_NumVertices, vao[18], &vbo[0], 72, model2_translation14, IdRotation);

    
    
    ////////////////////// Little Model
    // nb:  40
    DoTheImportThing("obj/modele1.obj", model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, 0, 16);
    float model1_translation1[3] = {3.66,-1,-1.66};
    float model1_translation2[3] = {3.66,-1,-1};
    float model1_translation3[3] = {3.66,-1,-0.33};
    float model1_translation4[3] = {4.33,-1,-1.66};
    float model1_translation5[3] = {4.33,-1,-1};
    float model1_translation6[3] = {-0.66,-1,3.33};
    float model1_translation7[3] = {0,-1,3.33};
    float model1_translation8[3] = {0.66,-1,3.33};
    float model1_translation9[3] = {1.33,-1,3.33};
    float model1_translation10[3] = {2,-1,3.33};
    float model1_translation11[3] = {2.66,-1,3.33};
    float model1_translation12[3] = {-0.66,-1,4};
    float model1_translation13[3] = {0,-1,4};
    float model1_translation14[3] = {0.66,-1,4};
    float model1_translation15[3] = {1.33,-1,4};
    float model1_translation16[3] = {2,-1,4};
    float model1_translation17[3] = {2.66,-1,4};
    float model1_translation18[3] = {7,-1,0};
    float model1_translation19[3] = {7,-1,0.66};
    float model1_translation20[3] = {7,-1,1.33};
    float model1_translation21[3] = {7,-1,2};
    float model1_translation22[3] = {7,-1,2.66};
    float model1_translation23[3] = {-3.66,-1,4.33};
    float model1_translation24[3] = {-4.33,-1,4.33};
    float model1_translation25[3] = {-5,-1,4.33};
    float model1_translation26[3] = {-5,-1,-2.33};
    float model1_translation27[3] = {-5.5,-1,-4.33};
    float model1_translation28[3] = {0.33,-1,-4.33};
    float model1_translation29[3] = {1,-1,-4.33};
    float model1_translation30[3] = {3,-1,-4.33};
    float model1_translation31[3] = {3.66,-1,-4.33};
    float model1_translation32[3] = {5.33,-1,-4.33};
    float model1_translation33[3] = {6,-1,-4.33};
    float model1_translation34[3] = {6.66,-1,-4.33};
    float model1_translation35[3] = {6.66,-1,-3.66};
    float model1_translation36[3] = {-1.33,-1,-4.33};
    float model1_translation37[3] = {-2,-1,-4.33};
    float model1_translation38[3] = {-2.66,-1,-4.33};
    float model1_translation39[3] = {-5,-1,-4.33};
    float model1_translation40[3] = {-5.66,-1,-4.33};
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[19], &vbo[0], 76, model1_translation1, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[20], &vbo[0], 80, model1_translation2, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[21], &vbo[0], 84, model1_translation3, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[22], &vbo[0], 88, model1_translation4, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[23], &vbo[0], 92, model1_translation5, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[24], &vbo[0], 96, model1_translation6, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[25], &vbo[0], 100, model1_translation7, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[26], &vbo[0], 104, model1_translation8, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[27], &vbo[0], 108, model1_translation9, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[28], &vbo[0], 112, model1_translation10, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[29], &vbo[0], 116, model1_translation11, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[30], &vbo[0], 120, model1_translation12, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[31], &vbo[0], 124, model1_translation13, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[32], &vbo[0], 128, model1_translation14, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[33], &vbo[0], 132, model1_translation15, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[34], &vbo[0], 136, model1_translation16, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[35], &vbo[0], 140, model1_translation17, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[36], &vbo[0], 144, model1_translation18, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[37], &vbo[0], 148, model1_translation19, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[38], &vbo[0], 152, model1_translation20, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[39], &vbo[0], 156, model1_translation21, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[40], &vbo[0], 160, model1_translation22, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[41], &vbo[0], 164, model1_translation23, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[42], &vbo[0], 168, model1_translation24, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[43], &vbo[0], 172, model1_translation25, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[44], &vbo[0], 176, model1_translation26, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[45], &vbo[0], 180, model1_translation27, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[46], &vbo[0], 184, model1_translation28, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[47], &vbo[0], 188, model1_translation29, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[48], &vbo[0], 192, model1_translation30, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[49], &vbo[0], 196, model1_translation31, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[50], &vbo[0], 200, model1_translation32, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[51], &vbo[0], 204, model1_translation33, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[52], &vbo[0], 208, model1_translation34, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[53], &vbo[0], 212, model1_translation35, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[54], &vbo[0], 216, model1_translation36, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[55], &vbo[0], 220, model1_translation37, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[56], &vbo[0], 224, model1_translation38, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[57], &vbo[0], 228, model1_translation39, IdRotation);
    BindingInstanceLike(model1_FacesIndices, model1_Vertices, model1_Normals, model1_UVs, model1_NumFaces, model1_NumVertices, vao[58], &vbo[0], 232, model1_translation40, IdRotation);


    ////////////////////// Lamps
    // nb:  15
    DoTheImportThing("obj/lampe_asura.obj", lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, 0, 20);
    float lampe_translation1[3*15] = {0, 0.3, 1.5};
    float lampe_translation2[3] = {0, 0, 0.5};
    float lampe_translation3[3] = {-0.66, 0, 0.88};
    float lampe_translation4[3] = {-1, 0, 1.5};
    float lampe_translation5[3] = {-0.66, 0, 2.16};
    float lampe_translation6[3] = {0, 0, 1.5};
    float lampe_translation7[3] = {0.66, 0, 2.16};
    float lampe_translation8[3] = {1, 0, 1.5};
    float lampe_translation9[3] = {0.66, 0, 0.88};
    float lampe_translation10[3]  = {0, 0, 3};
    float lampe_translation11[3]  = {0, 0, 3};
    float lampe_translation12[3]  = {0, 0, 3};
    float lampe_translation13[3]  = {0, 0, 3};
    float lampe_translation14[3]  = {0, 0, 3};
    float lampe_translation15[3]  = {-5, 0, -1.75};
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[59], &vbo[0], 236, lampe_translation1, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[60], &vbo[0], 240, lampe_translation2, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[61], &vbo[0], 244, lampe_translation3, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[62], &vbo[0], 248, lampe_translation4, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[63], &vbo[0], 252, lampe_translation5, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[64], &vbo[0], 256, lampe_translation6, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[65], &vbo[0], 260, lampe_translation7, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[66], &vbo[0], 264, lampe_translation8, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[67], &vbo[0], 268, lampe_translation9, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[68], &vbo[0], 272, lampe_translation10, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[69], &vbo[0], 276, lampe_translation11, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[70], &vbo[0], 280, lampe_translation12, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[71], &vbo[0], 284, lampe_translation13, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[72], &vbo[0], 288, lampe_translation14, IdRotation);
    BindingInstanceLike(lampe_FacesIndices, lampe_Vertices, lampe_Normals, lampe_UVs, lampe_NumFaces, lampe_NumVertices, vao[73], &vbo[0], 292, lampe_translation15, IdRotation);


     ////////////////////// Skybox
    // nb:  
    DoTheImportThing("obj/cube_reversed.obj", skybox_FacesIndices, skybox_Vertices, skybox_Normals, skybox_UVs, skybox_NumFaces, skybox_NumVertices, 0, 0.010);
    float skybox_translation[3] = {00., 0, 0.};
    BindingInstanceLike(skybox_FacesIndices, skybox_Vertices, skybox_Normals, skybox_UVs, skybox_NumFaces, skybox_NumVertices, 
                        vao[74], &vbo[0], 296, skybox_translation, IdRotation);

   








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


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////// THE LOOP !!!!! ////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool printInfos = true;
    do
    {
      t = glfwGetTime();

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
          
          // Bind textures
          glActiveTexture(GL_TEXTURE4);
          glBindTexture(GL_TEXTURE_2D, textures[4]);


          glBindVertexArray(vao[74]);
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

          glActiveTexture(GL_TEXTURE5);
          glBindTexture(GL_TEXTURE_2D, textures[5]);
          glActiveTexture(GL_TEXTURE6);
          glBindTexture(GL_TEXTURE_2D, textures[6]);

          // Render vaos  
          glBindVertexArray(vao[0]);
          glUniform1f(gbuffer_timeLocation, 0);
          glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

          glUniform1i(gbuffer_diffuseLocation,0);
          glUniform1i(gbuffer_specLocation, 1);
           // Bind textures
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, textures[0]);
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, textures[1]);

          for(int i = 0; i < 40; ++i){// 40
              glBindVertexArray(vao[i+19]);
              glDrawElements(GL_TRIANGLES, model1_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }

          glUniform1f(gbuffer_timeLocation, t);
          for(int i = 0; i < 15; ++i){// 15
              glBindVertexArray(vao[i+59]);
              glDrawElements(GL_TRIANGLES, lampe_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }

          glUniform1i(gbuffer_diffuseLocation, 7);
           // Bind textures
          glActiveTexture(GL_TEXTURE7);
          glBindTexture(GL_TEXTURE_2D, textures[7]);

          glUniform1f(gbuffer_timeLocation, 0);
          for(int i = 0; i < 3; ++i){ // 3
              glBindVertexArray(vao[i+2]);
              glDrawElements(GL_TRIANGLES, model3_NumFaces * 3, GL_UNSIGNED_INT, (void*)0); 
          }

          glUniform1i(gbuffer_diffuseLocation, 8);
          // Bind textures
          glActiveTexture(GL_TEXTURE8);
          glBindTexture(GL_TEXTURE_2D, textures[8]);

          for(int i = 0; i < 14; ++i){// 14
              glBindVertexArray(vao[i+5]);
              glDrawElements(GL_TRIANGLES, model2_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }
         

        

        
          // Compute light positions
          float lightPosition[3] = { -4.0, 500.0, -4.0};
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

          // Render vaos  
          glBindVertexArray(vao[0]);
          glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

          for(int i = 0; i < 40; ++i){// 40
              glBindVertexArray(vao[i+19]);
              glDrawElements(GL_TRIANGLES, model1_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }
          for(int i = 0; i < 15; ++i){// 15
              glBindVertexArray(vao[i+59]);
              glDrawElements(GL_TRIANGLES, lampe_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }
          for(int i = 0; i < 3; ++i){ // 3
              glBindVertexArray(vao[i+2]);
              glDrawElements(GL_TRIANGLES, model3_NumFaces * 3, GL_UNSIGNED_INT, (void*)0); 
          }
          for(int i = 0; i < 14; ++i){// 14
              glBindVertexArray(vao[i+5]);
              glDrawElements(GL_TRIANGLES, model2_NumFaces * 3, GL_UNSIGNED_INT, (void*)0);
          }


      // Unbind bufferx 
      glBindFramebuffer(GL_FRAMEBUFFER, 0);


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
      glUniform1i(laccum_shadowMaplLocation, 3);
      glUniform3fv(laccum_cameraPositionLocation, 1, cameraPosition);
      glUniformMatrix4fv(laccum_inverseViewProjectionLocation, 1, 0, iviewProjection);
      glUniformMatrix4fv(laccum_ProjectionLightLocationBias, 1, 0, projectionLightBias);
      
      

      // Bind color to unit 0
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gbuffer.colorTexId[0]);        
      // Bind normal to unit 1
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, gbuffer.colorTexId[1]);    
      // Bind depth to unit 2
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, gbuffer.depthTexId);
      // Bind normal to unit 1
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D,shadowBuffer.depthTexId);        

      // Blit above the rest
      glDisable(GL_DEPTH_TEST);

      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);

      // Light uniforms
      glUniform3fv(laccum_lightDirectionLocation, 1, lightDirection);
      glUniform3fv(laccum_lightPositionLocation, 1, lightPosition);
      glUniform3fv(laccum_lightColorLocation, 1, lightColor);
      glUniform1f(laccum_lightIntensityLocation, lightIntensity);
      glUniform1f(laccum_BiasLocation, shadowBias);
      glUniform1f(laccum_SamplesLocation, Samples);
      glUniform1f(laccum_SpreadLocation, Spread);

      // Draw quad
      glBindVertexArray(vao[1]);
      glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        glDisable(GL_BLEND);
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
      // imguiSlider("samples", &Samples, 1, 16, 1);
      // imguiSlider("spread", &Spread, 1, 1000, 1);
      // imguiEndScrollArea();
      // imguiEndFrame();


      // imguiRenderGLDraw(); 
      // glDisable(GL_BLEND);

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



void BindingInstanceLike(int *& obj_FacesIndices, float *& obj_Vertices, float *& obj_Normals, float *& obj_UVs, 
                      unsigned int &obj_NumFaces,  unsigned int &obj_NumVertices, GLuint & vao, GLuint * vbo, int startVBO,
                      float translation[3], float matRotation[16]){
    
    

    float InvmatRotation[16];
    mat4fInverse(matRotation, InvmatRotation);

    for(size_t j = 0 ; j < obj_NumVertices ; ++j) {
        float tmp[3];
        const float vert[3] = {obj_Vertices[j*3], obj_Vertices[j*3+1], obj_Vertices[j*3+2]};
        mat4fMulV3(matRotation, vert , tmp);
        
        obj_Vertices[j*3] = tmp[0]   += translation[0];
        obj_Vertices[j*3 + 1] = tmp[1] += translation[1];
        obj_Vertices[j*3 + 2] = tmp[2] += translation[2];
    }

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

    for(size_t j = 0 ; j < obj_NumVertices ; ++j) {


        obj_Vertices[j*3]   -= translation[0];
        obj_Vertices[j*3+1] -= translation[1];
        obj_Vertices[j*3+2] -= translation[2];


        float tmp[3];
        const float vert[3] = {obj_Vertices[j*3], obj_Vertices[j*3+1], obj_Vertices[j*3+2]};
        mat4fMulV3(InvmatRotation, vert , tmp);

        obj_Vertices[j*3]   = tmp[0];
        obj_Vertices[j*3+1] = tmp[1];
        obj_Vertices[j*3+2] = tmp[2];
    }
}