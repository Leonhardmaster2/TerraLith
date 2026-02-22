# CLErrorLookup

CLErrorLookup is a C++ lightweight library for decoding OpenCL error codes into human-readable strings and descriptions.

## Features

- Retrieve short or detailed descriptions of OpenCL error codes.
- Throw exceptions with detailed error messages for easier debugging.

## Getting the Sources

Clone the repository using `git`:

```bash
git clone git@github.com:otto-link/CLErrorLookup.git
```

## Build and Test

### Prerequisites
- A C++ compiler with C++17 support or higher.
- CMake version 3.15 or newer.

### Build Instructions
1. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
2. Run CMake to configure the build:
   ```bash
   cmake ..
   ```
3. Build the project using `make`:
   ```bash
   make
   ```

### Run Tests
After building, you can run the tests to verify the build:
```bash
bin/test
```

## CMake Integration

To integrate CLErrorLookup into your CMake-based project, follow these steps:

1. Add the library to your project's `CMakeLists.txt`:
   ```cmake
   add_subdirectory(CLErrorLookup)
   target_link_libraries(your_project_target clerrorlookup)
   ```
2. Link the `clerrorlookup` library to your target, as shown above.

## Usage Example

### Basic Example
```cpp
#include <cl_error_lookup.hpp>

// OpenCL error code
int code = -42;

// Get a short description
std::string short_desc = clerror::get_error_str(code);  // "CL_INVALID_BINARY [-42]"

// Get a detailed description
std::string full_desc = clerror::get_error_full(code);  // See output example below

// Throw an exception with a detailed error message
clerror::throw_opencl_error(code);
```

### Example Output
For an error code `-42`:
- **Short description** (`get_error_str`):
  ```
  CL_INVALID_BINARY [-42]
  ```
- **Detailed description** (`get_error_full`):
  ```
  CL_INVALID_BINARY [-42]: If the program is created with clCreateProgramWithBinary and devices listed in device_list do not have a valid program binary loaded. The provided binary is unfit for the selected device. Compile-time Errors (driver-independent).
  ```
  
## Attribution

The OpenCL error code descriptions used in this library are based on the detailed information provided by this post: [OpenCL Error Codes - StreamHPC](https://streamhpc.com/blog/2013-04-28/opencl-error-codes/).

## Contributing

If you find any incorrect or missing error codes, please use the [GitHub Issues](https://github.com/otto-link/CLErrorLookup/issues) to propose modifications. Contributions are always welcome and help ensure the accuracy and usefulness of the library.

## License

This project is licensed under the GPL-3.0 License. See the `LICENSE` file for details.
