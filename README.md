
# DartSyncGUI

DartSyncGUI is a Windows-based GUI application for managing versioned backups of files and directories. It provides an intuitive interface for configuring backup source and destination paths, file type filters, and backup frequencies (one-time, daily, or monthly).

## Features

- **Versioned backups** with timestamps to prevent overwriting.
- File type filtering (e.g., `.txt`, `.dll`).
- Maximum file size limit to exclude large files.
- Console display within the GUI for real-time feedback.
- Scheduled automatic backups.

---

## Installation and Compilation

### Prerequisites

- **Windows OS**
- [CMake](https://cmake.org/download/) (version 3.15+)
- A C++20-compatible compiler (e.g., GCC, Clang, or MSVC)

### Steps to Compile

1. **Clone the repository**:
   ```bash
   git clone https://github.com/yourusername/DartSyncGUI.git
   cd DartSyncGUI
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Generate build files using CMake**:
   ```bash
   cmake ..
   ```

4. **Build the executable**:
   ```bash
   cmake --build . --config Release
   ```

---

## Usage Instructions

1. **Run the application**: 
   Once compiled, run `DartSyncGUI.exe` to launch the GUI.

2. **Select Source and Destination**:
   - Click **Pick** buttons to choose the source folder (files to back up) and the destination folder.

3. **Set Backup Frequency**:
   - Options: `Once`, `Daily`, or `Monthly`.

4. **Optional Filters**:
   - Enter file extensions (e.g., `.txt .docx`) in the "File Types" field.
   - Set a maximum file size limit (in MB). A value of `0` means no limit.

5. **Start the Backup**:
   - Click **Start Backup** to run the process. The console in the GUI will display progress and messages.

---

## Project Structure

```
├── src/
│   ├── BackupManager.cpp/h       # Core logic for handling file backups
│   ├── ConsoleRedirect.cpp/h     # Redirects console output to GUI console
│   └── main_gui.cpp              # Main GUI entry point (WinMain)
├── CMakeLists.txt                # CMake configuration
├── LICENSE                       # Open-source license file
├── .gitignore                    # Git ignored files
└── README.md                     # Project documentation
```

---

## Dependencies

- **Windows API**: Required for GUI elements (e.g., `CreateWindowW`, `SHBrowseForFolderW`).
- **Shell32**: Linked for folder browsing dialogs.

---

## Contributing

Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a new branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Submit a pull request describing your changes.

---

## License

This project is licensed under the terms specified in the `LICENSE` file.
