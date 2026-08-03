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

Doing this manually for every `gl*` function (there are hundreds) would be tedious. This is the problem **GLAD** solves.

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
                                     //   (must happen before glad loads pointers)
glfwSwapBuffers(window);             // swap front/back buffers (double buffering)
glfwPollEvents();                    // process OS input events
glfwTerminate();                     // cleanup
```

`glfwWindowHint` before `glfwCreateWindow` configures the context — here it requests OpenGL 3.3 Core Profile.

---

## GLAD — GL Loader

GLAD is a **function pointer loader** generated from the official OpenGL registry. It:

1. Declares a function pointer variable for every `gl*` function you want (e.g. `PFNGLCLEARPROC glad_glClear`).
2. Redefines the plain `glClear` macro to call that pointer.
3. Provides `gladLoadGLLoader()` which iterates over every pointer and fills it in by calling whatever proc-address function you hand it.

In this project that one call is:

```cpp
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
```

GLAD uses GLFW's cross-platform proc-address getter to populate every `gl*` function pointer. After this call, all `gl*` functions work.

The source lives at `thirdparty/glad/src/glad.c` and headers at `thirdparty/glad/include/glad/glad.h`.

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
    └─ gladLoadGLLoader(             ← GLAD populates all gl* function pointers
           glfwGetProcAddress)          using GLFW's cross-platform proc lookup
                │
                └─ now glClear(), glBegin(), etc. are callable
```

**Include order matters**: `glad/glad.h` must come **before** `GLFW/glfw3.h`. GLAD defines the GL types; if GLFW's GL header loads first there are conflicts. The compile definition `GLFW_INCLUDE_NONE=1` (set in `CMakeLists.txt`) prevents GLFW from auto-including any OpenGL header at all, making the order explicit and safe.

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

### `glfwSwapInterval` — VSync control

```cpp
glfwSwapInterval(1); // call after glfwMakeContextCurrent
```

Controls how many monitor refreshes the driver waits before swapping buffers:

| Value | Behaviour |
|---|---|
| `0` | No wait — swap as fast as possible (uncapped FPS, may tear) |
| `1` | Wait for 1 refresh — locks to monitor refresh rate (VSync on) |
| `2` | Wait for 2 refreshes — locks to half the refresh rate |

This project uses `1` (VSync on), which prevents screen tearing and also keeps the render loop from burning 100 % CPU spinning at thousands of FPS when drawing a static scene. Without it the loop runs uncapped and `glfwPollEvents` / `glDrawElements` execute as fast as the CPU allows.

> `glfwSwapInterval` applies to the **current context** at call time — it must be called after `glfwMakeContextCurrent`. It delegates to the platform swap-interval extension (`WGL_EXT_swap_control` on Windows, `GLX_EXT_swap_control` on Linux, `NSOpenGLCPSwapInterval` on macOS). Drivers are allowed to ignore the request (e.g. when the user forces VSync off in driver settings).

---

## Error Reporting

### `glGetError` — the primitive approach

OpenGL does not throw exceptions. Errors are recorded internally and retrieved by polling `glGetError()`, which returns one error code per call and clears it from the queue. If you don't call it often enough, errors accumulate and you don't know which call caused what.

### `GLCall` macro — per-call error checking

The approach used in this project wraps every GL call in a macro that:

1. Drains any pre-existing errors (so we only see errors from *this* call).
2. Executes the call.
3. Checks for new errors and **triggers a debugger breakpoint** at the exact callsite if any are found.

```cpp
static void GLClearError()
{
    while (glGetError() != GL_NO_ERROR); // drain all queued errors
}

static bool GLLogCall(const char *function, const char *file, int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "[OpenGL Error] (0x" << std::hex << error << "): "
                  << function << " " << file << ": " << line << std::endl;
        return false;
    }
    return true;
}
```

The `GLCall` macro ties them together:

```cpp
#define GLCall(x)    \
    GLClearError();  \
    x;               \
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))
```

- `#x` is the **stringification operator** — the preprocessor turns the raw token `glDrawElements(...)` into the string `"glDrawElements(...)"`, which is then printed in the error message.
- `__FILE__` and `__LINE__` are standard preprocessor macros that expand to the source file path and line number at the call site.
- `ASSERT` triggers a debugger breakpoint so the call stack is preserved at the point of failure.

