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

### Double buffering — front and back color buffers

The default framebuffer has two color buffers:

- **Front buffer** — currently displayed on screen; the OS reads this to composite the window.
- **Back buffer** — the active render target; all `gl*` draw calls write here.

`glfwSwapBuffers` swaps their roles: the back buffer becomes the front (is displayed) and the former front becomes the new back (cleared and ready for the next frame). The swap happens atomically with respect to the vertical sync signal, preventing screen tearing.

`glClear(GL_COLOR_BUFFER_BIT)` clears the **back buffer** — the one you are about to draw into. Omitting it leaves stale content from the last time that physical buffer was the back buffer, which was **two frames ago** (not one), because the two buffers alternate.

**Observing the swap without clearing**: if you push data incrementally into a VBO and draw without clearing, you see two alternating partial states. Buffer A receives frames 1, 3, 5 … and buffer B receives frames 2, 4, 6 … — each fills up independently at half the update rate. The scene `SceneColorBuffer` exploits this to make the double-buffer mechanics visible: it adds one triangle per frame and skips `glClear`, so you observe each buffer catching up separately until both hold the complete picture.

### VBO vs. color buffers — a critical distinction

A **VBO** (Vertex Buffer Object) and the **front/back color buffers** are completely separate pieces of GPU memory with different purposes:

| | VBO | Color buffer |
|---|---|---|
| Contains | Vertex data (positions, UVs, etc.) | Rendered pixel output (RGBA per pixel) |
| Double-buffered? | **No** — one copy shared by all frames | **Yes** — front and back alternate each frame |
| Written by | `glBufferData` / `glBufferSubData` (CPU→GPU upload) | `glDrawArrays` / `glDrawElements` (GPU rasterization) |
| Read by | Vertex shader during a draw call | OS compositor (front), GPU rasterizer (back) |

`glBufferSubData` updates the VBO immediately and **both** color buffer passes can read that change. It does **not** write to whichever color buffer is currently the back buffer.

`glDrawArrays` / `glDrawElements` is what **reads** the VBO and **writes pixels** into the current back color buffer.

### Why `OnRender` must fire only once per triangle addition

This is about `SceneColorBuffer.cpp`

If `OnRender` fires every frame (which it does by default in the render loop), the same triangle gets rasterized into **both** physical buffers during the wait period between additions:

```
Frame N   (back = B1): OnUpdate adds triangle → OnRender draws it into B1. Swap.
Frame N+1 (back = B2): OnUpdate does nothing  → OnRender draws same triangle into B2. Swap.
Frame N+2 (back = B1): same.  Both B1 and B2 now contain triangle N before triangle N+1 is added.
```

The result: triangles "add up" identically on both buffers — no flickering.

The fix is a `m_TriangleAdded` flag: set it in `OnUpdate` when a triangle is uploaded, and in `OnRender` skip the draw call and clear the flag. Each triangle then lands in exactly **one** physical buffer. The other buffer is untouched during the wait, so the two buffers diverge and flickering is visible.

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
#version 410 core

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
#version 410 core

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

### Common texture pitfalls

- Binding texture but forgetting `SetUniform1i("u_Texture", slot)`
- Wrong UV attribute offset/stride
- Image appears upside-down (missing STB vertical flip)
- Using alpha texture without enabling blending
- Setting sampler uniform before shader is bound

---

## Depth Buffer

The depth buffer (z-buffer) is a 2D array of floating-point values, one per pixel, that tracks the depth of the closest fragment written to each pixel so far. It allows OpenGL to automatically discard fragments that are behind already-drawn geometry.

### Enabling depth testing

```cpp
glEnable(GL_DEPTH_TEST);
```

This comes with a small performance cost. When enabled, every `glDrawElements` / `glDrawArrays` call performs a per-fragment depth test before writing to the framebuffer.

### How the depth test works

