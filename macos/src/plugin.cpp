// Entry point of the macOS plugin.
//
// The game's launcher script (Contents/MacOS/reVC) loads this library through
// DYLD_INSERT_LIBRARIES. From there everything goes through dyld interposing
// of the GLFW functions the game imports, so no game code is patched:
//
//  - glfwSwapBuffers: runs the trainer each frame and draws the menu on top of
//    the finished game frame.
//  - input callbacks/polls: forwarded to the menu while it's open, and hidden
//    from the game so it doesn't drive or shoot while you click around.
//  - cursor mode: the game wants a captured cursor; the menu needs a visible
//    one. The game's requested mode is remembered and restored on close.

#include <OpenGL/gl3.h>

#include <GLFW/glfw3.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

#include "data.h"
#include "game.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "log.h"
#include "trainer.h"

#define DYLD_INTERPOSE(replacement, replacee)                                                                          \
    __attribute__((used)) static struct                                                                                \
    {                                                                                                                  \
        const void *replacement_;                                                                                      \
        const void *replacee_;                                                                                         \
    } interpose_##replacee __attribute__((section("__DATA,__interpose"))) = {(const void *)&replacement,              \
                                                                             (const void *)&replacee};

void Log(const char *fmt, ...)
{
    char line[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    fprintf(stderr, "[HammerillTrainerVC] %s\n", line);
    if (FILE *f = fopen("HammerillTrainerVC.log", "a"))
    {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
}

namespace
{
GLFWwindow *gameWindow = nullptr;
bool imguiReady = false;
bool menuOpen = false;
bool leftCtrlDown = false;
bool padChordDown = false;

// Callbacks the game registered; ours sit in front of them.
GLFWkeyfun gameKeyCallback = nullptr;
GLFWcursorposfun gameCursorPosCallback = nullptr;
GLFWscrollfun gameScrollCallback = nullptr;
GLFWwindowfocusfun gameFocusCallback = nullptr;
GLFWcursorenterfun gameCursorEnterCallback = nullptr;

// The game reads mouse movement as cursor position deltas. While the menu is
// open it sees a frozen position; afterwards an offset hides the distance the
// real cursor travelled, so the camera doesn't jump.
int gameCursorMode = GLFW_CURSOR_NORMAL;
double frozenX = 0, frozenY = 0;
double offsetX = 0, offsetY = 0;

std::string gameError;

void OpenMenu()
{
    double x, y;
    glfwGetCursorPos(gameWindow, &x, &y);
    frozenX = x - offsetX;
    frozenY = y - offsetY;
    menuOpen = true;
    glfwSetInputMode(gameWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (imguiReady)
    {
        glfwGetCursorPos(gameWindow, &x, &y);
        ImGui_ImplGlfw_CursorPosCallback(gameWindow, x, y);
    }
}

void CloseMenu()
{
    menuOpen = false;
    glfwSetInputMode(gameWindow, GLFW_CURSOR, gameCursorMode);
    double x, y;
    glfwGetCursorPos(gameWindow, &x, &y);
    offsetX = x - frozenX;
    offsetY = y - frozenY;
    if (imguiReady)
    {
        ImGui::GetIO().ClearInputKeys();
        ImGui::GetIO().ClearInputMouse();
    }
}

void ToggleMenu()
{
    if (menuOpen)
        CloseMenu();
    else
        OpenMenu();
}

// --- Our callbacks, installed on the game window when it's created ----------

void OnKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_LEFT_CONTROL)
        leftCtrlDown = action != GLFW_RELEASE;

    if (action == GLFW_RELEASE)
        trainer::OnKey(key, false);

    if (key == GLFW_KEY_T && action == GLFW_PRESS && leftCtrlDown)
    {
        ToggleMenu();
        return;
    }

    if (menuOpen)
    {
        if (imguiReady)
            ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && imguiReady && !ImGui::GetIO().WantTextInput)
            CloseMenu();
        // Let releases through so keys held when the menu opened don't stick.
        if (action == GLFW_RELEASE && gameKeyCallback)
            gameKeyCallback(window, key, scancode, action, mods);
        return;
    }
    if (action == GLFW_PRESS)
        trainer::OnKey(key, true);
    if (gameKeyCallback)
        gameKeyCallback(window, key, scancode, action, mods);
}

void OnChar(GLFWwindow *window, unsigned int codepoint)
{
    if (menuOpen && imguiReady)
        ImGui_ImplGlfw_CharCallback(window, codepoint);
}

void OnMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    if (menuOpen && imguiReady)
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void OnCursorPos(GLFWwindow *window, double x, double y)
{
    if (menuOpen)
    {
        if (imguiReady)
            ImGui_ImplGlfw_CursorPosCallback(window, x, y);
        return;
    }
    if (gameCursorPosCallback)
        gameCursorPosCallback(window, x - offsetX, y - offsetY);
}

void OnScroll(GLFWwindow *window, double dx, double dy)
{
    if (menuOpen)
    {
        if (imguiReady)
            ImGui_ImplGlfw_ScrollCallback(window, dx, dy);
        return;
    }
    if (gameScrollCallback)
        gameScrollCallback(window, dx, dy);
}

void OnFocus(GLFWwindow *window, int focused)
{
    if (imguiReady)
        ImGui_ImplGlfw_WindowFocusCallback(window, focused);
    if (gameFocusCallback)
        gameFocusCallback(window, focused);
}

void OnCursorEnter(GLFWwindow *window, int entered)
{
    if (imguiReady)
        ImGui_ImplGlfw_CursorEnterCallback(window, entered);
    if (gameCursorEnterCallback)
        gameCursorEnterCallback(window, entered);
}

// --- Menu lifecycle ---------------------------------------------------------

void InitImGui()
{
    if (!ImGui::GetCurrentContext())
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        struct stat st;
        io.IniFilename = stat("userfiles", &st) == 0 ? "userfiles/HammerillTrainerVC.ini" : nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad |
                          ImGuiConfigFlags_NoMouseCursorChange;

        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
        style.WindowPadding = ImVec2(10, 10);

        // Rasterise the font at the display's pixel density so it's sharp on
        // Retina screens, then scale it back to window points.
        float scaleX = 1.0f, scaleY = 1.0f;
        glfwGetWindowContentScale(gameWindow, &scaleX, &scaleY);
        std::string_view font = data::File("text.ttf");
        if (!font.empty())
        {
            ImFontConfig config;
            config.FontDataOwnedByAtlas = false;
            io.Fonts->AddFontFromMemoryTTF(const_cast<char *>(font.data()), static_cast<int>(font.size()), 17.0f * scaleX,
                                           &config);
            io.FontGlobalScale = 1.0f / scaleX;
        }
    }

    // librw creates a 3.3 core context when it can, else falls back to 2.1.
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    const char *glsl = version && version[0] >= '3' ? "#version 150" : "#version 120";
    ImGui_ImplGlfw_InitForOpenGL(gameWindow, false);
    ImGui_ImplOpenGL3_Init(glsl);
    imguiReady = true;
    Log("menu ready (OpenGL %s)", version ? version : "?");
}

