# Build instructions

## Windows

- [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
  - Windows Universal C Runtime
  - C++ Build Tools core features
  - Windows 10 SDK (10.0.19041.0)
  - MSVC v143 - VS 2022 C++ x64/x86 build tools
- Python 3.6+ (make sure to check "Add Python to PATH" during installation)
- Qt 6.10.0

  ```pwsh
  $qt_path = "C:\Qt" # Replace with desired path
  python -m pip install aqtinstall
  python -m aqt install-qt windows desktop 6.10.0 win64_msvc2022_64 --autodesktop --outputdir $qt_path
  python -m pip uninstall aqtinstall
  ```

- Qt Visual Studio Tools 3.4.1

  ```pwsh
  $qt_path = "C:\Qt" # Replace with desired path
  Invoke-WebRequest -Uri https://download.qt.io/archive/vsaddin/3.4.1/qt-vsaddin-msvc2022-x64-3.4.1.vsix -OutFile $qt_path\vspackage.vsix
  Rename-Item $qt_path\vspackage.vsix $qt_path\vspackage.zip
  Expand-Archive $qt_path\vspackage.zip -DestinationPath $qt_path\ExternalLibraries\qtvsaddin
  Remove-Item $qt_path\vspackage.zip
  ```

- Set environment variables (run as admin)

  ```pwsh
  $qt_path = "C:\Qt" # Replace with desired path
  [System.Environment]::SetEnvironmentVariable("QtToolsPath", "$qt_path\6.10.0\msvc2022_64\bin", 'Machine')
  [System.Environment]::SetEnvironmentVariable("QtMsBuild",   "$qt_path\ExternalLibraries\qtvsaddin\QtMsBuild", 'Machine')
  ```

- Build

  ```pwsh
  msbuild -m -p:Configuration=DebugGUI
  msbuild -m -p:Configuration=DebugCLI
  ```

### Deployment

To run the compiled binaries, you need the Qt runtime libraries on your `PATH`. See `QtToolsPath` and add it to your `PATH` environment variable.

Alternatively, you can use the following command to copy the necessary DLLs to the same directory as the binary for sharing.

```pwsh
$qt_path\6.10.0\msvc2022_64\bin\windeployqt.exe --release --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw .\GhostServer.exe
```

## Linux

```bash
# Debian/Ubuntu
sudo apt-get install g++ qtbase6-dev 

# Arch
sudo pacman -S gcc qt6-base
```

```bash
make -j$(nproc)
```