In Release builds the macro compiles away entirely:

```cpp
#ifdef DEBUG
#define GLCall(x) GLClearError(); x; ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#else
#define GLCall(x) x   // zero overhead in release
#endif
```

### Cross-platform `ASSERT`

A breakpoint instruction is not portable — each platform has its own intrinsic:

```cpp
#if defined(_WIN32)
#define ASSERT(x) if (!(x)) __debugbreak()      // MSVC / Windows
#elif defined(__clang__)
#define ASSERT(x) if (!(x)) __builtin_debugtrap() // Clang (macOS, Linux)
#else
#include <csignal>
#define ASSERT(x) if (!(x)) raise(SIGTRAP)      // GCC / other POSIX
#endif
```

All three pause the process and hand control to the attached debugger (e.g. CodeLLDB) at the exact line of the failing `GLCall`, showing you the full call stack.

### `glDebugMessageCallback` — the modern alternative

Available since **OpenGL 4.3** (or earlier via the `GL_KHR_debug` / `GL_ARB_debug_output` extensions on 3.x drivers that support it). Instead of polling after every call, you register a callback once and the driver invokes it automatically whenever anything noteworthy happens.

**Setup** (`src/openglErrorReporting.cpp`):

```cpp
void enableReportGlErrors()
{
    glEnable(GL_DEBUG_OUTPUT);             // enable the debug output system
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // callback fires on the same thread,
                                           // in the same call stack as the GL call
    glDebugMessageCallback(glDebugOutput, nullptr); // register our function
    glDebugMessageControl(
        GL_DONT_CARE,   // source filter  — any source
        GL_DONT_CARE,   // type filter    — any type
        GL_DONT_CARE,   // severity filter — any severity
        0, nullptr,     // no specific message IDs to filter
        GL_TRUE         // enable (not suppress)
    );
}
```

**Callback signature**:

```cpp
void GLAPIENTRY glDebugOutput(
    GLenum source,       // GL_DEBUG_SOURCE_API, _SHADER_COMPILER, _WINDOW_SYSTEM, ...
    GLenum type,         // GL_DEBUG_TYPE_ERROR, _DEPRECATED_BEHAVIOR, _PERFORMANCE, ...
    unsigned int id,     // driver-assigned message ID
    GLenum severity,     // GL_DEBUG_SEVERITY_HIGH, _MEDIUM, _LOW, _NOTIFICATION
    GLsizei length,      // length of message string
    const char *message, // human-readable description from the driver
    const void *userParam // the pointer passed to glDebugMessageCallback (nullptr here)
)
```

`GL_DEBUG_OUTPUT_SYNCHRONOUS` is the important flag: without it the driver may call the callback from a background thread at an unpredictable time, making the call stack useless. With it, the callback fires inline, so a breakpoint inside it shows exactly which GL call triggered it.

**What it reports that `glGetError` / `GLCall` cannot**:

| Category | `GLCall` | `glDebugMessageCallback` |
|---|---|---|
| Invalid API usage | Yes | Yes |
| Deprecated behaviour | No | Yes (`GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR`) |
| Undefined behaviour | No | Yes (`GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR`) |
| Performance warnings | No | Yes (`GL_DEBUG_TYPE_PERFORMANCE`) |
| Shader compiler messages | No | Yes (`GL_DEBUG_SOURCE_SHADER_COMPILER`) |
| Driver-internal notices | No | Yes |

**Advantages over `GLCall`**:

- No code changes needed at every call site — one registration covers everything.
- Catches a much wider category of issues (deprecation, UB, performance hints).
- The callback receives a structured message with source, type, and severity — useful for filtering (e.g. suppress `GL_DEBUG_SEVERITY_NOTIFICATION` spam).

**Disadvantages**:

- Requires OpenGL 4.3 or the extension. Not available on all macOS hardware/drivers (macOS caps at 4.1).
- Without `GL_DEBUG_OUTPUT_SYNCHRONOUS` the call stack is broken — but enabling synchronous mode adds some overhead.
- Does not give you the file/line of your own source code — only the driver message. `GLCall` points you to the exact `GLCall(...)` line in your `.cpp`.

