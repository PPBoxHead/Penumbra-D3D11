
# Penumbra-D3D11

**Penumbra-D3D11** it's a *DirectX 11* renderer/game engine and my first approach/try into the Direct3D API.
This project is meant to be a 3D FPS-based Game Engine written in C++17.


## Disclaimer

This project is meant to be a 3D FPS-based Game Engine, not a general purpose game engine like *Unity Engine*, *Unreal Engine* or *Godot Engine*. This project is under a propietary license at the moment, I don't think it will be released under MIT or another till I have a solid framework to use. It's done to learn DirectX 11 so, don't expect a lot and the code can look like a mess.

I have to change a lot of things on the current repository, add definitions and some core system features before work on the *sugar* which it's the D3D11 rendering stuff. I have to define the API, naming conventions and namespaces.

At the moment of writing this *(2024/12/25)*, there's not almost any feature to display. I can only name possible features and ideas if I got asked about this engine, but there's not much more yet.

**Soon it will pass a big change to match this ideal**


## Future (and current) Features

- [x] Scripted in C++17
- [x] Shader class with struct descriptors-based approach for easy declaration and initialization
- [x] Console Logger and File System (it's mostly almost finished) handling
- [ ] glTF/glb loader
- [ ] DirectX Shader Compiler integration
- [ ] Different GI and rendering solutions I would like to add and test (experimental)
    - Forward+ (Tiled Forward Rendering)
    - Stencil Shadow Volumes (on the style games like DOOM 3 and F.E.A.R. had)
    - DDGI (Dynamic Diffuse Global Illumination)


## Dependencies, Build and Run
Note that you need to have:
- Windows 10 SDK
- Git

Most of the 3rd-party dependencies are included as git submodules, so, once you download the repository, open the command prompt in the project folder and run `git submodule update --init --recursive`.
The only requirements to be able to build is have `Visual Studio 2022`, you download the repository project, open the `Penumbra-D3D11.sln` and hit `F5`.
  
The project is intended to be used on Windows 10 or later that supports DirectX 11, **There's not any support to build on platforms out of Windows.**

