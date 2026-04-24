# How to Create compile_commands.json for Each Subproject

This guide explains what `compile_commands.json` is, how it is produced, and how to generate it yourself for each part of this repo so that clangd (and other tools) can provide accurate IntelliSense and diagnostics.

---

## 1. What is compile_commands.json?

`compile_commands.json` is a **JSON Compilation Database**: a file that lists every source file in your project and the **exact compiler command** used to compile it (compiler binary, include paths, defines, flags). Tools like **clangd**, **clang-tidy**, and **rtags** use it to:

- Resolve `#include` and show correct diagnostics
- Apply the right language standard and defines
- Understand your build without guessing

If a file is **not** in the database, the language server has no compile command and often shows false errors (missing includes, unknown symbols).

The file is an array of objects. Each object has at least:

- **`directory`** – working directory when the command runs
- **`command`** – full compiler invocation (e.g. `clang++ -I... -D... -c file.cpp`)
- **`file`** – absolute path to the source file

---

## 2. How is it created?

### 2.1 CMake (primary method)

When you configure a project with **CMake**, you can ask it to write this database:

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
```

After configuration, CMake writes `build/compile_commands.json`. It does **not** require a full build; configuration is enough.

- **Android/Gradle + CMake**: The Android Gradle plugin already passes `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` when it runs CMake. So the file is generated in the CMake build directory (e.g. `app/.cxx/Debug/.../x86_64/compile_commands.json`) during a normal build or sync.

### 2.2 Other build systems

- **Make/ninja (non-CMake)**: Use **Bear** (`bear -- make`) or **compiledb** to record the actual compile commands and emit a `compile_commands.json`.
- **Visual Studio / MSVC**: There is no built-in export. Options:
  - Add a **CMakeLists.txt** that mirrors the VS project and run CMake with `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.
  - Use a converter (e.g. **vcxproj2cmake**) to generate CMake, then run CMake.
  - Or provide **compile flags** only (e.g. `.clangd` or `compile_flags.txt`) so clangd at least has includes and defines.

---

## 3. Subproject 1: depth_camera (Android + CMake)

**Build system:** Android Gradle + CMake (`app/src/main/cpp/CMakeLists.txt`).

### Option A: Let Gradle generate it (easiest)

1. Open the project in Android Studio or run from the repo root:
   ```bash
   cd CppPlugin/depth_camera
   ./gradlew :app:externalNativeBuildMl2Debug
   ```
2. Gradle runs CMake with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`. The database is created at:
   ```
   CppPlugin/depth_camera/app/.cxx/Debug/<hash>/x86_64/compile_commands.json
   ```
   The `<hash>` folder name depends on your Gradle/AGP version (e.g. `221u3xdb`).

3. **Copy it to a fixed location** (e.g. app `cpp` folder or project root) so your IDE always finds it:
   ```bash
   cp "CppPlugin/depth_camera/app/.cxx/Debug/221u3xdb/x86_64/compile_commands.json" \
      CppPlugin/depth_camera/compile_commands.json
   ```
   (Replace `221u3xdb` with the folder name you see under `.cxx/Debug/`.)

### Option B: Run CMake yourself (no Gradle)

Use the same arguments Gradle uses so the result matches the real build:

```bash
cd /path/to/magic-leap-2-navigation

# Set MLSDK if your CMakeLists uses it
export MLSDK=/path/to/MagicLeap/mlsdk/v1.12.0

cmake \
  -S CppPlugin/depth_camera/app/src/main/cpp \
  -B CppPlugin/depth_camera/app/build_compile_commands \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_SYSTEM_NAME=Android \
  -DCMAKE_SYSTEM_VERSION=29 \
  -DANDROID_PLATFORM=android-29 \
  -DANDROID_ABI=x86_64 \
  -DCMAKE_ANDROID_ARCH_ABI=x86_64 \
  -DANDROID_NDK=$ANDROID_NDK \
  -DCMAKE_ANDROID_NDK=$ANDROID_NDK \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DCMAKE_MAKE_PROGRAM=$(which ninja) \
  -GNinja
```

Then:

```bash
cp CppPlugin/depth_camera/app/build_compile_commands/compile_commands.json \
   CppPlugin/depth_camera/compile_commands.json
```

You can add `build_compile_commands/` to `.gitignore` if you don’t want to commit it.

---

## 4. Subproject 2: depth_navigation (Android + CMake)

**Build system:** Android Gradle + CMake (`app/src/main/cpp/CMakeLists.txt`). Same idea as depth_camera.

### Option A: Let Gradle generate it

1. Build the native part:
   ```bash
   cd CppPlugin/depth_navigation
   ./gradlew :app:externalNativeBuildMl2Debug
   ```
2. Find the generated file under:
   ```
   CppPlugin/depth_navigation/app/.cxx/Debug/<hash>/x86_64/compile_commands.json
   ```
3. Copy to a stable path, e.g.:
   ```bash
   cp "CppPlugin/depth_navigation/app/.cxx/Debug/221u3xdb/x86_64/compile_commands.json" \
      CppPlugin/depth_navigation/compile_commands.json
   ```

### Option B: Run CMake yourself

Same pattern as depth_camera; the source dir and build dir are different:

```bash
cd /path/to/magic-leap-2-navigation
export MLSDK=/path/to/MagicLeap/mlsdk/v1.12.0

