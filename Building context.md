# Building Context — Vulkan_Baseline

A running reference for this project: the conventions, the build system, the Vulkan
pipeline staging, the Engine/Game architecture, the shader + threading systems, and
the framework rework. Read this before adding code so new work matches what's here.

> Repo: `R:\Work\Projects\Vulkan_Baseline` (opened via the `Vulkan_Baseline.lnk`
> shortcut on the desktop). This is a Vulkan renderer being built toward eventual
> hosting inside a **C# / WPF editor**.

---

## 1. Namespaces

| Namespace | What lives there |
|---|---|
| `Neelam` | the engine: `Engine`, `Game`, `Color`, `Colors` |
| `Neelam::vk` | the Vulkan objects: `Window`, `Instance`, `Surface`, `PhysicalDevice`, `QueueFamily`, `LogicalDevice`, `Swapchain`, `GraphicsPipeline`, `ShaderObject`, `ShaderModule`, `ShaderCompiler`, `ShaderWatcher` — **plus**, from `Framework.h`: `VulkanAllocator`, `Validation::`, `MemTrack::` |
| `Azul` | the **libraries** only: Math (`Vec3/Vec4/Mat4/Quat`), File, AnimTime, Manager (`DLink`, `DLinkMan`, `ManBase`, `CompareStrategyBase`) |

The ported camera classes are `Neelam::` (engine code), and derive across the line —
`Neelam::CameraNode : Azul::DLink`, `Neelam::CameraNodeMan : Azul::ManBase`. Inside
them every math and container type is explicitly `Azul::`-qualified, the same way
`Neelam::Color` does it (§7).

> **Watch out:** a class that lives in the app but inherits from a lib class must
> **not** carry that lib's export macro. `CameraNodeCompareStrategyEnumName` shipped
> with `MANAGER_LIBRARY_API` on every member; in the app that expands to
> `__declspec(dllimport)`, so the compiler warns C4273 (*inconsistent dll linkage*)
> and the linker then wants `__imp_??0…` from Manager.dll, which never exports it →
> LNK2019. Strip the macro: only classes compiled **into** the DLL may wear it.

Rule: **"Azul is for the libraries."** Anything we write for the engine is `Neelam`
(or `Neelam::vk`). `Align16` is **global** (from `Framework.h`), not Azul.

---

## 2. House rules / conventions (follow these)

- **No `std::vector`.** Use fixed-size arrays + a count (e.g. `privQueues[8]`,
  `privImages[8]`, `MaxFramesInFlight`), or `new[]`/`delete[]` where the size is
  genuinely dynamic (as in `Validation::IsSupported`, `PhysicalDevice`,
  `QueueFamily`). `std::mutex`/`std::thread` are fine.
- **No designated initializers** (`.sType = ...`). The project is **C++17**; use
  `T x = {};` then assign fields. (Demo/sample code uses them — convert.)
- **No memory leaks — ever.** The framework tracks memory aggressively; a clean run
  must end with `Memory Tracking: passed`. A leak fails the build's own check.
- **One class per file**, file named after the class. Each `Vulkan/VulkanAPI` class
  lives in its **own subfolder** (`VulkanAPI/Instance/Instance.{h,cpp}`, etc.).
- **RAII lifecycle:** deleted copy ctor + copy assign; destructor calls `Destroy()`;
  `Destroy()` is idempotent (null-checks, re-nulls).
- **`void Create(...)`, not `bool`.** Failures go through `VK_Try` (assert + exit).
  Only `Window::Create` still returns `bool` (Win32 status, not `VkResult`).
- **`this->` on member access.** Tabs for indentation. Match surrounding comment
  density; comments explain **why**, not what.
- **Copyright header:** `// Copyright 2026 by Riddhiman Raguraman`.
- **`Debug::out`, not `Trace::out`** (thread-tagged — see §10/§11).

---

## 3. Build system (premake) — and the #1 gotcha

- `premake5.lua` is the single source of truth. `UberBuildMe.bat` runs premake then
  builds. Regenerate directly with:
  ```bash
  Vendor\bin\premake\premake5.exe vs2022
  ```
- **THE GOTCHA — re-run premake after adding or moving any file.** The file glob
  (`Vulkan_Baseline/**.{h,hpp,cpp}`) and `includeTree()` only sweep the tree **when
  premake runs**. Skip it and you get:
  - `LNK2019` (a new `.cpp` compiles but isn't linked — it's not in the `.vcxproj`)
  - `C1083` (a new folder's headers aren't found — folder not on the include path)
  - Building via **F5 in Visual Studio** uses the stale `.vcxproj`; `UberBuildMe.bat`
    regenerates first, so it's always safe.
- **`includeTree("Vulkan_Baseline")`** — a helper that returns the folder plus every
  subfolder at any depth, fed to `includedirs`. This is why includes are
  **unprefixed** (`#include "Instance.h"`) and keep working no matter how deep a
  class is nested. Skips `obj`/`bin`/`x64`/`intermediate` and dot-folders.
- **App target:** `WindowedApp` (`/SUBSYSTEM:WINDOWS`) + `entrypoint "mainCRTStartup"`
  so there's no console window but we keep plain `int main()`. Console output was
  always empty anyway — `Debug::out` writes via `OutputDebugString`, not stdout.
- **Vulkan SDK:** `os.getenv("VULKAN_SDK")` or fallback `C:/VulkanSDK/1.4.350.0`.
- **Prebuild wipe** of the `Azul` scratch folder (see §11) so the framework banner
  always prints.
- **`project "Framework"` — `kind "SharedItems"`** (a VS `.vcxitems` shared project) at
  the solution root, referenced by every project via `links { "Framework" }`. Shared
  items are compiled by each *consumer* under that consumer's own defines, which is
  exactly what the feature tiers need (§11): one `Framework.h`, two behaviours.
  It contains **`Framework.h` and nothing else** — there is no implementation `.cpp`
  (see below). `includedirs` still carries `frameworkDir` everywhere, so the
  `#include` resolves whether or not the shared reference contributes a path.

### volk + VMA + DXC

- **Declarations** for volk + VMA come from `Framework.h` (its `VULKAN FRAMEWORK`
  section), force-included into every TU.
- **Bodies** compile exactly once, in **`Framework/VulkanImpl.cpp`**. That split is
  not stylistic: `vmaCreateAllocator` and volk's ~400 function pointers are ordinary
  non-inline definitions, and the One Definition Rule allows each once per *program*.
  Compile them in all 23 app TUs and you get `LNK2005` hundreds of times.
  **An include guard cannot prevent this** — a guard is per-TU, and every TU starts
  with a clean macro table. Only "one file compiles it" works.
- `VulkanImpl.cpp` includes both headers a **second** time (Framework.h already
  brought in the declarations) with `VOLK_IMPLEMENTATION` / `VMA_IMPLEMENTATION` set.
  Safe because in both headers the implementation block sits **outside** the header's
  own include guard — so the guard swallows the declarations on that pass and only
  the bodies compile. Standard single-header-library contract.
