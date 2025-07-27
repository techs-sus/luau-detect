# luau-detect

an experimental C++ binary created to detect unoptimized code

## Building

Regular linux distros:

```bash
# assuming the current working directory is the repository, with all submodules cloned
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Minifier.CLI --config Release
```

Nix:

```bash
nix run ".?submodules=1#"
```
