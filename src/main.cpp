#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>

// ── Constants ────────────────────────────────────────────────────────────────
static constexpr int   WINDOW_WIDTH  = 1280;
static constexpr int   WINDOW_HEIGHT = 720;
static constexpr char  WINDOW_TITLE[] = "Cloth Simulation";
static constexpr char  GLSL_VERSION[] = "#version 330 core";

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

    // OpenGL 3.3 Core — required on macOS
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required on macOS

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // ── Init GLAD ────────────────────────────────────────────────────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

    // ── Init ImGui ───────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    // ── Render loop ──────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // -- ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Placeholder control panel
        ImGui::Begin("Cloth Simulation");
        ImGui::Text("Phase 1 — window bootstrap");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::End();

        // -- Clear
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // -- Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
