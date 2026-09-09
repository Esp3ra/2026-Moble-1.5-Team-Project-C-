# SFML dependency

- Version: SFML 3.1.0
- Package: official Visual C++ 17 (2022), 64-bit Windows dynamic libraries
- Download page: https://www.sfml-dev.org/download/sfml/3.1.0/
- Package URL: https://www.sfml-dev.org/files/SFML-3.1.0-windows-vc17-64-bit.zip
- Downloaded: 2026-09-08
- SHA-256 of the downloaded package: `2D32D05591AE218EEAE67DA32C84435877DB52D6E276D60CC2CC9C8ADB5F4152`
- Original license: `SFML-3.1.0/license.md`

The unmodified official package is extracted locally under `SFML-3.1.0/`.
The application uses its graphics, window, and system libraries.
Debug uses `-d` libraries and DLLs; Release uses the release libraries and DLLs.
The application itself is built with Visual Studio 2026's v145 toolset and C++17.
The build copies the required SFML DLLs and license next to the executable.

Run `Prepare-Dependencies.ps1` to restore this exact package when copying the source
without the ignored dependency directory. `Build.ps1` also restores it if the whole
dependency directory is absent. The build validates the original hashes of all
nine top-level SFML headers. If existing headers are missing or changed, it stops
with an error and preserves those files; it never silently replaces local changes.
