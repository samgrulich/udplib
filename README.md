# UDP comms
Simple file sender and receiver

## Disclaimer
The files have been mostly tested on linux so that 
would be the preffered platform. But implementation
for windows is also aviable.

## Default params
TARGET_IP 127.0.0.1
LOCAL_PORT 5001  // for receiver port is 5002
TARGET_PORT 5002 // for receiver port is 5001

## Usage (linux)
Project directory contains `CMakeLists.txt`.
```
cmake . -B build
cd build 
make
```
Makes receiver and sender in the build directory.

## Usage (Windows)
There's a project solution in the project directory.
That way it can be opened with visual studio and ran in it.
!on windows it is required to uncomment either `#define SENDER` or 
`#define RECEIVER`


