#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Cloth.h"
#include "Shader.h"
#include "Constants.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

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

// MARK: - Main
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
    Cloth cloth(CLOTH_ROWS, CLOTH_COLS, CLOTH_SPACING);

    // ── GPU buffers for cloth mesh ───────────────────────────────────
    // Cloth VAO/VBO/EBO — triangulated mesh + points
    GLuint clothVAO, clothVBO, clothEBO;
    glGenVertexArrays(1, &clothVAO);
    glGenBuffers(1, &clothVBO);
    glGenBuffers(1, &clothEBO);

    // Generate indices for the cloth mesh
    // Each quad (r, c) becomes two triangles (CCW winding)
    std::vector<unsigned int> indices;
    indices.reserve((CLOTH_ROWS - 1) * (CLOTH_COLS - 1) * 6);
    for (int r = 0; r < CLOTH_ROWS - 1; ++r) {
        for (int c = 0; c < CLOTH_COLS - 1; ++c) {
            auto idx = [](int row, int col) { return row * CLOTH_COLS + col; };
            // Triangle 1: (r, c), (r+1, c), (r, c+1)
            indices.push_back(idx(r,     c));
            indices.push_back(idx(r + 1, c));
            indices.push_back(idx(r,     c + 1));
            // Triangle 2: (r+1, c), (r+1, c+1), (r, c+1)
            indices.push_back(idx(r + 1, c));
            indices.push_back(idx(r + 1, c + 1));
            indices.push_back(idx(r,     c + 1));
        }
    }
    int indexCount = (int)indices.size();

    // Set up cloth VAO/VBO/EBO with interleaved position + normal
    glBindVertexArray(clothVAO);
    glBindBuffer(GL_ARRAY_BUFFER, clothVBO);
    // Allocate for position + normal (6 floats per vertex), GL_DYNAMIC_DRAW for per-frame updates
    glBufferData(GL_ARRAY_BUFFER, CLOTH_ROWS * CLOTH_COLS * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // Attribute 0: position (3 floats, stride=24, offset=0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Attribute 1: normal (3 floats, stride=24, offset=12)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, clothEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    // Pinned VAO/VBO — separate small buffer, never more than a handful of points
    GLuint pinnedVAO, pinnedVBO;
    glGenVertexArrays(1, &pinnedVAO);
    glGenBuffers(1, &pinnedVBO);
    glBindVertexArray(pinnedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pinnedVBO);
    glBufferData(GL_ARRAY_BUFFER, CLOTH_ROWS * CLOTH_COLS * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Shaders ──────────────────────────────────────────────────────────────
    Shader meshShader("mesh.vert", "mesh.frag");   // Phong shading for mesh
    Shader particleShader("cloth.vert", "cloth.frag"); // flat color for particles
    if (meshShader.ID == 0 || particleShader.ID == 0) {
        std::cerr << "Failed to load shaders!\n";
        return -1;
    }
    std::cout << "Shaders initialized successfully\n";

    // ── Camera / projection ──────────────────────────────────────────────────
    // Simple fixed camera looking at the cloth from a slight angle.
    // In Phase 4 we'll add proper camera controls.
    glm::mat4 proj = glm::perspective(glm::radians(CAMERA_FOV),
                                      (float)WINDOW_WIDTH / WINDOW_HEIGHT,
                                      CAMERA_NEAR, CAMERA_FAR);

    glm::vec3 cameraPos = DEFAULT_CAMERA_POS;
    glm::vec3 cameraTarget = DEFAULT_CAMERA_TARGET;
    glm::vec3 cameraUp = DEFAULT_CAMERA_UP;
    glm::vec3 lightPos = DEFAULT_LIGHT_POS;

    glm::mat4 view = glm::lookAt(cameraPos,cameraTarget, cameraUp);
    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 MVP   = proj * view * model;

    // ── Simulation state ─────────────────────────────────────────────────────
    bool  simRunning  = true;
    bool  showMesh    = true;
    bool  wireframe   = false;
    bool  showParticles = false;
    float particleSize = DEFAULT_POINT_SIZE;
    float bgColor[3]  = { 0.1f, 0.1f, 0.1f };
    float deltaTime          = DEFAULT_DELTA_TIME;

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
            cloth.update(deltaTime);

        // ── Upload particle positions and compute normals to GPU ────────────────
        const auto& particles = cloth.getParticles();

        // Compute vertex normals by accumulating face normals
        std::vector<glm::vec3> normals(particles.size(), glm::vec3(0.f));
        for (int i = 0; i < indexCount; i += 3) {
            auto& p0 = particles[indices[i]];
            auto& p1 = particles[indices[i+1]];
            auto& p2 = particles[indices[i+2]];
            glm::vec3 n = glm::cross(p1.position - p0.position, p2.position - p0.position);
            normals[indices[i]]   += n;
            normals[indices[i+1]] += n;
            normals[indices[i+2]] += n;
        }
        for (auto& n : normals) {
            if (glm::length(n) > 0.0001f)
                n = glm::normalize(n);
        }

        // Build interleaved position + normal data for mesh rendering
        std::vector<float> meshData;
        meshData.reserve(particles.size() * 6);
        for (int i = 0; i < (int)particles.size(); ++i) {
            meshData.push_back(particles[i].position.x);
            meshData.push_back(particles[i].position.y);
            meshData.push_back(particles[i].position.z);
            meshData.push_back(normals[i].x);
            meshData.push_back(normals[i].y);
            meshData.push_back(normals[i].z);
        }

        glBindBuffer(GL_ARRAY_BUFFER, clothVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, meshData.size() * sizeof(float), meshData.data());

        // Collect pinned particles for separate rendering
        std::vector<float> pinnedData;
        for (const auto& p : particles) {
            if (p.pinned) {
                pinnedData.push_back(p.position.x);
                pinnedData.push_back(p.position.y);
                pinnedData.push_back(p.position.z);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, pinnedVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, pinnedData.size() * sizeof(float), pinnedData.data());

        // ── ImGui ─────────────────────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Cloth Simulation");

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Separator();

        ImGui::Text("Simulation");
        ImGui::Checkbox("Running", &simRunning);
        if (ImGui::Button("Reset")) cloth.reset();
        ImGui::SliderFloat("Delta Time (ms)", &deltaTime, 0.001f, 0.033f, "%.4f");
        ImGui::Separator();

        ImGui::Text("Camera");
        ImGui::SliderFloat3("Camera pos", glm::value_ptr(cameraPos), -20.f, 20.f);
        ImGui::Separator();

        ImGui::Text("Physics");
        ImGui::SliderFloat3("Gravity", glm::value_ptr(cloth.gravity), -20.f, 20.f);
        ImGui::SliderFloat("Stiffness", &cloth.springStiffness, 1.f, 2000.f);
        ImGui::SliderFloat("Bend k", &cloth.bendStiffness, 0.f, 500.f);
        ImGui::SliderFloat("Air damp", &cloth.airDamping, 0.f, 0.5f);
        ImGui::SliderFloat("Spring damp", &cloth.springDamping, 0.f, 1.f);
        ImGui::SliderFloat("Max stretch", &cloth.maxStretch, 1.f, 1.3f);
        ImGui::SliderInt("Constraint iters", &cloth.constraintIters, 1, 40);
        ImGui::Separator();

        ImGui::Text("Display");
        ImGui::Checkbox("Show mesh", &showMesh);
        ImGui::BeginDisabled(!showMesh);
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::EndDisabled();
        ImGui::Checkbox("Show particles", &showParticles);
        ImGui::SliderFloat("Particle size", &particleSize, 0.5f, 15.f);
        ImGui::ColorEdit3("Background", bgColor);
        ImGui::SliderFloat3("Light pos", glm::value_ptr(lightPos), -10.f, 10.f);

        ImGui::End();

        // Recompute MVP every frame so camera changes take effect immediately
        glm::mat4 view = glm::lookAt(cameraPos,cameraTarget, cameraUp);
        glm::mat4 MVP = proj * view * model;

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render mesh with Phong shading
        if (showMesh) {
            meshShader.use();
            meshShader.setMat4("uMVP", MVP);
            meshShader.setMat4("uModel", model);
            meshShader.setVec3("uColor", DEFAULT_CLOTH_COLOR);
            meshShader.setVec3("uLightPos", lightPos);
            meshShader.setVec3("uViewPos", cameraPos);
            if (wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glBindVertexArray(clothVAO);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            if (wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Render particles
        if (showParticles) {
            particleShader.use();
            particleShader.setMat4("uMVP", MVP);
            particleShader.setFloat("uPointSize", particleSize);
            // All particles — white
            particleShader.setVec3("uColor", glm::vec3(1.f, 1.f, 1.f));
            glBindVertexArray(clothVAO);
            glDrawArrays(GL_POINTS, 0, (GLsizei)particles.size());

            // Pinned particles — red, drawn from their own VAO
            if (!pinnedData.empty()) {
                particleShader.setVec3("uColor", glm::vec3(1.f, 0.2f, 0.2f));
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
    glDeleteVertexArrays(1, &clothVAO);
    glDeleteBuffers(1, &clothVBO);
    glDeleteBuffers(1, &clothEBO);
    glDeleteVertexArrays(1, &pinnedVAO);
    glDeleteBuffers(1, &pinnedVBO);
    // Shader will be cleaned up by its destructor

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
