# 🌐 FB-Tray-Wrapper
**A lightweight C++/WebView2 container for Facebook that lives in your system tray.**

This project demonstrates proficiency in **Win32 API**, **C++14**, and integration of modern web technologies into desktop environments.

---

## ✨ Features
* **Minimize to Tray**: Closing or minimizing the window hides it instantly to the system tray.
* **Fast Access**: Double-click the tray icon to toggle visibility.
* **Resource Optimized**: Automatically pauses WebView2 rendering when hidden to save CPU/RAM.
* **Smart Build System**: Universal batch script that detects Visual Studio (2019/2022) automatically.
* **Zero-Config Install**: Automatically installs to `%LOCALAPPDATA%` for a modern app feel.

---

## 🏗️ Project Structure
To keep the repository clean, external dependencies are managed manually:
```text

├── deps/                   # WebView2 SDK (Excluded from Git)
│   ├── include/            # C++ Headers
│   └── native/x64/         # Static Libraries
├── main.cpp                # Main Application Logic
├── app.ico                 # FB Icon
├── resource.rc             # Icon and Version Resources
├── build.bat               # Smart Build & Install Script
└── README.md
```
---

## 🛠️ Development Setup

### 1. Prerequisites
* **Windows 10/11**
* **Visual Studio 2019 or 2022** (with "Desktop development with C++" workload).
* **WebView2 SDK**:
    * Download the NuGet package from [NuGet.org](https://www.nuget.org/packages/Microsoft.Web.WebView2).
    * Rename `.nupkg` to `.zip` and extract it.
    * Copy `build/native/include` to `deps/include`.
    * Copy `build/native/x64` to `deps/native/x64`.

### 2. Building and Installation
Simply run the batch script from the project root:
`build.bat`

The script will compile the resources, link the executable, and move it to:
`%LOCALAPPDATA%\FB-Tray-Wrapper`

---

## 🖱️ Usage
* **Double-click** the tray icon to **Show** the window.
* **Right-click** the icon for the **Context Menu** (Show / Exit).
* **Minimize/Close** buttons: The app will intercept these calls and safely hide to the tray instead of closing.

---

## 👨�💻 About the Project
This is one of the projects for my portfolio. It highlights:
* Working with **Microsoft Edge WebView2** SDK.
* Deep understanding of **Windows Message Loop** and **Tray Notification Area**.
* Automation of build environments in Windows.

**Built by a developer who turns 64 this year and proves that engineering passion has no age!**

---
## 📄 License
MIT
