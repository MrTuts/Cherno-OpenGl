# Knowledge

## OpenGL

OpenGL is a **specification** (not a library) created by the Khronos Group. It defines what functions exist, what they take, and what they should do. The actual **implementation** lives inside the GPU driver installed on the machine (e.g. `nvoglv64.dll` for NVIDIA on Windows, or Metal translation layers on macOS). This means:

- You don't link against a single "OpenGL.lib" that has all the code.
- The functions are resolved at **runtime** from the driver.
- Different GPU vendors may have slightly different behavior or extensions.

OpenGL is a **state machine**. You set state (bind a buffer, set a shader, set clear color), then issue draw calls that operate on whatever state is currently active. Functions like `glBindBuffer`, `glUseProgram`, `glClearColor` all modify this global state.

---

## The Problem: Loading Function Pointers

On most platforms, OpenGL function pointers beyond the very basics (v1.1) are **not exposed** in any standard import library. They live inside the driver and must be retrieved at runtime using platform-specific calls:

- Windows: `wglGetProcAddress`
- Linux: `glXGetProcAddress`
- macOS: `NSGLGetProcAddress` (macOS deprecated OpenGL in favor of Metal in 2018, but it still works)

Doing this manually for every `gl*` function (there are hundreds) would be tedious. This is the problem **GLEW** solves.

---

## GLFW — GL FrameWork

GLFW is a cross-platform library that handles everything that OpenGL itself does **not**:

- **Window creation** — OpenGL has no concept of a window; GLFW abstracts Win32/X11/Cocoa/Wayland.
- **OpenGL context creation** — A context is the object that holds all OpenGL state. It is OS-specific. You must have a context before making any `gl*` calls. GLFW creates one tied to the window.
- **Input handling** — keyboard, mouse, gamepad events.
- **Cross-platform `glfwGetProcAddress`** — wraps the platform-specific calls above into one portable function.

Key GLFW calls in this project (`src/Application.cpp`):

```cpp
glfwInit();                          // initialize GLFW
glfwCreateWindow(640, 480, ...);     // create OS window
glfwMakeContextCurrent(window);      // make the GL context active on this thread
                                     //   (must happen before GLEW initializes pointers)
glfwSwapBuffers(window);             // swap front/back buffers (double buffering)
glfwPollEvents();                    // process OS input events
glfwTerminate();                     // cleanup
```

`glfwWindowHint` before `glfwCreateWindow` configures the context — here it requests OpenGL 3.3 Core Profile.

---

## GLEW — GL Loader

GLEW is a **function pointer loader** that initializes OpenGL extension/function entry points at runtime. It:

1. Exposes OpenGL symbols through `glew.h` (loaded from runtime driver function pointers).
2. Initializes those symbols with `glewInit()` after a context exists.
3. Uses GLFW's context/proc setup so the initialized `gl*` calls are valid on the current context.

In this project the initialization is:

```cpp
glewExperimental = GL_TRUE;
glewInit();
```

After `glewInit()`, `gl*` functions are ready for use.

The source lives at `thirdparty/glew-2.3.1/src/glew.c` and headers at `thirdparty/glew-2.3.1/include/GL/glew.h`.

---

## How They Connect — Startup Sequence

```
[Your code]
    │
    ├─ glfwInit()                    ← GLFW sets up platform internals
    ├─ glfwWindowHint(...)           ← configure: OpenGL 3.3, Core Profile
    ├─ glfwCreateWindow(...)         ← OS window + OpenGL context created
    ├─ glfwMakeContextCurrent(win)   ← context bound to this thread
    │
    └─ glewInit()                    ← GLEW populates gl* function pointers
                │
                └─ now glClear(), glBegin(), etc. are callable
```

**Include order matters**: `GL/glew.h` must come **before** `GLFW/glfw3.h`. The compile definition `GLFW_INCLUDE_NONE=1` (set in `CMakeLists.txt`) prevents GLFW from auto-including any OpenGL header at all, making the order explicit and safe.

---

## The Render Loop

```cpp
while (!glfwWindowShouldClose(window))
{
    glClear(GL_COLOR_BUFFER_BIT);  // clear back buffer
    // ... draw calls ...
    glfwSwapBuffers(window);       // show back buffer (avoids tearing)
    glfwPollEvents();              // handle keyboard/mouse/close events
}
```

Double buffering: you always draw to the **back buffer**; `glfwSwapBuffers` flips it to the screen atomically so you never see a half-drawn frame.

---

## Error Reporting

`include/openglErrorReporting.h` + `src/openglErrorReporting.cpp` wrap OpenGL's debug output extension (`GL_KHR_debug`). Calling `enableReportGlErrors()` registers `glDebugOutput` as a callback. The driver then calls it whenever an error, warning, or performance issue occurs, printing the source, type, and message. This is much better than manually calling `glGetError()` after every draw call.

---

## CMake Structure

| Directory | Purpose |
|---|---|
| `thirdparty/glfw-3.4` | GLFW source, built as a static lib via `add_subdirectory` |
| `thirdparty/glew-2.3.1` | GLEW source (one `.c` file + headers), built as a static lib |
| `src/` | Your application code |
| `include/` | Your public headers |

The main executable links against `glew` and `glfw` targets. GLFW on macOS automatically links the required system frameworks (Cocoa, OpenGL, IOKit).