void ShutdownImGui()
{
    if (!imguiReady)
        return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    imguiReady = false;
}

// X + A on Xbox layout, Square + Cross on PlayStation, as in the Windows trainer.
void PollGamepadToggle()
{
    bool down = false;
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
    {
        GLFWgamepadstate state;
        if (glfwJoystickIsGamepad(jid) && glfwGetGamepadState(jid, &state))
            down |= state.buttons[GLFW_GAMEPAD_BUTTON_X] && state.buttons[GLFW_GAMEPAD_BUTTON_A];
    }
    if (down && !padChordDown)
        ToggleMenu();
    padChordDown = down;
}

void Frame()
{
    // Done here rather than in OnLoad(): constructors run before the C++
    // globals of the other source files are initialised.
    static bool started = false;
    if (!started)
    {
        started = true;
        if (game::Init(gameError))
            Log("game build recognised");
        else
            Log("%s", gameError.c_str());
        trainer::Init(gameError);
    }
    if (!imguiReady)
        InitImGui();

    PollGamepadToggle();
    trainer::Update();
    if (!menuOpen)
        return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    bool open = true;
    trainer::Draw(&open);
    ImGui::Render();

    GLint drawFramebuffer, readFramebuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);

    if (!open)
        CloseMenu();
}

// --- Interposed GLFW functions ----------------------------------------------
// Calls to glfw* from inside this library reach the real functions; dyld only
// redirects the game's calls.

