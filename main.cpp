#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <stack>
#include <stdlib.h>

using namespace std;
using namespace glm;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void renderScene(Shader& shader, Model& Merc, Model& F1Generic, Model& Road, Model& Grandstand, Model& Renault);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool shadows = true;
bool shadowsKeyPressed = false;

// camera
Camera camera(glm::vec3(0.0f, 3.0f, 5.0f));
float SPEED = 2.5f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//Lighting Stuff
vec3 lightStrength = vec3(0.7f);

//Animation Stuff (But not really this is actually positioning stuff)
float F1GenericGoX = -1.3f, F1GenericGoY = 0.0f, F1GenericGoZ = 0.0f;
float RenaultGoX = 1.4f, RenaultGoY = 0.4f, RenaultGoZ = -5.0f;
float MercGoX = -1.4f, MercGoY = 0.4f, MercGoZ = -10.2f;

//Actual Animation Stuff
int Icar1 = rand() % 70 + 30;
int Icar2 = rand() % 70 + 30;
int Icar3 = rand() % Icar1 + 30;
float car1 = static_cast<float>(Icar1) / 1000;
float car2 = static_cast<float>(Icar2) / 1000;
float car3 = static_cast<float>(Icar3) / 1000;

int main()
{
    cout << "MOVEMENT CONTROLS" << endl;
    cout << "The controls are set up like a 'fly camera'" << endl;
    cout << "So, use the mouse to look around, and the WASD to move!" << endl;
    cout << "W: Moves Forward" << endl;
    cout << "S: Moves Backward" << endl;
    cout << "A: Strafes Left" << endl;
    cout << "D: Strafes Right" << endl;
    cout << "LEFT SHIFT: Engages 'Sprint Move' while held done" << endl;
    cout << "" << endl;
    
    cout << "ANIMATION CONTROLS" << endl;
    cout << "This scene simulates the most crucial stages of an F1 Race Start - Getting of the line" << endl;
    cout << "Pressing and holding the 'UP' arrow key gets the cars off the line and allows you to watch the start" << endl;
    cout << "Pressing 'R' resets the cars to the starting postion" << endl;
    cout << "Each subsequent race start is always different!" << endl;
    cout << "" << endl;

    cout << "Lighting Controls" << endl;
    cout << "Q: Increase the strength of the light" << endl;
    cout << "E: Decrease the strength of the light" << endl;
    cout << "SPACEBAR: Toggle Shadows On/Off" << endl;


    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader mainShader("Glitter/Shaders/mainVert.vert", "Glitter/Shaders/mainFrag.frag");;
    Shader shadowShader("Glitter/Shaders/shadowVert.vert", "Glitter/Shaders/shadowFrag.frag", "Glitter/Shaders/shadowGeo.geo");
    Shader skyboxShader("Glitter/Shaders/skybox.vert", "Glitter/Shaders/skybox.frag");
    
    //Models
    Model Merc("Glitter/Sources/Merc/untitled.obj");
    Model Renault("Glitter/Sources/Renault/untitled.obj");
    Model Sphere("Glitter/Sources/Sphere/untitled.obj");
    Model F1Generic("Glitter/Sources/F1Generic/untitled.obj");
    Model Road("Glitter/Sources/Road/untitled.obj");
    Model Grandstand("Glitter/Sources/Grandstand/untitled.obj");

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // shader configuration
    // --------------------
    mainShader.use();
    mainShader.setInt("diffuseTexture", 0);
    mainShader.setInt("depthMap", 1);

    //-------------------------------------------------------------------------
    //Everything Relating to the Skybox
    //Skybox cube
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
    {
        "Glitter/Sources/Sky/right.png",
        "Glitter/Sources/Sky/left.png",
        "Glitter/Sources/Sky/top.png",
        "Glitter/Sources/Sky/bottom.png",
        "Glitter/Sources/Sky/front.png",
        "Glitter/Sources/Sky/back.png"
    };


    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    //-------------------------------------------------------------------------

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Define light position, and attach it just BELLOW the camera to make shadows easier to see.
        vec3 lightPos = camera.Position + vec3(0.0f, -2.0f, 0.0f);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 0. create depth cubemap transformation matrices
        // -----------------------------------------------
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        // 1. render scene to depth cubemap
        // --------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            shadowShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        shadowShader.setFloat("far_plane", far_plane);
        shadowShader.setVec3("lightPos", lightPos);
        renderScene(shadowShader, Merc, F1Generic, Road, Grandstand, Renault);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. render scene as normal 
        // -------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mainShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        mainShader.setMat4("projection", projection);
        mainShader.setMat4("view", view);
        // set lighting uniforms
        mainShader.setVec3("lightPos", lightPos);
        mainShader.setVec3("viewPos", camera.Position);
        mainShader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
        mainShader.setFloat("far_plane", far_plane);
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, woodTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        renderScene(mainShader, Merc, F1Generic, Road, Grandstand, Renault);


        //-----------------------------------------------------------
        //SKYBOX STUFF IN HERE
        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        //-----------------------------------------------------------

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// renders the 3D scene
// --------------------
void renderScene(Shader& shader, Model& Merc, Model& F1Generic, Model& Road, Model& Grandstand, Model& Renault)
{

    //Model stack and transformations
    stack<mat4> model;
    model.push(mat4(1.0));
    shader.setMat4("model", model.top());
    shader.setVec3("lightStrength", lightStrength);

    //Generic F1 Car
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(F1GenericGoX, F1GenericGoY, F1GenericGoZ)); // Setting up in "1st Place" + Animation
        //model.top() = glm::translate(model.top(), glm::vec3(-1.3f, 0.0f, 0.0f)); // Setting up in "1st Place"
        model.top() = glm::scale(model.top(), glm::vec3(1.0f, 1.0f, 1.0f));	// Size it
        shader.setMat4("model", model.top());


        F1Generic.Draw(shader);
    }
    model.pop();

    //Renault
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(RenaultGoX, RenaultGoY, RenaultGoZ)); // Setting up in "1st Place" + Animation
        //model.top() = glm::translate(model.top(), glm::vec3(1.4f, 0.4f, -5.0f)); // Setting up in "2nd Place"
        model.top() = glm::scale(model.top(), glm::vec3(0.7f, 0.7f, 0.7f));	// Sizing
        shader.setMat4("model", model.top());

        Renault.Draw(shader);
    }
    model.pop();

    //Merc
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(MercGoX, MercGoY, MercGoZ)); // Setting up in "1st Place" + Animation
        //model.top() = glm::translate(model.top(), glm::vec3(-1.4f, 0.4f, -10.2f)); // Setting up in "3rd Place"
        model.top() = glm::scale(model.top(), glm::vec3(0.7f, 0.7f, 0.7f));	// Sizing
        shader.setMat4("model", model.top());


        Merc.Draw(shader);
    }
    model.pop();

    //Road
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(0.0f, 0.0f, -8.0f)); // Road in the center
        model.top() = glm::scale(model.top(), glm::vec3(150.0f, 100.0f, 150.0f));	// Sizing
        shader.setMat4("model", model.top());

        //Light Properties


        Road.Draw(shader);

        //Resetting light properties

    }
    model.pop();

    //Road
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(0.0f, 0.0f, 10.0f)); // Road in the center
        model.top() = glm::scale(model.top(), glm::vec3(150.0f, 100.0f, 150.0f));	// Sizing
        shader.setMat4("model", model.top());

        //Light Properties


        Road.Draw(shader);

        //Resetting light properties
    }
    model.pop();

    //Road
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(0.0f, 0.0f, 28.0f)); // Road in the center
        model.top() = glm::scale(model.top(), glm::vec3(150.0f, 100.0f, 150.0f));	// Sizing
        shader.setMat4("model", model.top());

        //Light Properties


        Road.Draw(shader);

        //Resetting light properties
    }
    model.pop();


    //Grandstand right
    model.push(model.top());
    {
        // render the loaded model

        model.top() = glm::rotate(model.top(), radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
        model.top() = glm::translate(model.top(), glm::vec3(33.5f, -6.4f, -16.05f)); // Off too the left
        model.top() = glm::scale(model.top(), glm::vec3(0.2f, 0.2f, 0.2f));	// Sizing
        shader.setMat4("model", model.top());

        Grandstand.Draw(shader);
    }
    model.pop();

    //Grandstand left
    model.push(model.top());
    {
        // render the loaded model
        model.top() = glm::translate(model.top(), glm::vec3(33.5f, -6.4f, 15.78f)); // Off too the left
        model.top() = glm::scale(model.top(), glm::vec3(0.2f, 0.2f, 0.2f));	// Sizing
        shader.setMat4("model", model.top());

        Grandstand.Draw(shader);
    }
    model.pop();
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.sprint(12.5f);
    }
    else {
        camera.walk(2.5f);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        //Stopping the cars from going off track after the start phase
        if (F1GenericGoZ < 34.5 && RenaultGoZ < 34.5) {
            F1GenericGoZ += car1;
            RenaultGoZ += car2;
            MercGoZ += car3;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        Icar1 = rand() % 70 + 30;
        Icar2 = rand() % 70 + 30;
        Icar3 = rand() % Icar1 + 30;
        while (Icar3 > Icar1) {
            Icar3 = rand() % Icar1 + 30;
        }
        car1 = static_cast<float>(Icar1) / 1000;
        car2 = static_cast<float>(Icar2) / 1000;
        car3 = static_cast<float>(Icar3) / 1000;
        F1GenericGoZ = 0.0;
        RenaultGoZ = -5.0;
        MercGoZ = -10.2;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        if (lightStrength.x < 0.9f) {
            lightStrength += 0.005;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (lightStrength.x > 0.1f) {
            lightStrength -= 0.005;
        }
    }
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}