# Small GD Mods
A collection of small utility mods for Geometry Dash

# Download
Download for prebuilt DLLs can be found here [https://matcool.github.io/mods](https://matcool.github.io/mods), along with install instructions

# Compiling
If for some reason you want to compile this yourself, you'll need Git, CMake and the Visual Studio compiler

1. Clone the repo (make sure to do it recursively `--recursive`)
2. Configure CMake
```
cmake -G "Visual Studio 16 2019" -B build -DCMAKE_BUILD_TYPE=Release -T host=x86 -A win32
```
3. Build
```bash
cmake --build build --config Release --target ALL_BUILD
# you can switch out ALL_BUILD for any specific mod you want to compile
```