- It lives in `Framework/` but is compiled **only by the app project**, listed
  explicitly in premake. It must *not* join the shared `Framework` project: shared
  items are compiled by every consumer, and the lib DLLs have neither
  `USE_VULKAN_FRAMEWORK` nor the SDK include path. A `#ifdef USE_VULKAN_FRAMEWORK`
  around the whole file makes that a no-op rather than a `C1083` if it ever strays.
- A `vpaths` rule files it under a **Framework** folder in Solution Explorer —
  without it, a file from outside the project folder lands at the project root next
  to `main.cpp`. Cosmetic only.
- **We do NOT link `vulkan-1.lib`.** volk `LoadLibrary`s `vulkan-1.dll` itself at
  runtime (`volkInitialize`). Only the SDK **include** path is needed.
- **Defines:** `VK_NO_PROTOTYPES` (vk\* are volk function pointers) and
  `VK_USE_PLATFORM_WIN32_KHR` (turns on `vkCreateWin32SurfaceKHR`).
- **DXC** (DirectX Shader Compiler) is linked (`dxcompiler.lib`) and `dxcompiler.dll`
  is copied to the output dir (postbuild) — it compiles HLSL→SPIR-V at runtime (§9).

---

## 4. The Vulkan pipeline staging (creation order)

Built in `Engine::Initialize`, in this exact order; torn down in **reverse**.

| # | Class | Owns / does | Key notes |
|---|---|---|---|
| 1 | `Window` | the Win32 `HWND` | standalone top-level **or** child window (WPF host). Routes `WndProc` via a stashed `this` pointer. Child mode never `PostQuitMessage`. |
| 2 | `Instance` | `VkInstance` | `volkInitialize()` first, then `volkLoadInstance()`. Validation layer + debug messenger in Debug. |
| 3 | `Surface` | `VkSurfaceKHR` | the **Win32↔Vulkan bridge** — the only Win32-flavored Vulkan call. Borrows the instance. |
| 4 | `PhysicalDevice` | picks the GPU | **selects**, doesn't create (no `vkDestroy`). Prefers a discrete GPU; verifies the surface format. `Destroy()` is a no-op reset. |
| 5 | `QueueFamily` | graphics+present family **index** | selection only; the real `VkQueue` comes after the device. `Destroy()` no-op. |
| 6 | `LogicalDevice` | `VkDevice` + `VkQueue`(s) | first **owned** resource (`vkDestroyDevice`). Two-phase: `Add(familyIndex)` then `Create()`. `volkLoadDevice()` for direct dispatch. Enables `dynamicRendering`, `synchronization2`, `timelineSemaphore`, `bufferDeviceAddress`. |
| 7 | `VulkanAllocator` | `VmaAllocator` | **Framework singleton** (`Framework.h`), not an Engine member — Engine only calls `Create`/`Destroy` at the right point. `vmaImportVulkanFunctionsFromVolk`. `BUFFER_DEVICE_ADDRESS` flag (matches the device feature). Wires the memory tracker in unconditionally (§11). |
| 8 | `Swapchain` | `VkSwapchainKHR`, color images/views, per-image render-complete semaphores, depth image (via VMA) | FIFO present mode. Extent from surface `currentExtent`. Color images owned by the swapchain (not destroyed individually). |
| 9 | `GraphicsPipeline` | the **frame loop** (command pools/buffers, timeline semaphore) | see §8. |
| + | `ShaderObject` | the `VkPipeline` | created in `Game::LoadContent`; owns shaders + pipeline, hot-reloadable (§9). |

**Why the order matters:** each object borrows handles from the ones above it, so
teardown must reverse it (surface before instance; allocator/swapchain before device).

---

## 5. Engine / Game — Template Method

- **`Engine`** (abstract base) owns *all* the vk objects **by composition** plus the
  frame loop (`Azul::AnimTimer` for delta time, clamped to `1/30s`). Public API:
  - `Initialize(HWND hParent = nullptr)` — `nullptr` = standalone top-level window;
    non-null = child window inside the editor's HWND.
  - `Tic()` — one frame: compute dt, `Update(dt)`, `Render()`. **Does not** pump the
    message queue (WPF owns the pump when hosted).
  - `Run()` — standalone loop: pump messages + `Tic()` until `WM_QUIT`, then `Shutdown()`.
  - `Shutdown()`, `GetWindowHandle()`.
  - Protected pure-virtuals the Game fills: `LoadContent`, `UnloadContent`,
    `Update(float)`, `Render`.
- **`Game`** (derived) fills the four hooks. Owns the `ShaderObject_ColorByVertex` and
  the `ShaderWatcher`. `Game::Render()` calls `graphicsPipeline.Render(shader)` — the
  frame loop lives in `GraphicsPipeline`, **not** inline in `Game::Render`.
- **Two run modes:**
  - **Standalone** (`main.cpp`): `Game game; game.Initialize(); game.Run();`
  - **Editor** (later): C# links the engine as a **DLL**, calls `Initialize(hwnd)` /
    `Tic()` / `Shutdown()` itself, and hosts `GetWindowHandle()` in a WPF `HwndHost`.
    The public API is deliberately free of C++ types so it maps 1:1 to future
    `extern "C"` P/Invoke exports.

---

## 6. Frame loop (`GraphicsPipeline::Render`)

Modern Vulkan 1.3 path (all the enabled device features feed this):

- **Pacing via a single TIMELINE semaphore**, not per-frame fences. `MaxFramesInFlight
  = 2`. Frame N waits for the timeline to reach the value frame `N-MaxFramesInFlight`
  signalled — exactly when its command resources are free to reuse.
- **Per frame-in-flight:** own command pool + primary command buffer + binary
  image-acquire semaphore. **Per swapchain image:** a render-complete semaphore.
- Steps: wait timeline → `vkAcquireNextImageKHR` → record
  [`vkCmdPipelineBarrier2` (sync2) UNDEFINED→COLOR/DEPTH attachment → `vkCmdBeginRendering`
  (dynamic rendering, clear color+depth) → dynamic viewport/scissor → `SetActive`
  (bind pipeline) → `vkCmdDraw(3)` → `vkCmdEndRendering` → barrier COLOR→PRESENT_SRC]
  → `vkQueueSubmit2` (wait acquire, signal render-complete + timeline) → `vkQueuePresentKHR`.
- **Swapchain recreate:** `OUT_OF_DATE`/`SUBOPTIMAL` sets a stale flag; the Engine
  rebuilds the swapchain in place (the `GraphicsPipeline` holds a `Swapchain*` and
  queries it fresh each frame, so a rebuild is transparent).
- **Build-differentiating clear color:** Debug = `Colors::LightGray`, Release =
  `Colors::Wheat` (glance at the background, know the build). Settable via
  `SetClearColor`. Mirrors the old Azul `ClearDepthStencilBuffer`.

