# emerald-gen2_evk_console
A code example to run video stream with Teledyne Emerald-Gen2 Evaluation Kit

# Requirements

Software tested on PC with Windows 11 and following installation using Visual Studio Community 2022 (Desktop development with C++) :
* Windows 11 SDK
* MSVC v17.x build tools
* C++ CMake tools for Windows

Install openCV for windows in `C:` drive and check if *.lib binaries and *.cmake files are available here: `C:/opencv/build/x64/vc16/lib`. Modify the CMakeLists.txt if needed.

**IMPORTANT:** Add the link to openCV in the environment variable PATH: `C:/opencv/build/x64/vc16/bin`

Install Teledyne software `Evalkit-Emeraldgen2-1.0.1` on the PC. Modify the `CMakeLists.txt` with the correct path if needed.

**IMPORTANT:** Add the link to pigentl in the environment variable PATH: `C:\Program Files\Teledyne e2v\Evalkit-Emeraldgen2\1.0\pigentl\bin` and remove all reference to other Teledyne pigentl software.

To build this project and compile it, please use CMake into VSCode for example

# User Guide

This application open a display windows showing the video stream from the Evaluation Kit. The image has been resized before display to make it compatible with typical screen resolution.

`q` or `ESC`: Quit the application

This example runs the camera in Mono8 mode.
The exposure time is set to 20ms and can be modified directly in the code