**Typical usage**: enable `glDebugMessageCallback` in debug builds for broad coverage, and keep `GLCall` on the most error-prone new code where you want exact source locations pinpointed.

---

## CMake Structure

| Directory | Purpose |
|---|---|
| `thirdparty/glfw-3.4` | GLFW source, built as a static lib via `add_subdirectory` |
| `thirdparty/glad` | GLAD source (one `.c` file + headers), built as a static lib |
| `src/` | Your application code |
| `include/` | Your public headers |

The main executable links against `glad` and `glfw` targets. GLFW on macOS automatically links the required system frameworks (Cocoa, OpenGL, IOKit).

---

## Drawing with OpenGL

- We upload vertex data into a **Vertex Buffer** — a block of memory on the GPU (VRAM).
- We declare **shaders** — programs that run on the GPU, telling it how to transform and color the vertex data.
- A **vertex** is not just a position. It is a blob of data of any size needed to describe one point: position, texture coordinates, normals, colors, tangents, etc. Each individual piece is called a **vertex attribute**.

---

## Modern vs Legacy OpenGL

**Legacy / Compatibility Profile** (pre-3.2, or with `GLFW_OPENGL_COMPAT_PROFILE`) allowed the immediate mode API:

```cpp
glBegin(GL_TRIANGLES);
glVertex2f(-0.5f, -0.5f);
// ...
glEnd();
```

This is simple but slow — the CPU drives every vertex one at a time.

**Core Profile** (3.3+, what this project uses) removes that API entirely. You must use buffers, VAOs, and shaders. This pushes all data to the GPU up front and lets the GPU process it in parallel. On macOS, Core Profile is **required** (`GLFW_OPENGL_FORWARD_COMPAT` + `GLFW_OPENGL_CORE_PROFILE`).

---

## Vertex Buffer Object (VBO)

A VBO is a chunk of GPU memory. You allocate it, upload your vertex data to it, and later tell the GPU how to read it.

```cpp
unsigned int buffer;
glGenBuffers(1, &buffer);       // allocate 1 buffer, store its ID in `buffer`
glBindBuffer(GL_ARRAY_BUFFER, buffer);  // "current array buffer" = this buffer
glBufferData(
    GL_ARRAY_BUFFER,            // target (same slot as above)
    6 * sizeof(float),          // size in bytes
    positions,                  // pointer to CPU-side data to upload
    GL_STATIC_DRAW              // usage hint: data uploaded once, drawn many times
);
```

`GL_STATIC_DRAW` is a **hint** to the driver about how often the data changes and in which direction:

- `STATIC` — set once, used many times
- `DYNAMIC` — changed often, used many times
- `STREAM` — set once, used a few times

---

## Describing the Data: `glVertexAttribPointer`

Uploading raw bytes is not enough — you must tell OpenGL how to interpret them as vertex attributes:

```cpp
glEnableVertexAttribArray(0);   // enable attribute slot 0
glVertexAttribPointer(
    0,                  // attribute index (matches `layout(location = 0)` in shader)
    2,                  // number of components per vertex (x, y → 2 floats)
    GL_FLOAT,           // type of each component
    GL_FALSE,           // normalize? (maps int 0-255 → float 0.0-1.0 if true)
    sizeof(float) * 2,  // stride: bytes between the start of two consecutive vertices
    (void*)0            // offset: byte offset to the first component in the buffer
);
```

For a more complex vertex layout (position + UV + normal) the stride would be the total size of one vertex and the offset would differ per attribute:

```
[  x  ][  y  ][  z  ][ u ][ v ][ nx ][ ny ][ nz ]  ← one vertex
 ↑ offset 0        ↑ offset 12           ↑ offset 20
 
stride = sizeof(float) * 8 = 32 bytes
```

---

## Vertex Array Object (VAO)

A VAO records which VBO is bound and all `glVertexAttribPointer` / `glEnableVertexAttribArray` calls made while it is bound. It acts as a bookmark — binding the VAO later restores all that state at once.

