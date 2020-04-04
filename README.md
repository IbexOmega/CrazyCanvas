# Lambda Game Engine
Lambda Game Engine

A game engine that supports Win32 and MacOS

Tested on:
* Windows 10
* macOS 10.15

### Conntribute
If you want to contribute, start by reading the [coding-style-document](CodeStandard.MD)

### How to build

* Clone repository
* May need to update submodules. Perform following commands inside the folder for the repository
```
git submodule update
```
* Then use the included build scripts to build for desired
```
Premake vs2017.bat
Premake vs2019.bat
Premake xcode.command
```
* Project should build if master-branch is cloned
