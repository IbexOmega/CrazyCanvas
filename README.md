# Crazy Canvas
![Logo](/Assets/NoesisGUI/Textures/logo.png)
![Benchmark](https://github.com/IbexOmega/CrazyCanvas/workflows/Benchmark/badge.svg?branch=master)

Crazy Canvas is a Capture the Flag First Person Shooter with a twist, all enemies are invisible. The players are all equipped with paint shooting guns which can color everything in the world and the enemy players. By watching reflective surfaces like mirrors you can see your opponents and take the shot. When an enemy is covered in enough paint they die.

Crazy Canvas is designed from scratch during a 14 week period using a custom Vulkan engine (LambdaEngine) as a starting point.

## Trailer & Techniques Presentation
[![](http://img.youtube.com/vi/OKnG6HQkMr0/0.jpg)](http://www.youtube.com/watch?v=OKnG6HQkMr0 "Trailer")

## Download
[Latest Release](https://github.com/IbexOmega/CrazyCanvas/releases)

## Techniques
  - Mesh Painting
    - Server and Client synced
    - Procedural Textures and Normals
    - Tessellation
  
  - Ray Tracing
    - Glossy Reflections
    - Spatio-Temporal Denoising
  
  - PBR Shading
    - Image Based Lighting
    - Dynamic Point Lights
    - Dynamic Directional Lights
    - Dynamic Shadow Mapping
  
  - Skeletal Animation
    - With Blending
    
  - Anti-Aliasing
    - FXAA
    - TAA
    
  - Mesh Shaders
  
  - Particles
    - GPU Based Particle Simulation
    - Inline Ray Tracing
    
  - Marching Cubes
    - Meshes Animated on GPU
    - Reuse the same 5 meshes
    
  - Multiplayer
    - UDP and TCP supported
    - Reliable Packet Layer
    - Simple Security Layer
    - Dedicated and Integrated Server Support
    - Automatic LAN-server Discovery
    - Client-side Prediction
    - Authoritative Server
    - Movement Interpolation
    - Spectator Mode
    
  - Physics
    - Using Nvidia PhysX
    
  - In-Game Console
  
  - Render Graph
    - Graphical Node Editor
    - Synchronization of Graphics Resource
    - Creation of Graphics Resources
    - Automagic Simple Render Stage Creation
    - Custom Render Stage Support
    
  - Sounds
    - FMOD Studio
    - Spatial Sounds Support
    
  - GUI
    - Custom NoesisGUI Vulkan Implementation
    - Nameplates
    - Killfeed
    - Prompts
    - Lobby with Game Configuration and Chat
    - List of LAN Servers and Saved Servers
    - Ping (Like Apex Legends)
    - Indication of Picked up Flag
    
  - Basic Support for Bhop
  
  - Entity Component System
  
## Developers
Christoffer Andersson

Henry Bergström

Daniel Cheh

Alexander Dahlin

Herman Hansson Söderlund

Theo Holmqvist Berlin

Adrian Nordin

Simon Nylén

Tim Mellander

Jonathan Åleskog

## Coding standard
If you want to contribute, start by reading the [coding-style-document](CodeStandard.MD)

## Dependencies
* Get Vulkan Drivers [here](https://developer.nvidia.com/vulkan-driver)
* Get FMOD Engine [here](https://fmod.com/download)

## How to build

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
