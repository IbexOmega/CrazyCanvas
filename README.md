# Lambda Game Engine
![Benchmark](https://github.com/IbexOmega/CrazyCanvas/workflows/Benchmark/badge.svg?branch=master)

Lambda Game Engine

A game engine that supports Win32 and MacOS

Tested on:
* Windows 10
* macOS 10.15

### How to contribute
If you want to contribute, start by reading the [coding-style-document](CodeStandard.MD)

### Dependencies
* Get Vulkan Drivers [here](https://developer.nvidia.com/vulkan-driver)
* Get FMOD Engine [here](https://fmod.com/download)

### How to build

* Clone repository
* May need to update submodules. Perform the following commands inside the folder for the repository
```
git submodule update
```
* Then use the included build scripts to build for desired IDE
```
Premake vs2017.bat
Premake vs2019.bat
Premake xcode.command
```
* Project should build if master-branch is cloned
