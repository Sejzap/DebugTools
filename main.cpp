// ToolBox.cpp —— Windows 系统功能调试工具箱
// 编译（MSVC 开发者命令提示符）：
//   cl /EHsc /std:c++17 /DUNICODE /D_UNICODE ToolBox.cpp
#include <windows.h>
#include <string>

// 【1】强制链接器以窗口模式运行，并指定宽字符入口点（彻底去除控制台黑框）
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// 假设你在资源文件(.rc)中导入的图标 ID 为 101 或 IDI_ICON1
// 如果你在 Visual Studio 中自动生成了 resource.h，可以删掉下面这行，改为 #include "resource.h"
#ifndef IDI_ICON1
#define IDI_ICON1 101
#endif

// ---------- 控件 ID ----------
#define ID_BTN_CMD      1001
#define ID_BTN_REGEDIT  1002
#define ID_BTN_NOTEPAD  1003
#define ID_BTN_GPEDIT   1004

// ---------- 布局基准尺寸（96 DPI 下的逻辑像素） ----------
#define BTN_W   220   // 纯中文按钮宽度
#define BTN_H   44
#define BTN_GAP 14
#define MARGIN  24
#define BTN_CNT 4

static HFONT g_hFont = nullptr;
static int   g_dpi = 96;

// 按当前 DPI 缩放
static inline int S(int v) { return MulDiv(v, g_dpi, 96); }

// ---------- 创建界面字体 ----------
static HFONT CreateUiFont()
{
    NONCLIENTMETRICSW ncm = { sizeof(ncm) };
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        LOGFONTW lf = ncm.lfMessageFont;
        if (lf.lfHeight < 0)
            lf.lfHeight = MulDiv(lf.lfHeight, 11, 10); // 放大约 10%，中文更清晰
        lf.lfWeight = FW_NORMAL;
        return CreateFontIndirectW(&lf);
    }
    return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

// ---------- 启动外部程序 ----------
// 成功返回 0，失败返回 Win32 错误码
static DWORD LaunchProcess(const wchar_t* file)
{
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_FLAG_NO_UI;   // 错误由我们自己弹框，屏蔽系统默认提示
    sei.lpVerb = L"open";
    sei.lpFile = file;
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&sei))
        return 0;

    DWORD err = GetLastError();

    // 权限不足时，改用管理员权限重试（regedit / gpedit 通常需要）
    if (err == ERROR_ACCESS_DENIED || err == ERROR_ELEVATION_REQUIRED)
    {
        sei.lpVerb = L"runas";
        if (ShellExecuteExW(&sei))
            return 0;
        err = GetLastError();
    }
    return err;
}

// ---------- 统一的启动 + 错误提示 ----------
static void DoLaunch(HWND hWnd, const wchar_t* file, const wchar_t* name)
{
    DWORD err = LaunchProcess(file);
    if (err == 0)
        return;                         // 成功

    if (err == ERROR_CANCELLED)         // 用户在 UAC 弹窗点了“否”
    {
        MessageBoxW(hWnd, L"操作已取消（该功能需要管理员权限）。",
            L"提示", MB_ICONINFORMATION);
        return;
    }

    std::wstring msg = L"无法启动：";
    msg += name;
    msg += L"\n\n命令：";
    msg += file;
    msg += L"\n错误码：";
    msg += std::to_wstring(err);
    msg += L"\n\n常见原因：\n"
        L"  · 当前系统不含该组件（例如 Windows 家庭版没有 gpedit.msc）\n"
        L"  · 权限不足，请以管理员身份运行本工具\n"
        L"  · 被安全软件或组策略拦截\n"
        L"  · 系统文件缺失（可用 sfc /scannow 修复）";

    MessageBoxW(hWnd, msg.c_str(), L"启动失败", MB_ICONERROR | MB_OK);
}

// ---------- 遍历子控件设置字体 ----------
static BOOL CALLBACK SetFontProc(HWND hChild, LPARAM lParam)
{
    SendMessageW(hChild, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

// ---------- 窗口过程 ----------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // 按钮文字（已去掉末尾后缀）
        struct { int id; const wchar_t* text; } btns[BTN_CNT] = {
            { ID_BTN_CMD,     L"打开命令提示符"   },
            { ID_BTN_REGEDIT, L"打开注册表编辑器" },
            { ID_BTN_NOTEPAD, L"打开记事本"       },
            { ID_BTN_GPEDIT,  L"打开组策略编辑器" },
        };

        for (int i = 0; i < BTN_CNT; ++i)
        {
            CreateWindowExW(0, L"BUTTON", btns[i].text,
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                S(MARGIN),
                S(MARGIN) + i * S(BTN_H + BTN_GAP),
                S(BTN_W), S(BTN_H),
                hWnd, (HMENU)(INT_PTR)btns[i].id,
                GetModuleHandleW(nullptr), nullptr);
        }

        g_hFont = CreateUiFont();
        EnumChildWindows(hWnd, SetFontProc, (LPARAM)g_hFont);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BTN_CMD:     DoLaunch(hWnd, L"cmd.exe", L"命令提示符");   break;
        case ID_BTN_REGEDIT: DoLaunch(hWnd, L"regedit.exe", L"注册表编辑器"); break;
        case ID_BTN_NOTEPAD: DoLaunch(hWnd, L"notepad.exe", L"记事本");       break;
        case ID_BTN_GPEDIT:  DoLaunch(hWnd, L"gpedit.msc", L"组策略编辑器"); break;
        }
        return 0;

    case WM_DESTROY:
        if (g_hFont) { DeleteObject(g_hFont); g_hFont = nullptr; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ---------- 入口 ----------
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    // 高 DPI 适配
    SetProcessDPIAware();
    {
        HDC hdc = GetDC(nullptr);
        g_dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(nullptr, hdc);
        if (g_dpi <= 0) g_dpi = 96;
    }

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DebugToolsWndClass";

    // 【2】加载自定义图标（带后备机制，如果没有导入图标，则使用系统默认图标）
    wc.hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ICON1),
        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION); // 后备

    wc.hIconSm = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ICON1),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    if (!wc.hIconSm) wc.hIconSm = (HICON)LoadIconW(nullptr, IDI_APPLICATION); // 后备

    if (!RegisterClassExW(&wc))
        return 1;

    // 固定大小窗口（客户区尺寸 = 边距 + 按钮区）
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0,
                S(MARGIN) * 2 + S(BTN_W),
                S(MARGIN) * 2 + BTN_CNT * S(BTN_H) + (BTN_CNT - 1) * S(BTN_GAP) };
    AdjustWindowRectEx(&rc, style, FALSE, 0);

    HWND hWnd = CreateWindowExW(0, wc.lpszClassName,
        L"DebugTools - 系统调试工具箱",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);

    if (!hWnd)
        return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        // 让 Tab 键可以切换按钮焦点
        if (!IsDialogMessageW(hWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}