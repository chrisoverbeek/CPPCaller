# CPPCaller
C++ executable that will call a rust library

## Building the Project

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running the Tool

```bash
./build/cppcaller
```

Or with command line arguments:

```bash
./build/cppcaller arg1 arg2 arg3
```

## Current Status

The tool currently calls a rust library to format a message and writes the result to a SQLite db in the working directory.
