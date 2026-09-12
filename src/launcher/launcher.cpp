// Minimal Qt-free launcher for a deployment with DLLs in lib/.
//
// The distribution root contains Gambasse.exe and config.ini; lib/ contains
// the actual application (gambasse.exe), its DLLs, and plugins. Windows
// loads implicitly linked DLLs beside the executable, so the Qt application must
// run from lib/. This launcher has no Qt dependency and passes the distribution
// root through --base so the application can locate configuration, data, and
// photos.

#include <windows.h>
#include <shellapi.h>
#include <string>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    wchar_t path[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        MessageBoxW(nullptr, L"Could not determine the launcher path.",
                    L"Gambasse", MB_ICONERROR);
        return 1;
    }

    std::wstring base(path, n);
    const size_t sep = base.find_last_of(L"\\/");
    base.resize(sep == std::wstring::npos ? 0 : sep);   // launcher directory (root)

    const std::wstring app = base + L"\\lib\\gambasse.exe";
    const std::wstring params = L"--base \"" + base + L"\"";

    // ShellExecuteW starts an independent process, as a double click would. Its
    // working directory is the distribution root.
    const HINSTANCE r = ShellExecuteW(nullptr, L"open", app.c_str(), params.c_str(),
                                      base.c_str(), SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(r) <= 32) {
        const std::wstring msg = L"Could not start the application:\n" + app;
        MessageBoxW(nullptr, msg.c_str(), L"Gambasse", MB_ICONERROR);
        return 1;
    }
    return 0;
}
