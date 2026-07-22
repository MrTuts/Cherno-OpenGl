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

---

## Error Reporting

`include/openglErrorReporting.h` + `src/openglErrorReporting.cpp` wrap OpenGL's debug output extension (`GL_KHR_debug`). Calling `enableReportGlErrors()` registers `glDebugOutput` as a callback. The driver then calls it whenever an error, warning, or performance issue occurs, printing the source, type, and message. This is much better than manually calling `glGetError()` after every draw call.

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

```cpp
glDrawArrays(
    GL_TRIANGLES,   // primitive type
    0,              // starting vertex index
    3               // number of vertices to read
);
```

OpenGL reads 3 vertices starting at index 0 from the currently bound VAO's attribute sources, runs the vertex shader on each, rasterizes the resulting triangle, and runs the fragment shader on every covered pixel.

For non-sequential geometry (shared vertices), `glDrawElements` is used instead — it takes an **index buffer (EBO)** that lists which vertices to use in which order, avoiding duplication.

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
glGenBuffers / glBindBuffer / glBufferData   ← upload positions to GPU
    │
glVertexAttribPointer / glEnableVertexAttribArray  ← describe layout
    │
CompileShader(vertex) + CompileShader(fragment)
    │
CreateShader → glLinkProgram → glUseProgram
    │
── render loop ──
    glClear(GL_COLOR_BUFFER_BIT)
    glDrawArrays(GL_TRIANGLES, 0, 3)
    glfwSwapBuffers()
    glfwPollEvents()
── end loop ──
    │
glDeleteProgram / glfwTerminate
```
