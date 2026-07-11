@echo off
REM Build the C++ engine using CMake and launch the Streamlit dashboard.
setlocal enabledelayedexpansion

if not exist build (
    mkdir build
)
cd build
cmake ..
if errorlevel 1 (
    echo CMake configuration failed.
    exit /b 1
)
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
cd ..
python -m streamlit run dashboard.py
