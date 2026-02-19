#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../Cloth.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

// ── Constants ────────────────────────────────────────────────────────────────
static constexpr int  WINDOW_WIDTH  = 1280;
static constexpr int  WINDOW_HEIGHT = 720;
static constexpr char WINDOW_TITLE[] = "Cloth Simulation";
static constexpr char GLSL_VERSION[] = "#version 330 core";

// ── Shaders ──────────────────────────────────────────────────────────────────
// Simple MVP transform — just positions, flat white color for now.
// Will be replaced by Phong shaders in Phase 4.
static const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
    gl_PointSize = 2.0;
}
)";

static const char* FRAG_SRC = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 uColor;

void main()
{
    FragColor = vec4(uColor, 1.0);
}
)";

// ── Shader helpers ───────────────────────────────────────────────────────────
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return shader;
}

static GLuint buildProgram(const char* vertSrc, const char* fragSrc)
{
    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ── Callbacks ────────────────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main()
{
    // ── Init GLFW ────────────────────────────────────────────────────────────
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // ── Init GLAD ────────────────────────────────────────────────────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }
    std::cout << "OpenGL "  << glGetString(GL_VERSION)  << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE); // lets the shader set gl_PointSize

    // ── Init ImGui ───────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    // ── Cloth ─────────────────────────────────────────────────────────────────
    constexpr int   CLOTH_ROWS    = 30;
    constexpr int   CLOTH_COLS    = 30;
    constexpr float CLOTH_SPACING = 0.1f;

    Cloth cloth(CLOTH_ROWS, CLOTH_COLS, CLOTH_SPACING);

    // ── GPU buffers for particle positions ───────────────────────────────────
    // Main VAO/VBO — all particles
    GLuint pointVAO, pointVBO;
    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 CLOTH_ROWS * CLOTH_COLS * 3 * sizeof(float),
                 nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Pinned VAO/VBO — separate small buffer, never more than a handful of points
    GLuint pinnedVAO, pinnedVBO;
    glGenVertexArrays(1, &pinnedVAO);
    glGenBuffers(1, &pinnedVBO);
    glBindVertexArray(pinnedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pinnedVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 CLOTH_ROWS * CLOTH_COLS * 3 * sizeof(float), // worst case all pinned
                 nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Shader ───────────────────────────────────────────────────────────────
    GLuint shader  = buildProgram(VERT_SRC, FRAG_SRC);
    GLint  locMVP  = glGetUniformLocation(shader, "uMVP");
    GLint  locColor = glGetUniformLocation(shader, "uColor");

    // ── Camera / projection ──────────────────────────────────────────────────
    // Simple fixed camera looking at the cloth from a slight angle.
    // In Phase 4 we'll add proper camera controls.
    glm::mat4 proj = glm::perspective(glm::radians(45.f),
                                      (float)WINDOW_WIDTH / WINDOW_HEIGHT,
                                      0.01f, 100.f);

    glm::vec3 cameraPos = { 0.f, 1.f, 8.5f };

    glm::mat4 view = glm::lookAt(
                                 cameraPos,   // camera position
        glm::vec3(0.f, 0.5f, 0.f),   // look-at target (center of cloth)
        glm::vec3(0.f, 1.f, 0.f)     // up vector
    );
    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 MVP   = proj * view * model;

    // ── Simulation state ─────────────────────────────────────────────────────
    bool  simRunning  = true;
    bool  showParticles = true;
    float bgColor[3]  = { 0.12f, 0.12f, 0.14f };
    float dt          = 1.f / 60.f;

    double lastTime = glfwGetTime();

    // ── Render loop ──────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Delta time
        double now   = glfwGetTime();
        float  delta = (float)(now - lastTime);
        lastTime     = now;

        // ── Simulate ─────────────────────────────────────────────────────────
        if (simRunning)
            cloth.update(dt);

        // ── Upload particle positions to GPU ──────────────────────────────────
        const auto& particles = cloth.getParticles();
        std::vector<float> posData;
        std::vector<float> pinnedData;
        posData.reserve(particles.size() * 3);
        for (const auto& p : particles) {
            posData.push_back(p.position.x);
            posData.push_back(p.position.y);
            posData.push_back(p.position.z);
            if (p.pinned) {
                pinnedData.push_back(p.position.x);
                pinnedData.push_back(p.position.y);
                pinnedData.push_back(p.position.z);
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        posData.size() * sizeof(float), posData.data());

        glBindBuffer(GL_ARRAY_BUFFER, pinnedVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        pinnedData.size() * sizeof(float), pinnedData.data());

        // ── ImGui ─────────────────────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Cloth Simulation");

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Separator();

        ImGui::Text("Simulation");
        ImGui::Checkbox("Running", &simRunning);
        if (ImGui::Button("Reset"))
            cloth.reset();
        ImGui::SliderFloat("dt", &dt, 0.001f, 0.033f, "%.4f");
        ImGui::Separator();

        ImGui::Text("Camera");
        ImGui::SliderFloat3("Camera pos", glm::value_ptr(cameraPos), -10.f, 10.f);
        ImGui::Separator();

        ImGui::Text("Physics");
        ImGui::SliderFloat3("Gravity",  glm::value_ptr(cloth.gravity), -20.f, 20.f);
        ImGui::SliderFloat("Stiffness", &cloth.springStiffness, 1.f, 2000.f);
        ImGui::SliderFloat("Bend k",    &cloth.bendStiffness,   0.f, 500.f);
        ImGui::SliderFloat("Air damp",  &cloth.airDamping,      0.f, 0.5f);
        ImGui::SliderFloat("Spring damp",&cloth.springDamping,  0.f, 1.f);
        ImGui::SliderFloat("Max stretch",&cloth.maxStretch,     1.f, 1.3f);
        ImGui::SliderInt("Constraint iters", &cloth.constraintIters, 1, 40);
        ImGui::Separator();

        ImGui::Text("Display");
        ImGui::Checkbox("Show particles", &showParticles);
        ImGui::ColorEdit3("Background", bgColor);

        ImGui::End();

        // Recompute MVP every frame so camera changes take effect immediately
        glm::mat4 view = glm::lookAt(
            cameraPos,
            glm::vec3(0.f, 0.5f, 0.f),
            glm::vec3(0.f, 1.f,  0.f)
        );
        glm::mat4 MVP = proj * view * model;

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (showParticles) {
            glUseProgram(shader);
            glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(MVP));

            // All particles — white
            glUniform3f(locColor, 1.f, 1.f, 1.f);
            glBindVertexArray(pointVAO);
            glDrawArrays(GL_POINTS, 0, (GLsizei)particles.size());

            // Pinned particles — red, drawn from their own VAO
            if (!pinnedData.empty()) {
                glUniform3f(locColor, 1.f, 0.2f, 0.2f);
                glBindVertexArray(pinnedVAO);
                glDrawArrays(GL_POINTS, 0, (GLsizei)(pinnedData.size() / 3));
            }

            glBindVertexArray(0);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &pointVAO);
    glDeleteBuffers(1, &pointVBO);
    glDeleteVertexArrays(1, &pinnedVAO);
    glDeleteBuffers(1, &pinnedVBO);
    glDeleteProgram(shader);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
