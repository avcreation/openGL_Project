#include <stdio.h>
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
    GLuint textures[3];
    glGenTextures(3, textures);
    int x;
    int y;
    int comp; 
    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);
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

    // Load Blit shader
    ShaderGLSL blit_shader;
    status = load_shader_from_file(blit_shader, "td3/3_blit.glsl", ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  td3/3_blit.glsl\n");
        exit( EXIT_FAILURE );
    }
    // Compute locations for blit_shader
    GLuint blit_projectionLocation = glGetUniformLocation(blit_shader.program, "Projection");
    GLuint blit_tex1Location = glGetUniformLocation(blit_shader.program, "Texture1");

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

    // Load geometry
    int   cube_triangleCount = 12;
    int   cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };
    int   plane_triangleCount = 2;
    int   plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 10.f, 10.f, 0.f, 10.f, 10.f};
    float plane_vertices[] = {-50.0, -1.0, 50.0, 50.0, -1.0, 50.0, -50.0, -1.0, -50.0, 50.0, -1.0, -50.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_uvs[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f};
    float quad_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5,};
    float quad_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};

    // Vertex Array Object
    GLuint vao[3];
    glGenVertexArrays(3, vao);

    // Vertex Buffer Objects
    GLuint vbo[12];
    glGenBuffers(12, vbo);

    // Cube
    glBindVertexArray(vao[0]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);

    // Plane
    glBindVertexArray(vao[1]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);


    // Quad
    glBindVertexArray(vao[2]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[10]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_normals), quad_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_uvs), quad_uvs, GL_STATIC_DRAW);

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
    
    // Init frame buffers
    FramebufferGL shadowBuffer;
    status = build_framebuffer(shadowBuffer, 512, 512, 1);
    if (status == -1)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }

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

        // Bind gbuffer shader
        glUseProgram(gbuffer_shader.program);
        // Upload uniforms
        glUniformMatrix4fv(gbuffer_projectionLocation, 1, 0, projection);
        glUniformMatrix4fv(gbuffer_viewLocation, 1, 0, worldToView);
        glUniformMatrix4fv(gbuffer_objectLocation, 1, 0, objectToWorld);
        glUniform1f(gbuffer_timeLocation, t);
        glUniform1i(gbuffer_diffuseLocation, 0);
        glUniform1i(gbuffer_specLocation, 1);

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 4);
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        

   

        // Compute light positions
      	// float lightPosition[3] = { 5.0, 5.0, 5.0};
        float lightPosition[3] = { sin(t) * 5.0, 20.0, cos(t) * 5.0};
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
        // Upload uniforms
        glUniformMatrix4fv(shadowgen_projectionLocation, 1, 0, shadowProjection);
        glUniformMatrix4fv(shadowgen_viewLocation, 1, 0, worldToLight);
        glUniformMatrix4fv(shadowgen_objectLocation, 1, 0, objectToWorld);
        glUniform1f(shadowgen_timeLocation, t);

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 4);
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        
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
        glBindVertexArray(vao[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        glDisable(GL_BLEND);

        // Bind blit shader
        glUseProgram(blit_shader.program);
        // Upload uniforms
        glUniformMatrix4fv(blit_projectionLocation, 1, 0, orthoProj);
        glUniform1i(blit_tex1Location, 0);
        // use only unit 0
        glActiveTexture(GL_TEXTURE0);

        // Viewport 
        glViewport( 0, 0, width/4, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbuffer.colorTexId[0]);        
        // Draw quad
        glBindVertexArray(vao[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        // Viewport 
        glViewport( width/4, 0, width/4, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbuffer.colorTexId[1]);        
        // Draw quad
        glBindVertexArray(vao[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        // Viewport 
        glViewport( width/4 * 2, 0, width/4, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbuffer.depthTexId);        
        // Draw quad
        glBindVertexArray(vao[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        // Viewport 
        glViewport( width/4 * 3, 0, width/4, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthTexId);        
        // Draw quad
        glBindVertexArray(vao[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);


        // Draw UI
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float orthoUI[16];
        ortho(0, width, 0, height, 0.0, 1.0, orthoUI);
        glLoadMatrixf(orthoUI);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glUseProgram(0);

        unsigned char mbut = 0;
        int mscroll = 0;
        int mousex; int mousey;
        glfwGetMousePos(&mousex, &mousey);
        mousey = height - mousey;

        if( leftButton == GLFW_PRESS )
            mbut |= IMGUI_MBUT_LEFT;
    
        imguiBeginFrame(mousex, mousey, mbut, mscroll);
        const char msg[] = "UI Test";
        int logScroll = 0;
        imguiBeginScrollArea("Settings", width - 210, height - 310, 200, 300, &logScroll);
        imguiSlider("bias", &shadowBias, 0.0000, 0.1, 0.0005);
        imguiSlider("samples", &Samples, 1, 16, 1);
        imguiSlider("spread", &Spread, 1, 1000, 1);
        imguiEndScrollArea();
        imguiEndFrame();


        imguiRenderGLDraw(); 
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