GLFWwindow *Hook_glfwCreateWindow(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
{
    GLFWwindow *window = glfwCreateWindow(width, height, title, monitor, share);
    if (window && !gameWindow)
    {
        gameWindow = window;
        glfwSetKeyCallback(window, OnKey);
        glfwSetCharCallback(window, OnChar);
        glfwSetMouseButtonCallback(window, OnMouseButton);
        glfwSetCursorPosCallback(window, OnCursorPos);
        glfwSetScrollCallback(window, OnScroll);
        glfwSetWindowFocusCallback(window, OnFocus);
        glfwSetCursorEnterCallback(window, OnCursorEnter);
    }
    return window;
}
DYLD_INTERPOSE(Hook_glfwCreateWindow, glfwCreateWindow)

void Hook_glfwDestroyWindow(GLFWwindow *window)
{
    if (window && window == gameWindow)
    {
        ShutdownImGui();
        gameWindow = nullptr;
        menuOpen = false;
        gameKeyCallback = nullptr;
        gameCursorPosCallback = nullptr;
        gameScrollCallback = nullptr;
        gameFocusCallback = nullptr;
        gameCursorEnterCallback = nullptr;
    }
    glfwDestroyWindow(window);
}
DYLD_INTERPOSE(Hook_glfwDestroyWindow, glfwDestroyWindow)

void Hook_glfwSwapBuffers(GLFWwindow *window)
{
    if (window && window == gameWindow)
        Frame();
    glfwSwapBuffers(window);
}
DYLD_INTERPOSE(Hook_glfwSwapBuffers, glfwSwapBuffers)

// Registers the game's callback behind ours. Returns what the game had before,
// as the real setter would.
#define CHAIN_CALLBACK(Type, Setter, Stored)                                                                           \
    Type Hook_##Setter(GLFWwindow *window, Type callback)                                                              \
    {                                                                                                                  \
        if (window != gameWindow)                                                                                      \
            return Setter(window, callback);                                                                           \
        Type previous = Stored;                                                                                        \
        Stored = callback;                                                                                             \
        return previous;                                                                                               \
    }                                                                                                                  \
    DYLD_INTERPOSE(Hook_##Setter, Setter)

CHAIN_CALLBACK(GLFWkeyfun, glfwSetKeyCallback, gameKeyCallback)
CHAIN_CALLBACK(GLFWcursorposfun, glfwSetCursorPosCallback, gameCursorPosCallback)
CHAIN_CALLBACK(GLFWscrollfun, glfwSetScrollCallback, gameScrollCallback)
CHAIN_CALLBACK(GLFWwindowfocusfun, glfwSetWindowFocusCallback, gameFocusCallback)
CHAIN_CALLBACK(GLFWcursorenterfun, glfwSetCursorEnterCallback, gameCursorEnterCallback)

void Hook_glfwSetInputMode(GLFWwindow *window, int mode, int value)
{
    if (window == gameWindow && mode == GLFW_CURSOR)
    {
        // The game switching cursor modes (e.g. for its pause menu) resets
        // the relationship between real and reported positions.
        gameCursorMode = value;
        offsetX = offsetY = 0;
        if (menuOpen)
            return;
    }
    glfwSetInputMode(window, mode, value);
}
DYLD_INTERPOSE(Hook_glfwSetInputMode, glfwSetInputMode)

void Hook_glfwGetCursorPos(GLFWwindow *window, double *x, double *y)
{
    double realX, realY;
    if (window == gameWindow && menuOpen)
    {
        realX = frozenX;
        realY = frozenY;
    }
    else
    {
        glfwGetCursorPos(window, &realX, &realY);
        if (window == gameWindow)
        {
            realX -= offsetX;
            realY -= offsetY;
        }
    }
    if (x)
        *x = realX;
    if (y)
        *y = realY;
}
DYLD_INTERPOSE(Hook_glfwGetCursorPos, glfwGetCursorPos)

void Hook_glfwSetCursorPos(GLFWwindow *window, double x, double y)
{
    if (window == gameWindow && menuOpen)
    {
        frozenX = x;
        frozenY = y;
        return;
    }
    if (window == gameWindow)
        glfwSetCursorPos(window, x + offsetX, y + offsetY);
    else
        glfwSetCursorPos(window, x, y);
}
DYLD_INTERPOSE(Hook_glfwSetCursorPos, glfwSetCursorPos)

int Hook_glfwGetMouseButton(GLFWwindow *window, int button)
{
    if (window == gameWindow && menuOpen)
        return GLFW_RELEASE;
    return glfwGetMouseButton(window, button);
}
DYLD_INTERPOSE(Hook_glfwGetMouseButton, glfwGetMouseButton)

int Hook_glfwGetGamepadState(int jid, GLFWgamepadstate *state)
{
    int result = glfwGetGamepadState(jid, state);
    if (result && menuOpen)
    {
        memset(state, 0, sizeof(*state));
        state->axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = -1.0f; // triggers rest at -1
        state->axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.0f;
    }
    return result;
}
DYLD_INTERPOSE(Hook_glfwGetGamepadState, glfwGetGamepadState)

const unsigned char *Hook_glfwGetJoystickButtons(int jid, int *count)
{
    static unsigned char released[128];
    const unsigned char *buttons = glfwGetJoystickButtons(jid, count);
    if (buttons && menuOpen)
    {
        if (*count > (int)sizeof(released))
            *count = sizeof(released);
        return released;
    }
    return buttons;
}
DYLD_INTERPOSE(Hook_glfwGetJoystickButtons, glfwGetJoystickButtons)

const float *Hook_glfwGetJoystickAxes(int jid, int *count)
{
    static float centred[32];
    const float *axes = glfwGetJoystickAxes(jid, count);
    if (axes && menuOpen)
    {
        if (*count > (int)(sizeof(centred) / sizeof(centred[0])))
            *count = sizeof(centred) / sizeof(centred[0]);
        return centred;
    }
    return axes;
}
DYLD_INTERPOSE(Hook_glfwGetJoystickAxes, glfwGetJoystickAxes)

__attribute__((constructor)) void OnLoad()
{
    // Don't pass the injection on to anything the game launches.
    unsetenv("DYLD_INSERT_LIBRARIES");

    Log("Hammerill Trainer VC (macOS) %s loaded", HAMMERILL_TRAINER_VC_MACOS_VERSION);
}
} // namespace