```cpp
unsigned int VAO;
glGenVertexArrays(1, &VAO);
glBindVertexArray(VAO);         // start recording
// ... bind VBO, call glVertexAttribPointer ...
// VAO now remembers: "attribute 0 → this VBO, 2 floats, stride 8, offset 0"
```

In Core Profile a VAO **must** be bound before any draw call. The driver rejects draws without one.

**VAO stores:**

- Which VBO each attribute comes from
- The format of each attribute (count, type, stride, offset)
- Which attribute indices are enabled

**VAO does NOT store:**

- The actual vertex data (that lives in the VBO)
- Shader / program state

### What is a mesh?

A mesh is one renderable piece of geometry: vertex data plus optional indices that define triangles.

In practice, a mesh usually means:

- A VBO with vertex attributes (position, normal, UV, etc.)
- Often an IBO/EBO with triangle indices
- A VAO that describes how to read the vertex data

Think of it as one "draw-ready object part".

Real-world examples:

- A player character model is often split into multiple meshes: body, helmet, weapon.
- A car can be split into chassis mesh, wheel mesh, glass mesh.
- A building can be split into wall mesh, window mesh, roof mesh.

Why split into multiple meshes instead of one giant mesh:

- Different materials/shaders per part (metal vs glass)
- Selective rendering (hide helmet, animate wheels)
- Better culling and batching decisions by the engine

### Single global VAO vs one VAO per mesh

**Compatibility Profile default (single implicit VAO)**
Older drivers and the Compatibility Profile silently provide one default VAO. All attribute calls go into it. When you draw a different mesh you must re-call `glBindBuffer` + `glVertexAttribPointer` to overwrite the stored layout before every draw call.

**Core Profile requirement + typical modern usage (one VAO per mesh)**
Core Profile has no default VAO — you must create at least one. The idiomatic pattern is one VAO per mesh:

```
Setup:
  for each mesh:
    glGenVertexArrays → glBindVertexArray(vaoA)
    glBindBuffer(GL_ARRAY_BUFFER, vboA)
    glVertexAttribPointer(...)   ← recorded into vaoA
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboA)  ← also recorded

Render loop:
  glBindVertexArray(vaoA)   ← restores vboA binding + all attribute state
  glDrawElements(...)       ← no re-specification needed

  glBindVertexArray(vaoB)   ← switch to a different mesh in one call
  glDrawElements(...)
```

This project demonstrates the difference explicitly — after setup everything is unbound (`glBindVertexArray(0)`, `glUseProgram(0)`, etc.) and the render loop re-binds only the VAO and IBO before drawing:

```cpp
// render loop — attributes do NOT need to be re-specified
GLCall(glUseProgram(shader));
GLCall(glBindVertexArray(vao));
GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo)); // IBO is NOT stored in VAO on all drivers
GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
```

> **Note:** The IBO (`GL_ELEMENT_ARRAY_BUFFER`) binding *is* part of VAO state per spec, but some older drivers do not reliably restore it. Re-binding it explicitly before drawing is a safe habit.

**Performance — single vs multiple VAOs**

`glVertexAttribPointer` is not free — it validates arguments and writes into driver-internal structures. Re-calling it before every draw is measurably slower than a single `glBindVertexArray` switch:

