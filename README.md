
# Get physical drive model name and serial by drive letter
### Description
Retrieve the model name and serial number of a physical drive on the Windows platform using native WMI queries, by specifying a logical drive letter for targeted identification. 

### Scpecifications
target platform: Windows 32 bit / 64 bit only
IDE: Visual Studio 2022
C++ Compiler - needs to support at least the C++14

The program does not rely on external libraries or third-party libraries.

### Buiding the project

    git clone https://github.com/tolotratlt/Drive-Model-Serial-byDriveLetter.git

Open with Visual Studio 2022 and build solution

### Other
If you want a standalone executable
- go to Project > Properties > Configuration Properties > C/C++ >Code Generation
- Change Runtime Library into Multi-Htreaded (instead of Mutli-Threaded DLL)


### Usage
programname.exe \<driverletter\>

### Debugging
To set the driverletter when debuggin, in VS 2022:
- right click on project name > Properties > Configuration Properties > Debugging
- set Command Arguments to the driverletter (for example C)

## Development
Want to contribute? Great!
