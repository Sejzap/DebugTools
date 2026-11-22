\# DebugTools - 系统功能调试工具箱



一个基于 C++ 和纯 Win32 API 编写的轻量级 Windows 系统调试与功能快捷启动工具。



\## 功能特性

\- 🚀 一键打开 \*\*命令提示符\*\* (cmd.exe)

\- 🛠️ 一键打开 \*\*注册表编辑器\*\* (regedit.exe)

\- 📝 一键打开 \*\*记事本\*\* (notepad.exe)

\- 🔒 一键打开 \*\*组策略编辑器\*\* (gpedit.msc) \*(注：家庭版系统不支持)\*

\- ⚡ 纯原生 API 编写，无第三方依赖，极小体积

\- 🖥️ 支持高 DPI 缩放，界面清晰

\- 🛡️ 内置权限不足时的 UAC 自动提权处理



\## 编译说明

本项目使用 MSVC 编写。

可以使用 Visual Studio 打开工程直接编译（建议使用 Release /MT 模式），或者使用命令行编译：



```bash

cl /EHsc /std:c++17 /MT /DUNICODE /D\_UNICODE DebugTools.cpp /link /SUBSYSTEM:WINDOWS

```