- The Khronos wiki's [Vertex Specification Best Practices](https://www.khronos.org/opengl/wiki/Vertex_Specification_Best_Practices) explicitly recommends one VAO per mesh and treating `glVertexAttribPointer` as setup-time work.
- NVIDIA's and AMD's "Approaching Zero Driver Overhead" (AZDO) GDC 2014/2015 talks identify redundant state changes (including attribute re-specification) as a primary CPU-side bottleneck. Slides: [NVIDIA AZDO](https://developer.nvidia.com/sites/default/files/akamai/opengl/specs/GL_ARB_multi_draw_indirect.txt) / [GDC Vault recording](https://gdcvault.com/play/1020791/).
- A practical micro-benchmark by Fabian "ryg" Giesen and Rich Geldreich's driver overhead posts confirm that driver-side validation in `glVertexAttribPointer` contributes more latency than a simple VAO bind.

In short: **one VAO per mesh** is both more readable and faster at runtime.

---

## Shaders

Shaders are written in **GLSL** (OpenGL Shading Language), compiled at runtime by the driver, and linked into a **program** object. There are several shader stages; the two mandatory ones are:

### Vertex Shader

Runs once per vertex. Receives vertex attributes as `in` variables. Must write `gl_Position` (clip-space position).

```glsl
#version 330 core

layout(location = 0) in vec4 position; // attribute index 0

void main()
{
    gl_Position = position; // pass-through; no transformation yet
}
```

`layout(location = 0)` ties this `in` variable to attribute index 0 — the same index passed to `glVertexAttribPointer` and `glEnableVertexAttribArray`. Our data is 2 floats (x, y); OpenGL auto-promotes it to `vec4` with z=0, w=1.

### Fragment Shader (Pixel Shader)

Runs once per pixel (fragment) that lies inside a rasterized primitive. Writes to one or more `out` color outputs.

```glsl
#version 330 core

layout(location = 0) out vec4 color; // output 0 → default framebuffer color

void main()
{
    color = vec4(1.0, 0.0, 0.0, 1.0); // RGBA: solid red
}
```

Fragment shaders run **far more often** than vertex shaders — a full-screen triangle at 1080p means ~1 million fragment invocations vs. 3 vertex invocations.

### Other shader types

| Type | Purpose |
|---|---|
| Geometry shader | Optional; runs after vertex stage, can generate/discard primitives |
| Tessellation shaders | Subdivide geometry on the GPU for smooth curves/surfaces |
| Compute shader | General GPU computation, no fixed rendering pipeline |

---

## Shader Compilation & Linking

Shaders go through a compile–link cycle, similar to C++:

```
vertexShader source string  ──┐
                               ├── glCreateShader / glShaderSource / glCompileShader
fragmentShader source string ──┘

compiled VS ──┐
               ├── glCreateProgram / glAttachShader / glLinkProgram / glValidateProgram
compiled FS ──┘

→ program object (an ID / handle)
```

```cpp
// --- CompileShader ---
unsigned int id = glCreateShader(type);       // GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
glShaderSource(id, 1, &src, nullptr);         // upload GLSL source
glCompileShader(id);                          // driver compiles it

// check for errors:
int result;
glGetShaderiv(id, GL_COMPILE_STATUS, &result);
if (!result) {
    // glGetShaderInfoLog gives the compiler error message
}

// --- CreateShader (program) ---
unsigned int program = glCreateProgram();
glAttachShader(program, vs);
glAttachShader(program, fs);
glLinkProgram(program);       // link stages together, resolve in/out connections
glValidateProgram(program);   // check program can run in current GL state

// compiled shader objects are like .obj files — delete after linking
glDeleteShader(vs);
glDeleteShader(fs);

glUseProgram(program);        // install program into the rendering pipeline
```

`glDeleteProgram(shader)` at the end of the program frees GPU resources.

---

## Drawing

### `glDrawArrays`

```cpp
glDrawArrays(GL_TRIANGLES, 0, 3);
// primitive type, starting vertex index, vertex count
```

OpenGL reads vertices sequentially from the bound VBO. Simple, but wasteful when vertices are shared between primitives — a rectangle needs 4 unique vertices but 6 vertex entries (two triangles × 3 vertices each), duplicating 2.

### Index Buffer Object (IBO / EBO)

An index buffer holds a list of **unsigned integers** that tell OpenGL which vertices to use and in what order, instead of repeating vertex data. This lets multiple triangles share the same vertex.

Drawing a rectangle with 4 vertices instead of 6:

```
3 ---- 2
|    / |
|   /  |
|  /   |
| /    |
0 ---- 1

positions[4]:  { (-0.5,-0.5), (0.5,-0.5), (0.5,0.5), (-0.5,0.5) }
indices[6]:    { 0,1,2,  2,3,0 }
               \_____/  \_____/
               bottom    top
               right      left
               triangle   triangle
```

Vertices 0 and 2 are shared — stored once, referenced twice. This matters at scale: a detailed mesh can reuse thousands of vertices.

```cpp
unsigned int ibo;
glGenBuffers(1, &ibo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);            // different target than VBO
glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);
```

Key differences from a VBO:

- Target is `GL_ELEMENT_ARRAY_BUFFER` (not `GL_ARRAY_BUFFER`).
- Indices **must be unsigned** — `unsigned int`, `unsigned short`, or `unsigned char`.
- The IBO binding is stored inside the VAO — binding the VAO restores the IBO too.

Drawing with an index buffer:

```cpp
glDrawElements(
    GL_TRIANGLES,       // primitive type
    6,                  // number of indices to consume
    GL_UNSIGNED_INT,    // type of each index (must match what was uploaded)
    nullptr             // offset into the IBO (nullptr = start from beginning)
);
```

The `nullptr` offset works because the IBO is already bound to `GL_ELEMENT_ARRAY_BUFFER`. OpenGL reads 6 indices, fetches the corresponding vertices from the VBO, and renders two triangles.

---

## Uniforms

Uniforms are **CPU-to-shader variables** — values you set once from C++ that every shader invocation can read. Unlike vertex attributes (which differ per vertex), a uniform has the same value across all vertices and fragments in a single draw call.

### Declaring a uniform in GLSL

```glsl
// Basic.frag
uniform vec4 u_Color; // convention: prefix with u_
```

The `uniform` keyword makes the variable visible across all shader stages in the same program.

### Setting a uniform from C++

```cpp
// 1. look up the location by name — do this ONCE after glUseProgram, store the result
GLCall(int uColorLocation = glGetUniformLocation(shader, "u_Color"));

// -1 means the uniform was not found (or was optimised away by the driver)
ASSERT(uColorLocation != -1);

// 2. upload the value — shader must be currently bound with glUseProgram
GLCall(glUniform4f(uColorLocation, 0.8f, 0.3f, 0.8f, 1.0f)); // r, g, b, a
```

**Important**: `glGetUniformLocation` queries by string name. Even if the uniform is declared in the shader, the driver may **silently remove** it if its value is never actually used in the output. Always check for `-1`.

### Updating a uniform every frame

A uniform can be changed as often as needed — including inside the render loop:

```cpp
float r = 0.0f;
float increment = 0.05f;

while (!glfwWindowShouldClose(window))
{
    // animate r between 0 and 1
    if (r > 1.0f) increment = -0.05f;
    else if (r < 0.0f) increment = 0.05f;
    r += increment;

    GLCall(glUniform4f(uColorLocation, r, 0.3f, 0.8f, 1.0f));
    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
    // ...
}
```

Each `glUniform*` call overwrites the stored value for that location. The new value takes effect on the next draw call.

### `glUniform` type suffix

The function name encodes the type being uploaded:

| Function | GLSL type | Description |
|---|---|---|
| `glUniform1f(loc, x)` | `float` | 1 float |
| `glUniform2f(loc, x, y)` | `vec2` | 2 floats |
| `glUniform3f(loc, x, y, z)` | `vec3` | 3 floats |
| `glUniform4f(loc, x, y, z, w)` | `vec4` | 4 floats |
| `glUniform1i(loc, x)` | `int` / `sampler2D` | 1 integer (used for texture slots) |
| `glUniformMatrix4fv(loc, 1, GL_FALSE, ptr)` | `mat4` | 4×4 matrix |

### Performance note

`glGetUniformLocation` is a string lookup — do it **once** at setup and cache the integer location. Calling it every frame wastes CPU cycles.

---

## Texture Mapping

Your current app uses a 2D texture (`res/textures/pizza.png`) mapped onto a rectangle.

### 1. Vertex data now includes UV coordinates

Each vertex contains 4 floats:

- `x, y` position
- `u, v` texture coordinate

Example layout used in the app:

```cpp
float vertices[] = {
    -0.5f, -0.5f, 0.0f, 0.0f,
     0.5f, -0.5f, 1.0f, 0.0f,
     0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 1.0f
};
```

`u` and `v` are normalized coordinates in $[0,1]$ across the texture image.

### 2. Vertex attribute layout uses two attributes

With your wrapper classes this becomes:

```cpp
VertexBufferLayout layout;
layout.Push<float>(2); // location 0: position
layout.Push<float>(2); // location 1: texCoord
va.Addbuffer(vb, layout);
```

Equivalent raw OpenGL idea:

```cpp
// location 0: position (x, y)
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)0);
glEnableVertexAttribArray(0);

// location 1: texCoord (u, v)
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(2 * sizeof(float)));
glEnableVertexAttribArray(1);
```

### 3. Shader pipeline for textures

In the vertex shader:

- `layout(location = 1) in vec2 texCoord;`
- pass it to fragment shader via `out vec2 v_TexCoord;`

In the fragment shader:

- receive `in vec2 v_TexCoord;`
- sample from `uniform sampler2D u_Texture;`

```glsl
vec4 texColor = texture(u_Texture, v_TexCoord);
color = texColor;
```

This means final color currently comes fully from the texture sample (not from `u_Color`).

### 4. Texture object creation and upload

Your `Texture` class does the full setup:

- loads image bytes via `stbi_load(..., 4)` forcing RGBA
- flips image vertically with `stbi_set_flip_vertically_on_load(1)`
- creates and binds `GL_TEXTURE_2D`
- sets filtering and wrapping
- uploads data using `glTexImage2D`

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

### 5. Texture unit binding and sampler uniform

Textures are bound to texture units (`GL_TEXTURE0`, `GL_TEXTURE1`, ...).

Your bind call:

```cpp
texture.Bind(); // defaults to slot 0
```

internally does:

```cpp
glActiveTexture(GL_TEXTURE0 + slot);
glBindTexture(GL_TEXTURE_2D, rendererId);
```

Then you connect sampler uniform to that slot:

```cpp
shader.SetUniform1i("u_Texture", 0);
```

Rule: sampler value is the **texture unit index**, not texture object ID.

### 6. Alpha blending for PNG textures

Because PNG may contain alpha, blending is enabled:

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

This computes final color as:

$$
C_{out} = C_{src} \cdot A_{src} + C_{dst} \cdot (1 - A_{src})
$$

Without blending, transparent pixels from the texture would be drawn as fully opaque.

### Common texture pitfalls

- Binding texture but forgetting `SetUniform1i("u_Texture", slot)`
- Wrong UV attribute offset/stride
- Image appears upside-down (missing STB vertical flip)
- Using alpha texture without enabling blending
- Setting sampler uniform before shader is bound

---

## Rendering Pipeline (simplified)

```
CPU uploads vertex data to VBO
        │
        ▼
[Vertex Shader]  ← runs once per vertex, outputs gl_Position
        │
        ▼
[Primitive Assembly]  ← groups vertices into triangles / lines / points
        │
        ▼
[Rasterization]  ← determines which pixels each triangle covers
        │
        ▼
[Fragment Shader]  ← runs once per pixel, outputs color
        │
        ▼
[Framebuffer / Screen]
```

---

## Full Modern Setup Flow (this project)

```
glfwInit() + glfwCreateWindow() + glfwMakeContextCurrent()
    │
gladLoadGLLoader()              ← all gl* functions now available
    │
glGenVertexArrays / glBindVertexArray(VAO)
    │
glGenBuffers / glBindBuffer(GL_ARRAY_BUFFER) / glBufferData   ← upload positions + UVs
    │
glVertexAttribPointer / glEnableVertexAttribArray  ← describe location 0 (pos), 1 (uv)
    │
glGenBuffers / glBindBuffer(GL_ELEMENT_ARRAY_BUFFER) / glBufferData  ← upload indices
    │                                                                    (VAO records this)
CompileShader(vertex) + CompileShader(fragment)
    │
CreateShader → glLinkProgram → glUseProgram
    │
Texture load (stb_image) → glTexImage2D → texture.Bind(slot 0)
    │
SetUniform1i("u_Texture", 0)
    │
glEnable(GL_BLEND) + glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    │
── render loop ──
    glClear(GL_COLOR_BUFFER_BIT)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr)
    glfwSwapBuffers()
    glfwPollEvents()
── end loop ──
    │
glDeleteProgram / glfwTerminate
```