cmake \
  -S CppPlugin/depth_navigation/app/src/main/cpp \
  -B CppPlugin/depth_navigation/app/build_compile_commands \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_SYSTEM_NAME=Android \
  -DCMAKE_SYSTEM_VERSION=29 \
  -DANDROID_PLATFORM=android-29 \
  -DANDROID_ABI=x86_64 \
  -DCMAKE_ANDROID_ARCH_ABI=x86_64 \
  -DANDROID_NDK=$ANDROID_NDK \
  -DCMAKE_ANDROID_NDK=$ANDROID_NDK \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DCMAKE_MAKE_PROGRAM=$(which ninja) \
  -GNinja
```

Then copy:

```bash
cp CppPlugin/depth_navigation/app/build_compile_commands/compile_commands.json \
   CppPlugin/depth_navigation/compile_commands.json
```

---

## 5. Subproject 3: HoloLens2-IRTracking-main (Visual Studio, no CMake)

**Build system:** Visual Studio (`.vcxproj`). CMake does not run here, so there is no automatic `compile_commands.json`.

You have two practical options.

### Option A: Add a CMakeLists.txt and use CMake (recommended for full accuracy)

Add a small CMake project that compiles the same sources with the same includes and defines as the Release|ARM64 configuration. Then run CMake with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and use the generated database.

1. **Create** `HoloLens2-IRTracking-main/CMakeLists.txt` that:
   - Defines a library or executable with the same `.cpp` files as the vcxproj: `IRToolTrack.cpp`, `pch.cpp`, `HL2IRToolTracking.cpp`, `TimeConverter.cpp` (and any generated file you can mimic or leave out for IDE purposes).
   - Adds include directories: e.g. `OpenCV/include`, and any Windows SDK / C++/WinRT paths you need.
   - Sets defines: `_XM_NO_INTRINSICS_;WIN32_LEAN_AND_MEAN;NOMINMAX;NODRAWTEXT;_ENABLE_EXTENDED_ALIGNED_STORAGE;_UNICODE;UNICODE` and C++17.
   - Targets ARM64 or x64 so the compiler and paths match your real build.

2. **Configure** (on Windows, with Visual Studio or a toolchain that matches the vcxproj):
   ```bat
   cd HoloLens2-IRTracking-main
   cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   ```
   Then use `build/compile_commands.json` (or copy it to `HoloLens2-IRTracking-main/compile_commands.json`).

This requires a bit of CMake work but gives you a database that matches how the code is compiled.

### Option B: Give clangd flags via .clangd (no compile_commands.json)

If you only need IntelliSense and don’t need a full compile database, you can add a **.clangd** file in `HoloLens2-IRTracking-main/` so clangd uses the right includes and defines for that folder.

Example **HoloLens2-IRTracking-main/.clangd**:

```yaml
CompileFlags:
  Add:
    - -std=c++17
    - -DWIN32_LEAN_AND_MEAN
    - -DNOMINMAX
    - -D_UNICODE
    - -D_UNICODE
    - -I.
    - -IOpenCV/include
  Remove: []
```

Use **absolute paths** for `-I` if opening the repo from a parent folder (e.g. `-I/Users/.../HoloLens2-IRTracking-main/OpenCV/include`). This reduces false “missing include” errors for that subproject even without `compile_commands.json`.

---

## 6. Using multiple compile_commands in one workspace

You have several subprojects; each can have its own `compile_commands.json` in its directory. **clangd** looks for `compile_commands.json` in the file’s directory and then in parent directories. So:

- Put **depth_camera**’s database at `CppPlugin/depth_camera/compile_commands.json` when working under `CppPlugin/depth_camera/`.
- Put **depth_navigation**’s at `CppPlugin/depth_navigation/compile_commands.json` when working under `CppPlugin/depth_navigation/`.
- Put **HoloLens2**’s at `HoloLens2-IRTracking-main/compile_commands.json` (or use `.clangd` as above).

Your root **.vscode/settings.json** has:

```json
"clangd.arguments": ["--compile-commands-dir=${workspaceFolder}", ...]
```

So clangd only looks at the **workspace root** for one single database. To support multiple subprojects you can:

1. **Merge** all compile_commands into one big `compile_commands.json` at the repo root (script that concatenates the arrays and deduplicates by file path), or  
2. **Change** clangd config so it doesn’t force one directory: remove `--compile-commands-dir=${workspaceFolder}` and rely on clangd’s default behavior (search from each file’s directory upward for `compile_commands.json`). Then each subproject’s `compile_commands.json` will be used when you edit files in that subproject.

Option 2 is usually simpler: remove the override and place one `compile_commands.json` (or `.clangd`) per subproject as above.

---

## 7. Quick reference

| Subproject           | Build system   | How to get compile_commands.json |
|----------------------|----------------|-----------------------------------|
| depth_camera         | Gradle + CMake | Build with Gradle or run CMake manually; copy from `app/.cxx/Debug/.../x86_64/` or from your `-B` build dir. |
| depth_navigation     | Gradle + CMake | Same: Gradle build or manual CMake; copy from `app/.cxx/Debug/.../x86_64/` or from your `-B` build dir. |
| HoloLens2-IRTracking | Visual Studio  | Add CMakeLists.txt and run CMake with `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, or use `.clangd` with CompileFlags. |

**Core idea:** For CMake projects, set `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and use the `compile_commands.json` produced in the CMake build directory. For VS-only projects, either introduce CMake and export compile commands, or feed flags to clangd via `.clangd`.
