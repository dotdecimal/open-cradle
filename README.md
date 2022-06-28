# ALIA & CRADLE Open Source C++ Project

Welcome to ALIA & CRADLE

This is an open source software project provided under the terms of the MIT License
as given below. This project provides various facilities for:
1) developing functional, reactive user interface Windows software products;
2) interfacing with the Thinknode (R) cloud services platform in C++;
3) performing various algebraic, geometric, and image processing tasks in C++.

The current version provided herein is a pre-release state that is not intended
to be a final built end user application at this time. Instead the provided code can be
used as needed to expedite development. As this project matures, this code base
will be more fully partitioned, cleaned, and organized and complete working demo
projects will be added to help new users gain a full understanding of the system.

Additional Notes:
1) Functionality of this repository has been reduced to protect proprietary algorithms and processes. This reduced functionality is tagged with the comment ```// [open-cradle]```. The function declarations and implementations are left in tack to facilitate repo maintainability and compiling. But any code needed to be removed has been flagged with this comment and an exception.
2) This repository assumes the user will be logging into the Thinknode cloud used by cradle. This functionality may be removed by the developer and may be removed by the repository maintainer at a later time. Until then .decimal will need to create and allow users to have accounts on the Thinknode cloud in order to use these applications.

## Compiling Alia Demo or GUI Demo Application

Currently the GUI-Demo application is the only compile-able user interface application. This provides a demonstration of the cradle/alia UI framework and a proof of concept for being able to compile and use this project.

Additional Notes:
1) The GUI-Demo App is a bit outdated in the alia UI widgets, state handling, and accessor management. Newer methods and code examples may be provided and updated by the developer at a later time.

Use the following steps to compile the this application.

Note: Currently this repository only supports Visual Studio 2015 SP3.

1) Environment Setup
  * Install or manually build the dependent libraries to C:\libs-vc14-x64 (note: these can also be provided by .decimal upon request)
  * Create an environment variable called ASTROID_EXTERNAL_LIB_DIR and set it to the full path of the libs-vc14-x64 directory
  * Add the bin/ subdirectory of libs-vc14-x64/ to your path so that the DLLs can be found.
2) Checkout the open-cradle repository to a new folder called 'src' such that the folder structure matches:
  * ../open-cradle/
    * src/
      * alia_demo
      * ...
3) Copy the src/scrpts folder up a directory to a new 'scripts' folder such that the folder structure matches:
  * ../open-cradle/
    * scripts/
    * src/
      * alia_demo
      * ...
4) Open a VS2015 x64 Native Tools Command Prompt and run the following from the open-cradle root folder a level above the checked out src folder (Note: if missing the Native Tools Command Prompt make sure to modify the VS install to install the Visual C++ optional Programming Language):
  * ```src\scripts\init-x64-builds.bat```
  * No cmake errors should be present during this step
  * This will create the following directory structure:
build/
    * x64-release/ - the command-line 'release' build - 'release' is a misnomer here because this build is optimized for fast builds and acceptable testing performance, so it's meant to be used for iterative development, not for actual release. However, it corresponds to what CMake and VS both call 'release', so for now it's left that way.
    * x64-debug/ - the command-line debug build - This is used when you really need the debugger. It's slower and takes slightly longer to build.
    * x64-rwdi/ - the command-line 'release with debug info' build - This is actually used for release and thorough testing. It's optimized for run-time performance.
    * x64-vs14/ - the Visual Studio build - Note that this is currently a little shaky. All the projects are in one solution, and the VS build system apparently doesn't understand their dependencies, so you have to be sure to build them in the proper order. The VS build is also much slower, so it's recommended to just build via ninja and edit/debug within VS. (Eventually we'll have VS invoke ninja internally.)
5) Run the /open-cradle/scripts/ninja-alia-demo.bat or ninja-gui-demo.bat to compile and run the GUI Demo application

## LICENSE

Copyright (c) 2012 - present  .decimal, LLC & Partners HealthCare

Licensed under the [MIT](License.txt) License.