---

## 7. Color system

- `Neelam::Color` — a `Vec4`-backed `Align16` class (red/green/blue/alpha) in
  `Vulkan/Utilities/Color.{h,cpp}`.
- `Neelam::Colors::*` — the full DirectX-style named palette (`Vec4`s) in
  `Vulkan/Utilities/Colors.h`.
- `Align16` is global; `Vec3`/`Vec4` are `Azul::` (qualified explicitly).

---

## 8. Shader system

- **HLSL source, compiled to SPIR-V at RUNTIME via DXC** (`dxcapi.h`, `IDxcCompiler3`)
  — **not** precompiled. That's what makes it hot-reloadable.
- Layers:
  - **`ShaderCompiler`** — DXC wrapper: `CompileFile(path, stage) → IDxcBlob` (SPIR-V).
    Uses the `SafeRelease(x)` macro (`{ if(x){ x->Release(); x = 0; } }`) for the COM
    objects. Logs DXC errors and returns null on failure (hot-reload keep-last-good).
  - **`ShaderModule`** — wraps one `VkShaderModule` + source path + stage. `Reload()`
    recompiles; keeps the old module on failure.
  - **`ShaderObject`** (abstract) — owns the pipeline **layout + `VkPipeline`**. In
    Vulkan shaders are **baked into the pipeline** (not bound individually), so
    `SetActive(cmd)` = `vkCmdBindPipeline`, and `Reload()` **rebuilds the pipeline**
    (`vkDeviceWaitIdle` first). Dynamic rendering; depth on; cull none.
  - **`ShaderObject_ColorByVertex`** (derived) — the triangle technique; supplies the
    two `.hlsl` paths.
