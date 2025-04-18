# STARBUILD Generator Tool

## Overview

This C++ program is an interactive command-line tool designed to help users create `STARBUILD` specification files for SFG OS packages managed by `starpack`. It prompts the user for various pieces of information about one or more software packages – such as names, version, descriptions, dependencies, source files, and build scripts – and then formats this information into a `STARBUILD` file in the current directory.

The tool handles both single-package and multi-package `STARBUILD` files, adjusting its prompts and output format accordingly.

## Features

* Interactively prompts for all necessary `STARBUILD` fields.
* Supports defining single or multiple packages within one `STARBUILD` file.
* Handles multi-line input for build scripts.
* Automatically formats dependencies, sources, and other lists into the correct `STARBUILD` array syntax.
* Trims whitespace from user inputs.
* Generates a ready-to-use `STARBUILD` file.

## Requirements

* A C++ compiler supporting C++11 or later (e.g., g++, clang++).
* CMake (for building using the provided `CMakeLists.txt`).
* Standard C++ libraries (iostream, fstream, sstream, string, vector, algorithm).

## Building

This project uses CMake. To build the tool:

1.  Make sure you have CMake and a C++11 compatible compiler installed.
2.  Create a build directory (recommended):
    ```bash
    mkdir build
    cd build
    ```
3.  Run CMake to configure the project:
    ```bash
    cmake ..
    ```
4.  Compile the project:
    ```bash
    make
    ```
This will create an executable named `StarbuildCreator` inside the `build` directory.

*(See the `CMakeLists.txt` content in the dedicated section below).*

## Usage

1.  Run the compiled executable (e.g., from the `build` directory):
    ```bash
    ./StarbuildCreator
    ```
2.  The tool will then prompt you interactively to enter information for the `STARBUILD` file. Follow the prompts carefully.
3.  For inputs that accept multiple values (like package names or dependencies), enter them separated by commas. Leading/trailing whitespace around items will be automatically trimmed.
4.  For script inputs (`prepare`, `compile`, `verify`, `assemble`), enter the script content line by line. To finish entering a script, type `END` on a new line by itself and press Enter.
5.  Once all prompts are answered, the tool will create a file named `STARBUILD` in the directory *from which you ran the executable*.

## Input Prompts Explained

* **`Enter package name(s) (comma separated):`**
    * The primary name(s) of the package(s) this `STARBUILD` file describes.
    * Example (single): `my-package`
    * Example (multiple): `core-utils, shared-lib, my-app`
* **`Enter package version:`**
    * The version string that applies to *all* packages defined in this file.
    * Example: `1.2.3`
* **`Enter package description / Enter description for package "..." :`**
    * If you entered a single package name, you'll be asked for one description.
    * If you entered multiple package names, you'll be prompted for a description for each package individually.
* **`Enter additional dependencies for package "..." (comma separated, or leave blank):`**
    * This prompt appears *only* if you entered multiple package names.
    * Allows specifying runtime dependencies specific to *that individual package*, in addition to any global dependencies. Leave blank if none.
* **`Enter global dependencies (comma separated, or leave blank):`**
    * Runtime dependencies required by *all* packages in this file.
    * Example: `shared-lib, glibc`
* **`Enter global build dependencies (comma separated, or leave blank):`**
    * Dependencies required only during the build process (compilation, etc.) for *all* packages.
    * Example: `gcc, make, cmake`
* **`Enter sources (comma separated, or leave blank):`**
    * Source files (URLs, local paths) needed to build the package(s).
    * Example: `https://example.com/src/my-package-1.2.3.tar.gz, patchfile.patch`
* **`Enter prepare() script (end with a line containing only END):`**
    * Multi-line shell script commands executed to prepare the sources (e.g., extract archives, apply patches).
* **`Enter compile() script (end with a line containing only END):`**
    * Multi-line shell script commands executed to compile the source code (e.g., `make`, `cmake --build .`).
* **`Enter verify() script (end with a line containing only END):`**
    * Multi-line shell script commands executed to run tests or checks (e.g., `make check`). (Optional content)
* **`Enter assemble() script / Enter assemble_***pkgName***() script (end with a line containing only END):`**
    * If you defined a single package, you'll enter one `assemble()` script. This script installs the built files into a staging area.
    * If you defined multiple packages, you'll enter an `assemble_pkgName()` script for *each* package. Each script installs files specific to that package.

## Output: `STARBUILD` File Format

The tool generates a `STARBUILD` file with the following structure (syntax resembles shell variables and functions):

```bash
# STARBUILD generated by CLI tool

# --- Core Package Information ---
package_name="pkg1" # Or: package_name=( "pkg1" "pkg2" )
package_version="1.2.3"

# --- Descriptions ---
# If single package:
description="This is a single package description."
# If multiple packages:
package_descriptions=( "Description for pkg1" "Description for pkg2" )

# --- Dependencies & Sources (Formatted as arrays) ---
dependencies=( "glibc" "shared-lib" ) # Global runtime dependencies
build_dependencies=( "gcc" "make" ) # Global build-time dependencies
sources=( "https://example.com/pkg1-1.2.3.tar.gz" )

# --- Package-Specific Dependencies (Only if multiple packages & provided) ---
# dependencies_pkg2=( "extra-lib-for-pkg2" ) # Example

# --- Build Stage Scripts ---
prepare() {
# User-entered prepare script lines...
# (ends without the "END" line)
}

compile() {
# User-entered compile script lines...
}

verify() {
# User-entered verify script lines...
}

# --- Assembly Stage Scripts ---
# If single package:
assemble() {
# User-entered assemble script lines...
}

# If multiple packages:
assemble_pkg1() {
# User-entered assemble script for pkg1...
}

assemble_pkg2() {
# User-entered assemble script for pkg2...
}