- The buffer is initialized to `1.0` (furthest from camera) by `glClear(GL_DEPTH_BUFFER_BIT)`.
- The z-axis convention: `+1.0` is furthest from the camera; `-1.0` is closest. Only `z ∈ [-1, 1]` (NDC) is rendered; anything outside fails and is clipped.
- Default test function is `GL_LESS`: the new fragment is written **only if** its depth is strictly less than the stored depth. On equal depth the existing fragment wins.
- Like other vertex attributes, the `z` value is **interpolated** across the triangle between its vertices.

```
if (newDepth < storedDepth)  →  write fragment + update depth buffer
else                         →  discard fragment
```

### Clearing the depth buffer each frame

```cpp
glClear(GL_DEPTH_BUFFER_BIT);  // resets all depth values to 1.0 (furthest away)
```

Both bit flags can be combined:

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

### Depth buffer and alpha blending

The depth buffer alone cannot handle transparent objects correctly — a transparent fragment still writes to the depth buffer and occludes geometry behind it. The standard workaround is to draw opaque objects first (with depth write enabled), then draw transparent objects sorted **back to front** (painter's algorithm), optionally with depth writes disabled (`glDepthMask(GL_FALSE)`) during the transparent pass.

---

## Alpha Blending

OpenGL does not blend by default — every fragment written to the framebuffer simply replaces what was there. Blending must be explicitly enabled and configured.

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

### `glBlendFunc` — source and destination factors

`glBlendFunc(sfactor, dfactor)` tells OpenGL what to multiply the **source** color (the fragment being drawn) and the **destination** color (what is already in the framebuffer) by before combining them.

The general formula per channel is determined by `glBlendEquation`, which defaults to `GL_FUNC_ADD`:

$$
C_{out} = C_{src} \cdot f_{src} + C_{dst} \cdot f_{dst}
$$

**Default** (no blending effect, source always wins):

```cpp
glBlendFunc(GL_ONE, GL_ZERO);
// R_out = R_src * 1 + R_dst * 0 = R_src
```

**Standard transparency** (used in this project):

```cpp
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// R_out = R_src * alpha + R_dst * (1 - alpha)
```

What this means for different alpha values:

| Alpha | Result |
|---|---|
| `0.0` | `R_out = R_dst` — fragment fully transparent, destination shows through |
| `1.0` | `R_out = R_src` — fragment fully opaque, destination replaced |
| `0.1` | `R_out = R_src * 0.1 + R_dst * 0.9` — mostly destination, slight tint of source |

### `glBlendEquation` — how src and dst are combined

The equation operator between the two terms. Default is `GL_FUNC_ADD` (addition). Other modes:

| Mode | Formula |
|---|---|
| `GL_FUNC_ADD` | `C_src * f + C_dst * f` (default) |
| `GL_FUNC_SUBTRACT` | `C_src * f - C_dst * f` |
| `GL_FUNC_REVERSE_SUBTRACT` | `C_dst * f - C_src * f` |
| `GL_MIN` | `min(C_src, C_dst)` — factors ignored |
| `GL_MAX` | `max(C_src, C_dst)` — factors ignored |

### Important: draw order matters

With standard `GL_SRC_ALPHA` blending, transparent objects must be drawn **back to front** (painter's algorithm). A transparent fragment blends against whatever is in the framebuffer at that moment — if opaque geometry behind it hasn't been drawn yet, blending produces the wrong result. The depth buffer alone cannot solve this because transparent fragments still write depth and occlude geometry behind them.

---

## GLM — OpenGL Mathematics Library

GLM is a header-only C++ math library designed to mirror GLSL syntax. It provides vector types (`glm::vec2`, `glm::vec3`, `glm::vec4`), matrix types (`glm::mat4`), and transform helpers (`glm::translate`, `glm::ortho`, etc.).

It does **not** call any OpenGL functions. Its role is pure CPU-side math; the results are uploaded to shaders as uniforms.

```cpp
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"  // glm::translate, glm::ortho, etc.
```

---

## MVP — Model, View, Projection

The MVP transform chain converts vertex positions from **object (local) space** all the way to **clip space** (what OpenGL outputs to the screen). It is applied in the vertex shader:

```glsl
uniform mat4 u_MVP;

void main() {
    gl_Position = u_MVP * position;
}
```

The three matrices are multiplied on the CPU once per frame and uploaded as a single `mat4` uniform.

### Model Matrix

Transforms vertex positions from **local/object space** into **world space**. Encodes where the object sits in the world — its position, rotation, and scale.

```cpp
// move the object 200px right, 200px up
glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 200.0f, 0.0f));
```

`glm::mat4(1.0f)` is the **identity matrix** — a starting point that applies no transformation.

### View Matrix

Transforms vertex positions from **world space** into **camera (eye) space**. Represents where and how the camera is positioned. A camera "moving right" is mathematically equivalent to the entire world moving left.

```cpp
// move camera 100px to the left → world shifts 100px to the right
glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, 0.0f, 0.0f));
```

For a full 3D camera, `glm::lookAt(position, target, up)` constructs this matrix from intuitive camera parameters.

### Projection Matrix

Transforms from **camera space** into **clip space**, then implicitly into **NDC** (Normalized Device Coordinates). NDC is the canonical coordinate system OpenGL rasterizes in: x, y, and z each range from `−1` to `+1`, the screen centre is `(0, 0)`, and anything outside the `[−1, 1]³` cube is clipped and never reaches the fragment shader. The conversion from clip space to NDC is the **perspective divide** — the GPU divides the vertex's x, y, z by its w component automatically after the vertex shader. The projection matrix is what sets up `w` so that division produces the right NDC values.

The projection matrix defines how your working coordinate system maps to this NDC cube.

**Orthographic projection** (`glm::ortho`) — objects do not shrink with distance. Used for 2D, UI, and technical drawings:

```cpp
glm::mat4 projectionMatrix = glm::ortho(0.0f, 640.0f, 0.0f, 480.0f, -1.0f, 1.0f);
//                                       left  right  bottom  top   near   far
```

This maps x ∈ `[0, 640]` and y ∈ `[0, 480]` to NDC `[-1, 1]`. It lets you specify vertex positions in **pixel coordinates** instead of NDC:

```cpp
float vertices[] = { 240.0f, 180.0f, ... }; // pixel coords, not NDC
```

**Perspective projection** (`glm::perspective`) — objects shrink with distance, giving a realistic 3D look. It maps a **view frustum** (a truncated pyramid defined by FOV, aspect ratio, and clip planes) to the NDC cube.

```cpp
glm::mat4 proj = glm::perspective(
    glm::radians(45.0f),  // vertical field of view
    640.0f / 480.0f,      // aspect ratio (width / height)
    0.1f,                 // near clip plane — closer fragments are clipped
    100.0f                // far clip plane — farther fragments are clipped
);
```

A wider FOV widens the visible area and distorts edges (wide-angle lens effect); a narrower FOV zooms in and compresses depth (telephoto effect). Fragments outside the frustum are clipped before the fragment shader runs.

#### Right-handed vs left-handed coordinate systems

**OpenGL uses a right-handed coordinate system** in view/eye space:

- +X right, +Y up, **+Z toward the viewer** (out of the screen)
- Objects in front of the camera sit at **negative Z** values

**DirectX uses a left-handed coordinate system**:

- +X right, +Y up, **+Z away from the viewer** (into the screen)
- Objects in front of the camera sit at **positive Z** values

The right-hand rule: point the fingers of your right hand along +X and curl them toward +Y — your thumb points in the +Z direction (toward you). Doing the same with your left hand points +Z away from you (left-handed).

| | OpenGL | DirectX |
|---|---|---|
| Handedness | Right-handed | Left-handed |
| "In front of camera" | Negative Z | Positive Z |
| GLM function | `glm::perspective` (default, RH) | `glm::perspectiveLH` |

`glm::perspective` generates a right-handed projection matrix by default, matching OpenGL convention. Targeting DirectX or Vulkan requires `glm::perspectiveLH` or the compile-time define `GLM_FORCE_LEFT_HANDED`.

> **Caveat**: OpenGL's *final* NDC space is technically left-handed — the projection transform negates Z so that +Z in NDC points away from the viewer. The "OpenGL is right-handed" statement refers to **view/eye space** (before projection), which is the space you set up cameras and place objects in.

---

## Matrix Multiplication and Order

Matrix multiplication is **not commutative**: `A * B ≠ B * A`. The rightmost matrix is applied first.

**Example** using 2D homogeneous coordinates (vec3 with `w=1` to allow translation):

```
Scale by ×2            Translate by (+3, 0)
S = | 2  0  0 |        T = | 1  0  3 |
    | 0  2  0 |            | 0  1  0 |
    | 0  0  1 |            | 0  0  1 |

p = (1, 0, 1)   ← w=1 marks a point (not a direction)
```

Each output component is the **dot product of one matrix row with the vector column**:

$$
\begin{pmatrix} a & b & c \\ d & e & f \\ g & h & i \end{pmatrix}
\begin{pmatrix} x \\ y \\ w \end{pmatrix}
=
\begin{pmatrix} ax + by + cw \\ dx + ey + fw \\ gx + hy + iw \end{pmatrix}
$$

**Scale first, then translate** (column-major notation: `T * S * p`):

```
        S * p:                              T * (2, 0, 1):

| 2  0  0 |   | 1 |   | 2·1 + 0·0 + 0·1 |   | 2 |       | 1  0  3 |   | 2 |   | 1·2 + 0·0 + 3·1 |   | 5 |
| 0  2  0 | × | 0 | = | 0·1 + 2·0 + 0·1 | = | 0 |  →    | 0  1  0 | × | 0 | = | 0·2 + 1·0 + 0·1 | = | 0 |
| 0  0  1 |   | 1 |   | 0·1 + 0·0 + 1·1 |   | 1 |       | 0  0  1 |   | 1 |   | 0·2 + 0·0 + 1·1 |   | 1 |
```

**Translate first, then scale** (`S * T * p`):

```
        T * p:                              S * (4, 0, 1):

| 1  0  3 |   | 1 |   | 1·1 + 0·0 + 3·1 |   | 4 |       | 2  0  0 |   | 4 |   | 2·4 + 0·0 + 0·1 |   | 8 |
| 0  1  0 | × | 0 | = | 0·1 + 1·0 + 0·1 | = | 0 |  →    | 0  2  0 | × | 0 | = | 0·4 + 2·0 + 0·1 | = | 0 |
| 0  0  1 |   | 1 |   | 0·1 + 0·0 + 1·1 |   | 1 |       | 0  0  1 |   | 1 |   | 0·4 + 0·0 + 1·1 |   | 1 |
```

Same matrices, different order → different result.

### Column-major (OpenGL / GLM) vs row-major (DirectX)

**Memory layout** is what "column-major" and "row-major" actually refer to — how the 16 floats of a 4×4 matrix are ordered in a flat array.

Given a matrix written on paper as:

```
M = | a  b  c  d |
    | e  f  g  h |
    | i  j  k  l |
    | m  n  o  p |
```

| Convention | Memory order | Used by |
|---|---|---|
| **Column-major** | `a e i m  b f j n  c g k o  d h l p` | OpenGL, GLM, GLSL, Metal |
| **Row-major** | `a b c d  e f g h  i j k l  m n o p` | DirectX, HLSL, row vectors in math textbooks |

Column-major stores the **first column** (`a e i m`) first, then the second column, and so on. Row-major stores the **first row** (`a b c d`) first.

This affects how translation is stored. A translation matrix looks like:

```
| 1  0  0  tx |
| 0  1  0  ty |
| 0  0  1  tz |
| 0  0  0   1 |
```

- **Column-major (GLM)**: `tx ty tz` are at indices `[12] [13] [14]` — i.e. the 4th column.
- **Row-major**: `tx ty tz` are at indices `[3] [7] [11]` — i.e. the 4th element of rows 1–3.

**Why this affects multiplication notation.** A column-major matrix multiplied by a **column vector** on the right (`M * v`) matches the convention used in GLSL (`u_MVP * position`). A row-major matrix is typically multiplied by a **row vector** on the left (`v * M`). Same transform, different syntax.

**In code**, the multiplication order in the chain is reversed between the two:

```cpp
// GLM / OpenGL (column-major): rightmost applied first
glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;

// HLSL / DirectX (row-major): leftmost applied first
float4x4 mvp = modelMatrix * viewMatrix * projectionMatrix;
```

**`GL_FALSE` in `glUniformMatrix4fv`** means "don't transpose before uploading". Since GLM is already column-major and that is what the GLSL shader expects, no transposition is needed. If you were feeding a row-major matrix (e.g. from a DirectX math library), you would pass `GL_TRUE` to transpose it on upload.

---

## Uploading a Matrix Uniform

```cpp
glUniformMatrix4fv(
    GetUniformLocation(name),
    1,          // number of matrices
    GL_FALSE,   // do NOT transpose — GLM is already column-major
    &matrix[0][0]  // pointer to first float; glm::value_ptr(matrix) is equivalent
);
```

`&matrix[0][0]` and `glm::value_ptr(matrix)` (from `<glm/gtc/type_ptr.hpp>`) both yield a `const float*` to the raw matrix data.

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

---

## Rendering Multiple Objects

There are two fundamentally different approaches to drawing more than one object on screen.

### Approach 1 — Multiple draw calls (one per object)

Reuse the same VAO/VBO/IBO and issue one `Draw` call per object, updating the relevant uniforms (typically the model matrix) between calls.

```cpp
// render loop
{
    glm::mat4 mvp = proj * view * glm::translate(glm::mat4(1.0f), translationA);
    shader.SetUniformMat4f("u_MVP", mvp);
    renderer.Draw(va, ib, shader);
}
{
    glm::mat4 mvp = proj * view * glm::translate(glm::mat4(1.0f), translationB);
    shader.SetUniformMat4f("u_MVP", mvp);
    renderer.Draw(va, ib, shader);
}
```

Each `Draw` call is a separate GPU command. The GPU receives N draw calls for N objects.

**When to use**: different 3D meshes, objects with unique shaders or textures, anything where per-object state (transform, material) differs.

### Approach 2 — Batch rendering (single draw call)

Combine all object vertices into one VBO with their world-space positions pre-baked in (or with per-vertex transform data). Issue one `glDrawElements` for the whole batch.

```
VBO = [quad A vertices | quad B vertices | ...]
IBO = [indices for A   | indices for B   | ...]
→ one glDrawElements draws everything
```

**When to use**: many identical or similar objects (tilemap, particle system, text glyphs) where the cost of individual draw calls would dominate. The CPU overhead of N draw calls is replaced by the cost of updating the combined buffer.

### Trade-offs

| | Multiple draw calls | Batch rendering |
|---|---|---|
| CPU overhead | O(N) draw calls | O(1) draw call + buffer update |
| Flexibility | Easy per-object shader / texture | All objects share one shader / texture |
| Typical use case | 3D scene objects | Tilemaps, sprites, text |

---

## Batch Rendering Variants

### Variant A — Per-vertex color (no textures)

Each vertex carries RGBA data directly in the VBO. The fragment shader outputs the interpolated color; no texture sampling is involved.

**Vertex layout** (9 floats if combined with texture variant, 6 floats for color-only):

```
[  x  ][  y  ][ r ][ g ][ b ][ a ]
 pos(2)         color(4)
```

**Vertex shader** receives `layout(location = 1) in vec4 a_Color` and passes it through as `out vec4 v_Color`. Fragment shader simply writes `o_Color = v_Color`.

This is the simplest batch — one draw call, no texture binds, shader is trivial.

### Variant B — Multiple textures via texture slots

When each quad in the batch needs a different texture, the texture unit index is stored as a per-vertex float attribute (`a_TexIndex`). Each unique texture is bound to a different slot before the draw call.

**Vertex layout** (per vertex):

```
[  x  ][  y  ][ r ][ g ][ b ][ a ][  u  ][  v  ][ texIdx ]
 pos(2)         color(4)              uv(2)        index(1)
```

**Fragment shader** samples from a `sampler2D` array indexed by the vertex value:

```glsl
uniform sampler2D u_Textures[2];

void main() {
    int index = int(v_TexIndex);
    o_Color = texture(u_Textures[index], v_TexCoord);
}
```

**C++ setup** — bind each texture to its slot, then upload the slot indices as an int array uniform:

```cpp
m_TexturePizza->Bind(0);       // GL_TEXTURE0
m_TextureBaguette->Bind(1);    // GL_TEXTURE1
int samplers[2] = {0, 1};
shader.SetUniform1iv("u_Textures", 2, samplers);
```

`glUniform1iv` uploads an `int[]` uniform — the `iv` suffix means "integer vector (array)":

```cpp
void Shader::SetUniform1iv(const std::string& name, unsigned int size, int* value)
{
    GLCall(glUniform1iv(GetUniformLocation(name), size, value));
}
```

**Limitation**: the number of available texture slots is hardware-dependent and typically small (OpenGL guarantees at least 8 combined image units; most desktop GPUs expose 16–32). A batch can only contain as many unique textures as there are available slots.

### Variant D — Dynamic batch (CPU updates vertex data every frame)

In all previous variants the vertex data is uploaded once with `GL_STATIC_DRAW` and never changed. A **dynamic batch** allocates the VBO with `GL_DYNAMIC_DRAW` and `nullptr` data, then overwrites part of it each frame with `glBufferSubData`.

**Allocation — reserve GPU memory without uploading data:**

```cpp
glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB);
glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 1000, nullptr, GL_DYNAMIC_DRAW);
//                                                    ^^^^^^^ no data yet
```

Passing `nullptr` tells the driver to allocate the memory but leave it uninitialised. `GL_DYNAMIC_DRAW` is a hint that the buffer will be updated frequently, allowing the driver to place it in more write-friendly memory.

**Per-frame update — write new vertex data into the existing buffer:**

```cpp
// build quads on the CPU
auto q0 = CreateQuad(x, y, texID);
auto q1 = CreateQuad(200, -50, 1);
Vertex vertices[8];
memcpy(vertices,              q0.data(), q0.size() * sizeof(Vertex));
memcpy(vertices + q0.size(),  q1.data(), q1.size() * sizeof(Vertex));

// push to GPU — no reallocation, just overwrites bytes [0, sizeof(vertices))
glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB);
glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
```

`glBufferSubData(target, offset, size, data)` copies `size` bytes from `data` into the bound buffer starting at `offset`. The buffer must already have been allocated with at least `offset + size` bytes via `glBufferData`.

The IBO is still uploaded once as `GL_STATIC_DRAW` — index patterns don't change, only vertex positions do.

**`offsetof` for struct-based vertex layouts:**

When vertices are defined as structs, `offsetof` gives the byte offset of each member without manual arithmetic:

```cpp
struct Vertex { Vec2 Position; Vec4 Color; Vec2 TexCoords; float TexID; };

glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position));
glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Color));
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexCoords));
glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexID));
```

**When to use dynamic batching**: object positions, sizes, or colors change every frame (e.g. particles, draggable sprites). The CPU assembles world-space geometry and uploads the changed region; the GPU issues a single draw call for the whole batch.

---

### Variant C — Texture atlas (alternative to multiple slots)

A texture atlas packs multiple images into a single texture file. Each quad's UV coordinates are set to the sub-region of the atlas that contains its image. Only one texture is bound, so the slot limitation does not apply.

**Trade-offs vs. texture slots**:

| | Texture slots | Texture atlas |
|---|---|---|
| Number of unique textures per batch | Limited by GPU slot count | Unlimited (atlas can be arbitrarily large) |
| Setup complexity | Simple — bind textures individually | Requires pre-packing atlas and computing sub-UVs |
| Runtime flexibility | Can swap textures freely | Adding new textures requires rebuilding the atlas |
| Typical use case | Small fixed sets (2–8 textures) | Sprite sheets, tilemaps, UI icon sets |