- HLSL lives in `Vulkan_Baseline/Shader/hlsl/`, resolved at runtime via the
  `SOLUTION_DIR` macro (so the working directory doesn't matter). The triangle's
  positions + colors are baked in the vertex shader, indexed by `SV_VertexID` — **no
  vertex buffer / input layout** yet.
- **Triangle size** is resolution-independent: NDC coords → the dynamic viewport (=
  window). It scales with the window and stretches with aspect ratio (no projection
  matrix / aspect correction yet).

---

## 9. Threading / actor model

- **`ThreadManagement/`** folder holds the threading + actor plumbing.
- **`ShaderWatcher`** is the first "actor": a background thread that polls the
  `.hlsl` folder's timestamps and, on a change, posts a message into a **mutex-guarded
  mailbox** (`std::queue`). The **engine thread** drains the mailbox each frame
  (`Drain()`) and does the actual reload — **all GPU/Vulkan work stays on the engine
  thread**; the watcher only detects + posts. This one-way
  detect→message→consume split is the seed of the planned engine-thread / game-thread
  actor model (later: typed messages + handles).
- **Thread framework is ON** (`USE_THREAD_FRAMEWORK` in `Framework.h`). `Debug::out`
  tags each line with the thread name + indentation. Main thread is named
  `"EngineThread"` (`START_BANNER_MAIN`); the watcher is `"ShaderWatcher"`
  (`Debug::SetCurrentName` + `SimpleBanner`). Output looks like:
  ```
  thread(...) EngineThread: begin()
      (EngineThread): Swapchain: created (2 images, 1280x720)
      thread(...) ShaderWatcher: begin()
          (ShaderWatcher): change detected -> posting reload message
      (EngineThread): ShaderObject: reloaded + pipeline rebuilt
  ```

---

## 10. Error handling

- **`VK_Try(expr)`** (in `Framework.h`, VULKAN FRAMEWORK section): checks a `VkResult`. Failure test is
  **`< 0`** (negative = error; `VK_SUBOPTIMAL_KHR` etc. are positive/OK — do **not**
  use `!= VK_SUCCESS`). On failure: `Debug::out` with file/line + decoded name
  (`vkResultToString`), then `assert(false)` + `ExitProcess`.
- Non-`VkResult` logic failures (no GPU, no queue family, unsupported format): the
  same `Debug::out` + `assert(false)` + `ExitProcess` pattern. This is why the
  `Create` functions are `void` — there's no failure path to branch on.

---

## 11. Framework rework (the big one)

`Framework.h` is force-included into **every** TU (provides `Trace`/`Debug`/`MemTrace`/
`Align16`/`windows.h` with `NOMINMAX`). Changes made:

### Thread framework + Debug::out
`USE_THREAD_FRAMEWORK` enabled; all engine `Trace::out` swapped to `Debug::out` so
prints carry the thread name. `START_BANNER_MAIN("EngineThread")` in `main()` starts
the Debug system + names the main thread.

### Validation callback buffer fix
`Debug::out` formats into a **256-char** internal buffer and asserts ("Buffer too
small") on overflow. Validation messages are longer, so the debug-utils callback
**memcpys the message through `Debug::out` in ≤180-char chunks** (nothing lost, stays
thread-tagged, no `OutputDebugStringA` — we keep the Keenan framework).

### Placement-new barricade (volk/VMA in-app)
Framework's Debug macro `#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)` mangles
VMA's **placement new**. Both VMA includes in `Framework.h` (declarations, and the
implementation block) wrap only the `#include` with `#pragma push_macro("new")` / `#undef` (also
`malloc`/`free`/`calloc`/`realloc`) / `#pragma pop_macro`. This is **per-TU and
balanced** — it does not affect any other file; `new`/`malloc` elsewhere are still
tracked normally. (volk needs no barricade — it's C-style, no `new`.)
- The framework's `AZUL_PLACEMENT_NEW_BEGIN/END` are for placement new **inside a
  function body** (they emit runtime statements); for a **file-scope `#include`** use
  the raw `push_macro`/`#undef`/`pop_macro` as above.

### Vulkan memory tracking — `USE_VULKAN_FRAMEWORK`
Mirrors the CUDA `_CLIENT_BLOCK` shadow pattern. Independent of
`USE_CUDA_FRAMEWORK` — **both can be enabled at once**; guarded so it's a no-op when
off. In `Framework.h`:
- `_VkMemTrackerHeader` — shadow record (`VkDeviceMemory` kept as `void*`, so this
  part of `MemTrace` still compiles with no Vulkan include).
- **Device (GPU) memory** — `VkMemInsert` / `VkMemRemove`: `_malloc_dbg(sizeof(hdr),
  _CLIENT_BLOCK, ...)` on alloc, walk-and-`_free_dbg` on free. Leaks reported at
  process end by `privVkDumpLeaks` (`VkLeak(n) ... <double-click>`, like `CudaLeak`).
- **Host (CPU) memory** — `VkHostAlloc/Realloc/Free` via `_aligned_*_dbg` (host
  Vulkan allocations *are* CRT memory, so they're tracked natively — no shadow).
- `VkLeakCount()`; `privVkDumpLeaks()` hooked into the ProcessEnd report.

Wired in `vk::VulkanAllocator::Create` — **inside the framework, with no `#if` around
it**: a `VkAllocationCallbacks` (host → `VkHost*`) and a `VmaDeviceMemoryCallbacks`
(device → `VkMemInsert`/`VkMemRemove`) handed to `VmaAllocatorCreateInfo`. VMA fires
the device callback only **per big VkDeviceMemory block** (a handful of times), so —
unlike the CUDA per-thread device path — it's cheap.

### The VULKAN FRAMEWORK section (why the allocator moved)
`USE_VULKAN_FRAMEWORK` used to leak into engine code: `Allocator.cpp` carried a
`#if defined(_DEBUG) && defined(USE_VULKAN_FRAMEWORK)` block, so engine code knew
tracking existed. It shouldn't — the framework's own idiom is
`#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)`: interception is
invisible at the call site. So the whole Vulkan layer moved into `Framework.h`, under
one `#ifdef USE_VULKAN_FRAMEWORK` at the **end of the file** (after the thread block,
so `Debug::out` and `using namespace ThreadFramework` are already in scope):

| In `Framework.h` | Was |
|---|---|
| `#include <Volk/volk.h>` + barricaded `<vma/vk_mem_alloc.h>` | `VulkanUtilities.h`, `Allocator.h` |
| `vkResultToString` / `vkAssertImpl` / `VK_Try` | `VulkanUtilities.h` |
| `Neelam::vk::Validation::*` | `VulkanUtilities.h` |
| `Neelam::vk::MemTrack::*` (the VMA callbacks) | anon namespace in `Allocator.cpp` |
| `Neelam::vk::VulkanAllocator` (**singleton**) | `Neelam::vk::Allocator` (Engine member) |

`VulkanAllocator` is a singleton because there is exactly one `VmaAllocator` per
process (one logical device), and because `Swapchain` — and later buffer/texture
code — needs it without `Engine` threading a handle down. `Engine` still decides
**when**: `Create` after the logical device, `Destroy` before it (§4 order unchanged).

Both VMA includes inside `Framework.h` need the **placement-new barricade**, because
`#define new ...` is set earlier in the same file.

### Feature tiers — the flags are per-project now
`USE_THREAD_FRAMEWORK` / `USE_VULKAN_FRAMEWORK` / `MEM_TRACKER_ENABLED` are no longer
`#define`d inside `Framework.h`. They are **project-level defines in `premake5.lua`**,
because the projects are not all in the same tier:

| Project | Defines | Gets |
|---|---|---|
| Math / File / AnimTime + their tests | `MEM_TRACKER_ENABLED` | memory tracking only — never parses volk/VMA, needs no Vulkan SDK include path |
| Vulkan_Baseline (app) | all three | + thread framework + Vulkan framework |

`Framework.h` enforces the tiering with `#error`s: `USE_VULKAN_FRAMEWORK` requires
both of the others (it reports through `MemTrace` and prints through `Debug::out`).

### ⚠ Tier gotcha — what may NOT be gated on `USE_VULKAN_FRAMEWORK`

**The process-end report is printed by whichever module unwinds LAST, and that is a
lib DLL, not the app.** `MemTrace::ProcessEnd()` runs once per *module* (exe + each
DLL); each decrements `DLL_COUNT.bin`, and only the one that hits zero prints. DLLs
unload after the exe's statics, so the printer is `Math` / `File` / `AnimTime` — none
of which are in the Vulkan tier.

So anything gated on `USE_VULKAN_FRAMEWORK` **compiles out of the module that actually
prints**. That is what silently killed the `VkLeak` dump: `privVkDumpLeaks()` was
guarded that way, the host `Leak(n)` lines still appeared (shared debug CRT heap sees
every module's blocks), and the device section just vanished — no error, no warning.
It worked before the tiering only because `USE_VULKAN_FRAMEWORK` was `#define`d inside
`Framework.h`, so every module had it.

**Rule:** the `MemTrace` Vulkan members — `_VkMemTrackerHeader`, `vk_mtx`/`vk_leak`,
`VkMemInsert`/`VkMemRemove`, `VkHostAlloc/Realloc/Free`, `VkLeakCount`,
`privVkDumpLeaks`, and its `ProcessEnd` call — are gated on **`MEM_TRACKER_ENABLED`**,
which every module has. They can be, because *none of them names a Vulkan type* —
`VkDeviceMemory` is deliberately held as `void*`. Only the genuinely Vulkan-typed
layer (the volk/VMA includes, `VK_Try`, `Validation::`, `MemTrack::`,
`VulkanAllocator`) sits under `USE_VULKAN_FRAMEWORK`. **If a leak reports its host
blocks but no `VkLeak` line, check this first.**

### Memory-tracking gotchas
- The framework keeps a `DLL_COUNT.bin` refcount in `<AZUL_PATH>\Azul`. A **killed run**
  (Stop Debugging / crash) leaves it stale, which suppresses the framework banner on
  later runs. The premake **prebuild wipes** that folder. `AZUL_PATH` is a machine-wide
  user env var pointing at a shared scratch folder.

---

## 12. Validation layers + implicit-layer noise

- Validation (`VK_LAYER_KHRONOS_validation` + `VK_EXT_debug_utils`) is **Debug-only**,
  set up in `Instance::Create` via the `Validation::` helpers in `Framework.h`.
  Availability is checked first (graceful if the SDK isn't installed).
  `vkCreateDebugUtilsMessengerEXT` is an extension fn — under the static loader you'd
  fetch it with `vkGetInstanceProcAddr`; volk auto-loads it (the boilerplate volk
  removes).
- **Implicit layers:** OBS / Steam / GOG Galaxy / Epic register Vulkan *implicit
  layers* (under `HKLM\...\Khronos\Vulkan\ImplicitLayers`) that the loader pulls into
  **every** Vulkan process — the tool need not be running. Each ships a
  `disable_environment` key. OBS's noise is silenced with the
  `DISABLE_VULKAN_OBS_CAPTURE=1` debugger env in premake. The recurring
  `GalaxyOverlayVkLayer ... naming standard` `WARN`s are harmless (GOG's overlay).
  Overlays hook `vkQueuePresentKHR`, so disable them first if present/swapchain
  behavior ever looks weird.

---

## 13. Testing the tracker (no scaffolding in engine code)

**Rule: a tracker test never edits an engine `.cpp`.** The tracker is wired in
unconditionally inside `vk::VulkanAllocator::Create` (Framework.h), so any real leak
reports itself — there is nothing to switch on. The deliberate leak is a **framework
API**, not a commented-out `Destroy` in `Swapchain`:

```cpp
Neelam::vk::VulkanAllocator::LeakTest();     // one call, from main.cpp
```

It allocates one `DEDICATED_MEMORY` VMA buffer and never frees it — dedicated so it
gets its own `VkDeviceMemory` block, which is what guarantees the device-alloc
callback fires. **Delete the call site, never the function.** Expected output (all via
`OutputDebugString`, so the VS Output window — **not** `MemTrackerLog.txt`, which
carries only the summary counts):

```
VulkanAllocator: LEAK TEST -- leaked 4194304 bytes on purpose
[VMA] unfreed allocation -- vk_mem_alloc.h(6578)
vkDestroyDevice(): VkDevice ... has N leaked objects ... VkDeviceMemory, VkBuffer
Engine: shutdown clean
VkLeak(0)  4194304 bytes  VMA-device  Framework.h(<line>)   <- device shadow
Leak(0..n) ...                        Framework.h(<line>)   <- host, VkHostAlloc
>>> Memory Tracking: FAIL <<<
```

### Why `VMA_ASSERT_LEAK` is permanently downgraded

A leaked VMA allocation trips VMA in `~VmaDedicatedAllocationList`:

```cpp
VMA_ASSERT_LEAK(false && "Unfreed dedicated allocations found!");
```

`VMA_ASSERT_LEAK` defaults to `assert()`, and **Abort** on that dialog calls
`abort()` — reported by the debug CRT as `Debug Error! ... abort() has been called`.
That kills the process *before* `MemTrace::ProcessEnd` runs, **destroying the very
leak report the framework exists to produce**. The tell: output stops after
`Game: UnloadContent` and never reaches `Engine: shutdown clean`, putting the death in
the teardown chain (`graphicsPipeline` → `swapchain` → `allocator`).

So the implementation block redefines `VMA_ASSERT_LEAK` to a `Debug::out` line — VMA's
own documented extension point. This is not a test hack: here **MemTrace owns leak
reporting**, which makes VMA aborting both redundant and actively harmful. `VMA_ASSERT`
proper is left **fatal**, so a genuine VMA programming error still breaks into the
debugger. Format strings are capped (`%.120s`) because `Debug::out` runs through a
256-char `vsprintf_s` and overflowing it trips its own "Buffer too small" assert.

Note VMA defines its `VMA_ASSERT` / `VMA_ASSERT_LEAK` defaults **inside** its
implementation section, so the override lands first and no redefinition warning fires.

**Rule this makes concrete:** every VMA allocation must be freed *before*
`vmaDestroyAllocator`, and every Vulkan object before `vkDestroyDevice` — the
teardown-in-reverse order in §4 is what guarantees it.

---

## 14. Where things are (folder map)

The framework is **common to the whole solution**, so it sits at the root next to
`Libs/` — not inside the app.

```
Framework/                         <- shared by EVERY project (SharedItems)
  Framework.h                      Keenan + our rework + VULKAN FRAMEWORK.
                                   DECLARATIONS only (force-included everywhere)
  VulkanImpl.cpp                   volk + VMA bodies, barricaded. The ONE TU.
                                   Built by the app project, NOT by the shared
                                   Framework project (see §3)
Libs/          Math/ File/ AnimTime/   the Azul libraries (+ their Test/ suites)
               Manager/            DLink / DLinkMan / ManBase /
                                   CompareStrategyBase  (4th lib DLL)
Vulkan_Baseline/
  main.cpp                         standalone entry (int main)
  Source/Camera/                   ported from the DX11 engine (§16):
    Camera.{h,cpp}                 3D perspective + 2D ortho, frustum, viewport
    CameraNode.{h,cpp}             DLink node wrapping a Camera
    CameraNodeMan.{h,cpp}          manager + WASD/mouse ProcessInput
    CameraMan.{h,cpp}              older near-duplicate of CameraNodeMan
    CameraNull.{h,cpp}             null-object Camera
    CameraNodeCompareStrategyEnumName.{h,cpp}   find-by-Name strategy
  Engine/        Engine.{h,cpp}    Template-Method base + frame loop
  Game/          Game.{h,cpp}      derived; owns ShaderObject + ShaderWatcher
  Shader/
    ShaderCompiler.{h,cpp}         DXC HLSL->SPIR-V
    ShaderModule.{h,cpp}           wraps VkShaderModule
    ShaderObject.{h,cpp}           abstract; owns the pipeline
    ShaderObject_ColorByVertex.*   the triangle technique
    hlsl/ColorByVertex.vs/.ps.hlsl runtime-loaded HLSL
  ThreadManagement/
    ShaderWatcher.{h,cpp}          background file-watch actor + mailbox
  Vulkan/
    Utilities/
      Color.{h,cpp}, Colors.h      color system
    VulkanAPI/                     one class per subfolder:
      Instance/  Surface/  PhysicalDevice/  QueueFamily/
      LogicalDevice/  Swapchain/  GraphicsPipeline/
    Window/        Window.{h,cpp}  the Win32 window
premake5.lua                       build definition (single source of truth)
```

**Gone (absorbed into `Framework.h`):** `Vulkan/Utilities/VulkanUtilities.h` (volk
include + `VK_Try` + `Validation::`) and `Vulkan/VulkanAPI/Allocator/`
(`Allocator.{h,cpp}` → the `vk::VulkanAllocator` singleton). `VulkanImpl.cpp` moved
from `Vulkan/Utilities/` to `Framework/`. Engine headers that used to
`#include "VulkanUtilities.h"` now carry a one-line note instead — Vulkan types
arrive via the force-included `Framework.h`.

---

## 16. Camera port (DX11 → Vulkan)

Ported as-is; only the graphics-API-bound parts changed. Classes are `Neelam::` and
live in `Vulkan_Baseline/Source/Camera/`.

**Wired into `Game`:** `LoadContent` creates a 3D perspective (`CAMERA_0`) and a 2D
ortho (`CAMERA_1`) camera and registers both with `CameraNodeMan`; `Update` syncs them
to the window on resize then calls `CameraNodeMan::Update()` (input + rebuild
matrices); `Render` hands the active 3D camera to `GraphicsPipeline::Render`, which
now takes `const Camera*` and calls `Camera::SetActive(cmd)` for viewport + scissor
(`nullptr` falls back to the old full-swapchain viewport); `UnloadContent` destroys
the manager, which deletes the cameras.

**The matrices reach the shader via PUSH CONSTANTS.** `GraphicsPipeline::Render`
fills a `vk::ShaderMatrices { Azul::Mat4 view; Azul::Mat4 proj; }` from the camera and
calls `ShaderObject::SetMatrices` → `vkCmdPushConstants`, right after the pipeline
bind. Identity is pushed when there is no camera.

- **Why push constants:** a small block written straight into the command buffer —
  no descriptor set, no buffer, no allocation. Right tool for two per-frame matrices.
- **128-byte ceiling.** The spec only guarantees `maxPushConstantsSize >= 128`, and
  two `Mat4`s is exactly 128. A `static_assert` in `ShaderObject.h` holds the line.
  Adding a per-object world matrix means pre-multiplying `view*proj` on the CPU
  (frees 64 bytes) or moving to a UBO + descriptor set.
- **`row_major` is mandatory in the HLSL.** HLSL packs matrices **column-major** by
  default; `Azul::Mat4` is 4 row `Vec4`s and Azul is a **row-vector** library (`v * M`).
  Declaring `row_major float4x4` + `mul(vector, matrix)` makes the GPU do exactly what
  the C++ does. Without it every matrix silently arrives transposed.
- The range is declared in `ShaderObject::privBuildLayout` and must match the HLSL
  block — a mismatch is a validation error, not a silent wrong result. The layout is
  built once in `Create()` and **not** rebuilt by `Reload()`, so hot-reload keeps the
  same push-constant contract.
- Push constants live in the command buffer, so they are re-pushed **every frame** —
  the pool reset at the top of `Render` wipes the previous frame's.

The triangle's positions moved from Vulkan NDC to **world space, +Y up** (signs
flipped), because the camera's projection now does the Y flip.

| # | Was (DX11) | Now (Vulkan) | Why |
|---|---|---|---|
| 1 | `privSetViewState()` → `D3D11_VIEWPORT` + `RSSetViewports` | `SetActive(VkCommandBuffer)` → `vkCmdSetViewport` + `vkCmdSetScissor`; `setViewport()` only stores | Vulkan has no immediate context — viewport is dynamic state **recorded** into a command buffer. Matches the existing `ShaderObject::SetActive(cmd)` idiom. Also adds a scissor: D3D defaults to none, Vulkan requires one. |
| 2 | `projMatrix[m5] = d` | `= -d` (and ortho `= -2/(yMax-yMin)`) | **Vulkan NDC has +Y DOWN**, D3D/GL have +Y up. Without it everything renders vertically mirrored. |
| 3 | `Trans B(0,0,1) * Scale S(1,1,0.5)` depth remap | **unchanged** | The trap: this *looks* DX-specific but Vulkan clip Z is `[0,1]` too — OpenGL is the odd one out. Deleting it puts half the depth range behind the near plane. |
| 4 | `EngineDLLInterface.h`, `AZUL_ENGINE_LIBRARY_API` | removed | Camera is app source now, not a DLL export. |
| 5 | `StringThis.h` / `StringMe()` | local `switch` in `Camera::GetName()` | That header dragged in `Mesh` / `TextureObject` / `AnimMan` / `JointTable` / `HierarchyTable` — none exist here. |
| 6 | `Trace::out` | `Debug::out` | House rule §2; the app is in the thread tier. Manager keeps `Trace::out` — it is **not** in that tier. |

**Bug fixed (pre-existing, not DX-related):** `privCalcPlaneHeightWidth()` did
`tanf((fovy * MATH_PI / 180) * 0.5f)`, but `setPerspective()` already stores `fovy` in
radians — a second conversion made every frustum plane ~57× too small. The projection
matrix used `tanf(fovy/2)` correctly, so it never showed on screen; only the frustum
verts and collision normals were wrong.

**Fixed in the cleanup pass:** `getFieldOfView`/`setFieldOfView` only ever touched
`aspectRatio` — the names were simply wrong. Renamed to `getAspectRatio`/`setAspectRatio`
(the setter was byte-for-byte the `setAspectRatio` added for the resize path, so it is
gone). Nothing outside `Camera` called them. Actual FOV is `GetFovY`/`SetFovY`; the
`FieldOfView_Degs` parameter on `setPerspective` really is a FOV and keeps its name.

**Also removed:** `CameraMan.{h,cpp}` — a near-duplicate of `CameraNodeMan` that was
ported because it compiled, never wired to anything. `CameraNull` now earns its place
as `CameraNodeMan`'s compare node (heap-allocated, **not** `CameraNull::Instance()` —
`CameraNode` deletes the `Camera` it holds, and the singleton must not be deleted).

**Known benign quirk:** `updateCamera()` runs the perspective frustum math on the 2D
camera too, where `fovy` is 0 — so all eight frustum verts collapse to one point and
the six normals stay zero. Harmless: `Azul::Vec3::norm()` guards on
`lenSq > 1.0e-6f` and leaves a zero vector alone, so there are no NaNs. Nothing reads
the ortho camera's frustum. Same behaviour as the DX11 original.

---

## 17. Input

**There is no input manager, and none is needed.** `CameraNodeMan::ProcessInput()`
calls Win32 `GetKeyState` / `GetCursorPos` directly, which is exactly what the
reference engine (`R:\Work\Projects\AzulGameEngine`) does — it has no input manager
either, just `GetKeyState` in `Camera.h` and `GetAsyncKeyState` in `Game.cpp`.

`GetKeyState` is **message-queue based**: it reports the key state as of the last
message the calling thread retrieved. That makes it dependent on two things, both of
which already hold here:

1. **A correct pump.** `Window::ProcessMessages` does `PeekMessage(PM_REMOVE)` +
   `TranslateMessage` + `DispatchMessage` in a loop — standard and sufficient.
2. **Window focus.** No focus, no keyboard messages, no state. Click the render
   window first.

If input ever needs to work while unfocused, or without a pump, swap to
`GetAsyncKeyState` (reads hardware state directly) — that is the one-line fix, not a
manager.

Controls, once a camera is current: **WASD** move, **Q/E** down/up, **Z/C** zoom
(fovy), **left-drag** yaw, **right-drag** roll, **middle-drag** pitch.

### ESC quits — and it is window state, not camera input

`ESC` is handled in `Window::privHandleMessage` (`WM_KEYDOWN`), **not** in
`ProcessInput`. Quitting is window lifecycle, so it lives with the other lifecycle
messages, and it is deliberately routed through `WM_CLOSE` so it takes the *exact*
same path as the title-bar close button:

```
WM_CLOSE -> DestroyWindow -> WM_DESTROY -> PostQuitMessage
         -> ProcessMessages() false -> Engine::Run -> Shutdown()
```

That routing is the point. `PostQuitMessage(0)` straight from the key handler — what
the old Azul engine does from `WM_CHAR` — leaves the `HWND` alive and skips the
window's own teardown, so the two ways of exiting would not match, and only one of
them would be the clean-shutdown path the memory tracker signs off on.

Two deliberate differences from Azul:
- **`WM_KEYDOWN`, not `WM_CHAR`.** ESC only generates a `WM_CHAR` because its ASCII
  (27) coincides with `VK_ESCAPE`, and that route needs `TranslateMessage`.
- **Standalone only** (`!privIsChild`). Hosted in the WPF editor, ESC belongs to the
  host; `WM_DESTROY` already refuses to `PostQuitMessage` from a child, which would
  otherwise kill WPF's whole message loop (§4).

Any other key falls through to `DefWindowProc` — the `switch` `break`s rather than
returning 0, so normal keyboard handling is untouched.

---

## 18. Actor-model plan (from the FinalProject audio engine)

Source: `…\Vulkan Building\FinalProject` — `System/Circular`, `System/Handle`,
`Command/`, and the four `Thread - *` folders. Its load-bearing idea: **a thread
never touches another thread's data; it posts a `Command` into that thread's inbox,
and the owner executes it on its own thread.** Everything else is bookkeeping.

| Piece | Job |
|---|---|
| `CircularIndex` | power-of-2 index + mask; `operator++(int)` wraps, no modulo |
| `CircularData` | fixed `Command*[32]` + one mutex; `PushBack`/`PopFront` return `bool`. No allocation, no blocking, no exceptions |
| `QueueMan` | singleton owning one **inbox per thread** + `SendX(Command*)` |
| `Command` | abstract, `virtual void Execute() = 0`; concrete commands end with `delete this` |
| `Handle` / `HandleMan` | id+index into a mutex-guarded table + RAII `Handle::Lock` — "is the thing this message points at still alive?" |
| `XMain(atomic_bool &QuitFlag)` | create managers → pop/execute until quit → destroy → join children |

### Phases

**Phases 1 + 2 are DONE.** What landed:

```
ThreadManagement/
  Circular/  CircularIndex.{h,cpp}   power-of-2 wrapping index
             CircularData.{h,cpp}    the mailbox: fixed Command*[32] + mutex
             QueueMan.{h,cpp}        singleton; owns the engine inbox
  Command/   Command.h               abstract base, Execute()
             ShaderReload_Cmd.{h,cpp}
  ShaderWatcher.{h,cpp}              now posts instead of owning a mailbox
```

- **`std::queue` is gone** — the app has no STL containers left.
- **The inbox moved to the consumer.** It used to live in `ShaderWatcher`
  (producer-owned), so only that one producer could ever feed the engine. It is now
  `QueueMan`'s, and any actor can post.
- **The reload logic left `Game::Update`** and became `ShaderReload_Cmd::Execute()`.
  `Game` no longer participates in hot-reload at all.
- **`Engine::privDrainCommands()`** runs at the top of `Tic()`, before `Update`, so a
  reload posted last frame is live for this frame's `Render`. Bounded at
  `privMaxCommandsPerFrame = 8`.
- **`ShaderWatcher` moved `Neelam::vk` → `Neelam`** — it touches no Vulkan (§1).
- `ShaderWatcher::Start` now takes the `ShaderObject*` to rebuild, so it can
  construct the command. It never dereferences it on the watcher thread.

Ownership is enforced at all three points from the rules below: consumer deletes in
`Execute()`, sender deletes on a failed `SendEngine`, `QueueMan::~QueueMan` drains and
deletes the remainder (and logs a count if any were dropped).

Remaining phases:

1. ~~**Plumbing, no behaviour change.**~~ **done** — `CircularIndex`, `CircularData`,
   `Command`, `QueueMan` into `ThreadManagement/`. Only *our* inboxes (Engine,
   Watcher) — not their Audio/File/Aux set. Drop `Command`'s `Handle` member until
   Phase 3 needs it. Namespace `Neelam` (infra, not Vulkan — and `ShaderWatcher`
   being `Neelam::vk` today is a §1 violation worth fixing in the same pass).
   Keep it **app-side, not in the Manager DLL**: Command subclasses reference engine
   types, and a DLL boundary means an export macro on every one — the exact
   `MANAGER_LIBRARY_API` trap that broke the camera port (§1).
2. ~~**Convert `ShaderWatcher`, deleting `std::queue`.**~~ **done** — see above.
3. **`FileThread`** — first real actor. Reads/compiles off-thread, posts a completion
   command back; **all GPU work stays on the engine thread** (§9).
   `ThreadManagement/Handle/` — `Handle`, `HandleMan`, `Handle::Lock` — is **ported
   and ready**, but not yet wired to anything (see the correction below).

### ⚠ What `Handle` actually is (correction — it is not a weak pointer)

Worth getting right before designing around it. Every real use in the reference engine
looks like this, inside the owner's **own** method:

```cpp
void Snd::Play() { Handle::Lock lock(this->handle); if (lock) { ... } }
```

Always `this->handle` — never a handle to some *other* object. `Handle` is
non-copyable, so a referrer **cannot** hold one anyway. So:

- `Handle` is a **self-pin / re-entrancy guard**: "while I am running a method on
  myself, no other thread may invalidate me." `AcquireResource` deliberately leaves
  the table slot's mutex *held*; `ReleaseResource` (via `~Lock`) drops it.
- Cross-object safety in the reference does **not** come from handles. It comes from
  **manager lookup by ID** — `SndMan::Find(snd_id)` — which either returns a live
  object or nullptr. `Command`'s own `Handle` member guards the command, not its target.

Consequence for the FileThread: a command must **not** carry `ShaderObject*` and
"validate it with a handle" — that is not a thing this system does. **Decision: option
(a)** — shaders get a manager, and a cross-thread command carries a
`ShaderObject::Name` that the engine thread resolves through `ShaderObjectMan::Find()`
when the command executes. A technique destroyed while a compile was in flight
resolves to `nullptr` instead of a dangling pointer.

### Phase 3A progress

**Landed (engine-thread half + identity):**
- `ShaderObject::Name` enum, `GetName`/`SetName`, `privName`.
- `ShaderModule::CreateFromBlob(IDxcBlob*)` — the *only* part of a reload that touches
  Vulkan, split out from `privCompile()` so the disk+DXC half can move off-thread.
  Does not release the blob; caller owns it.
- `ShaderObject::ReloadFromBlobs(vs, ps)` — `vkDeviceWaitIdle` → swap modules →
  rebuild pipeline. A null blob keeps that stage's last-good module.
- `Handle` / `HandleMan` ported (not wired — see the correction above).

**Still to do:** `ShaderObjectNode`/`ShaderObjectMan` + compare strategy (mirror
`CameraNodeMan`), `FileThread` + its inbox in `QueueMan`, `File_CompileShader_Cmd`
(file thread: `ShaderCompiler::CompileFile` ×2 → blobs → post completion) and
`ShaderModulesReady_Cmd` (engine: `Find(name)` → `ReloadFromBlobs` → `Release` blobs),
then repoint `ShaderWatcher` at the file inbox.

### Who owns what (Phase 3 complete)

| Owner | Thing | Why |
|---|---|---|
| **Engine** | `QueueMan`, `FileThread`, `privDrainCommands` | infrastructure — no knowledge of shaders or cameras |
| **Game** | `ShaderObjectNodeMan`, `CameraNodeMan`, the techniques | **content**, and content is the Game's job |

The registry sits with `CameraNodeMan` in `Game::LoadContent`/`UnloadContent`, matching
old-Azul, where `ShaderObjectNodeMan::Create` is called from `scene1/2/3.cpp` — never
from the engine. `Engine.h` does not include it.

Dropping the registry inside `UnloadContent` is safe even with a compile in flight,
because from that point `Engine::Shutdown` only ever **deletes** queued commands and
never **executes** one — so no `Find()` can run against a dead registry. That property
is what the shutdown ordering in `Engine::Shutdown` exists to guarantee.

### ⚠ Tier reporting belongs in the APP, not the framework banner

Second instance of the §11 tier trap, worth its own note. Adding
`Thread Frmwk: %s` / `Vulkan Frmwk: %s` lines to the framework banner printed
**`--> DISABLED <--`** on a build where both are plainly on.

Why: the banner comes from `MemTrace::ProcessBegin`, run by the **first** module to
initialize — and a DLL's static initializers run before the exe's, so the printer is
always a lib DLL. The strings are compile-time constants evaluated **per module**, and
the libs are in neither tier. The banner was honestly reporting *its own* module.

Same rule as the vanished `VkLeak`: **never put tier-gated output in code the lib DLLs
compile.**

Verified by printing the tiers from `Engine::Initialize` (compiled in the app), which
correctly read `on/on/on` — then removed, having served its purpose. The tiers are
already provable from the log itself: a `thread(...) X: begin()` banner and the
`(ThreadName):` prefix only exist under the thread tier, and `VulkanAllocator: VMA
allocator created (tracked)` only under the Vulkan tier. **Do not re-add tier lines to
the framework banner.**
4. **Engine/Game thread split** — **deferred, deliberately.** Not blocked on
   stability; blocked on there being anything worth parallelising. Three reasons:
   - **`GetKeyState` is per-thread.** It reports the state as of the last message
     *the calling thread* retrieved, and `ProcessInput` runs inside `Update()`. Move
     `Update()` to a game thread and input silently dies — the same mechanism behind
     the earlier "no input worked" (§17). A split must keep input on the
     message-pumping thread and hand it over as posted state.
   - **The camera becomes a data race.** `Render()` reads `getViewMatrix()` /
     `getProjMatrix()` while `updateCamera()` rewrites them. Needs double-buffered
     camera output — a mutex would just serialise the two threads back together and
     buy nothing.
   - **There is no simulation load.** `Game::Update` is camera input plus two matrix
     rebuilds. The split would add races and synchronisation for no measurable gain.

   Revisit when the scene graph, animation, or physics give the game thread real work
   to overlap with rendering.

### Changes from the reference

- **Don't busy-spin.** `while(!AudioReadyFlag);` and the `while(!QuitFlag)` pop loop
  burn a full core each. Use `std::condition_variable` (same family as the
  `std::mutex`/`std::thread` §2 already allows) so an idle worker costs nothing.
- **Command ownership, or the leak tracker will catch it.** Convention is *consumer
  deletes* (`delete this`). Two holes: `PushBack` returning `false` (queue full) →
  **the sender must delete**; and anything still queued at shutdown is never executed
  → **drain-and-delete in `Destroy()`**. Easiest way to regress a clean run (§13).
- **`IsEmpty()`/`IsFull()` are a trap** — they lock, return, release, so the answer is
  stale before use. Call `PushBack`/`PopFront` and check the `bool`. Make them private.
- **One heap allocation per message.** Fine at reload rates, wasteful per-frame; the
  reserve-pool pattern already in `ManBase` is the fix when it matters.
- **Keep the `Aux` idea** — slow/untrusted callbacks off the owner's thread.

### ⚠ Why the per-frame `Drain()` stays a poll

Recurring question: *isn't draining the mailbox every frame costly — could the
watcher interrupt the render instead?*

**No, and it must not.** `Drain()` is one uncontended mutex lock (tens of ns) against
a 16.6 ms frame — ~0.0001%. The *reload* is the expensive part (`vkDeviceWaitIdle` +
pipeline rebuild, milliseconds), and it only runs when a message is actually waiting.

Interrupting is not merely unnecessary, it is **unsafe**: reloading mid-frame destroys
a `VkPipeline` that the in-flight command buffer already recorded via
`vkCmdBindPipeline` (`VUID-vkDestroyPipeline-pipeline-00765`). Polling at one known
point is not a workaround for lacking interrupts — it *is* the mechanism that makes
"all GPU work on the engine thread" mean anything. A condition variable does not help
here either: the engine thread is never idle. CVs are for worker threads.

**The real wins, aimed one layer lower — both now DONE:**

1. **✔ OS-driven change detection.** `ShaderWatcher` no longer polls. It blocks in
   `WaitForMultipleObjects` on `{FindFirstChangeNotification, quit-event}` —
   `FILE_NOTIFY_CHANGE_LAST_WRITE`, non-recursive. Idle cost is now exactly zero
   instead of a folder stat 4×/second, and a save is seen immediately rather than up
   to 250 ms late. Three details that matter:
   - The quit event is **manual-reset**, so `Stop()` can never be missed by a wait
     that had not started yet.
   - `FindNextChangeNotification` is called **before** the work, so a save landing
     during a compile is not lost.
   - A 60 ms **settle** (itself interruptible by the quit event) collapses the
     multi-write burst editors produce into one compile, and keeps DXC from reading a
     half-written file. `privNewestWriteTime` survives, but its job changed from
     *detection* to *dedupe*.

2. **✔ Dirty-flagged `updateCamera()`.** Every mutator sets `privDirty`;
   `updateCamera()` early-outs when clear. `setViewport` deliberately does **not**
   mark — it feeds `SetActive()` per frame, not the matrices. Input reaches it through
   `SetFovY` (marks directly) and `SetHelper` → `setOrientAndPosition` (marks).
   Note `Camera` is `Align16` with hand-balanced padding: the `bool` was paid for by
   dropping the trailing `pad[12]` to `pad[11]`, so `sizeof(Camera)` is unchanged.
   **Adjust the two together.**

3. Optional, not done: an `std::atomic<bool> hasWork` fast path before taking the
   mailbox mutex. Three lines, genuinely unnecessary, listed only because it is the
   correct form of "make the check free".

---

## 15. Next steps (roadmap)

- Vertex/index buffers + a projection matrix (aspect correction) — replaces the
  `SV_VertexID` hardcoded triangle; the `Projection` constant buffer from old-Azul.
- A scene graph (`PCSTree` / GameObjects) inside `Game` — slots behind
  `Update`/`Render`; the Engine loop never changes.
- The engine-thread / game-thread **actor model** (typed messages + handles) growing
  out of the `ShaderWatcher` mailbox pattern.
- The **C# / WPF editor**: flip the app to a `SharedLib` (DLL) with `extern "C"`
  exports (`Initialize(HWND)` / `Tic()` / `Shutdown()` / `GetHwnd()`) alongside the
  standalone `.exe`.
- Possible **CUDA** shading/compute later — the memory tracking already supports
  `USE_CUDA_FRAMEWORK` + `USE_VULKAN_FRAMEWORK` together.

<!-- End of File -->
