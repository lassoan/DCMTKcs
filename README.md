# DCMTKcs

Community-supported external modules for [DCMTK](https://dicom.offis.de/dcmtk).

These modules are not part of the official DCMTK distribution. They are built
alongside DCMTK by placing them in DCMTK's `ExternalModules/` directory and
listing them in the `DCMTK_MODULES` CMake variable.

## Modules

### dcmjp2kcs — JPEG 2000 codec

Implements DICOM JPEG 2000 compression and decompression using the
[OpenJPEG](https://www.openjpeg.org/) library. Supports both transfer syntaxes:

| Transfer Syntax | UID | Description |
|---|---|---|
| JPEG 2000 Lossless Only | 1.2.840.10008.1.2.4.90 | Lossless compression |
| JPEG 2000 | 1.2.840.10008.1.2.4.91 | Lossy or lossless compression |

#### Command-line tools

- `dcmcjp2kcs` — compress a DICOM file to JPEG 2000 transfer syntax
- `dcmdjp2kcs` — decompress a JPEG 2000 DICOM file to uncompressed transfer syntax

#### Usage

Key classes:

- `DJ2KDecoderRegistration` / `DJ2KEncoderRegistration` — call `registerCodecs()` at application startup and `cleanup()` at shutdown to install the codecs into DCMTK's global codec list
- `DJ2KDecoderBase`, `DJ2KLosslessDecoder`, `DJ2KDecoder` — decoder implementations
- `DJ2KEncoderBase`, `DJ2KLosslessEncoder`, `DJ2KEncoder` — encoder implementations
- `DJ2KRepresentationParameter` — controls encoding parameters (lossless flag, compression ratio)
- `DJ2KCodecParameter` — controls codec behaviour (UID creation, planar configuration, offset table, fragment size)

Register the codecs once at application startup, before any DICOM data is read
or written:

```cpp
#include <dcmtk/dcmjp2kcs/dj2kdecode.h>
#include <dcmtk/dcmjp2kcs/dj2kencode.h>

// Register JPEG 2000 codecs
DJ2KDecoderRegistration::registerCodecs();
DJ2KEncoderRegistration::registerCodecs();

// ... use DCMTK as normal; JPEG 2000 files are now transparently decoded ...

// Deregister at shutdown
DJ2KDecoderRegistration::cleanup();
DJ2KEncoderRegistration::cleanup();
```

## Building

### Standalone build

The simplest way to build the command-line tools is to use top-level CMakeLists.txt file in this repository, which will automatically:

- Download and build OpenJPEG
- Download and build DCMTK (a patched version, which allows cleanly specifying external modules)
- Build dcmjp2kcs as an external DCMTK module

#### Steps

1. Clone this repository:
   ```
   git clone https://github.com/YOUR_ORG_OR_USER/DCMTKcs.git
   cd DCMTKcs
   ```

2. Configure the superbuild:
   ```
   cmake -S . -B build
   ```

3. Build everything:
   ```
   cmake --build build --config Release
   ```

This will build OpenJPEG, DCMTK, and dcmjp2kcs. The resulting DCMTK build (with dcmjp2kcs) will be in `build/dcmtk-install`.

### Library build

To add JPEG2000 support to a project that already uses DCMTK, add building of OpenJPEG as an external project and fetch the source code of DCMTKcs, then update the DCMTK build to use OpenJPEG and specify DCMTKCS/dcmjp2kcs as an external module. Top-level CMakeLists.txt file of this project demonstrates how to do this.

### In-place build

You can integrate dcmjp2kcs into an existing DCMTK build by copying it into the root of the DCMTK source tree and following the steps below:

1. Copy (or symlink) the `dcmjp2kcs` folder into the root of your DCMTK source directory, e.g.:
   ```
   <DCMTK root>/dcmjp2kcs/
   ```
2. Add the module path to the `DCMTK_MODULES` CMake variable when configuring DCMTK:
   ```
   -DDCMTK_MODULES="ofstd;oflog;...;dcmjp2kcs"
   ```
3. Enable OpenJPEG support and point CMake to an OpenJPEG installation:
   ```
   -DDCMTK_WITH_OPENJPEG=ON
   -DOpenJPEG_DIR=/path/to/openjpeg/lib/cmake/openjpeg-2.5
   ```

## Dependencies

- [DCMTK](https://dicom.offis.de/dcmtk) — modules `ofstd`, `oflog`, `dcmdata`, `dcmimgle`, `dcmimage`
- [OpenJPEG](https://www.openjpeg.org/) ≥ 2.3 (required at build time when `DCMTK_WITH_OPENJPEG=ON`)

## License

Licensed under the BSD 2-Clause License. See the `LICENSE` file for details.

The OpenJPEG library used by `dcmjp2kcs` is also distributed under the BSD 2-Clause
License. See `dcmjp2kcs/docs/readme.txt` for the full OpenJPEG license text.
