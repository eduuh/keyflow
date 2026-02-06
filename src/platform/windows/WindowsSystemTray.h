#pragma once

// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on

#include "platform/IPlatform.h"

#include <string>
#include <string_view>

namespace keyflow {

class WindowsSystemTray final : public ISystemTray {
  public:
    WindowsSystemTray() = default;
    ~WindowsSystemTray() override { cleanup(); }

    WindowsSystemTray(const WindowsSystemTray&) = delete;
    WindowsSystemTray& operator=(const WindowsSystemTray&) = delete;
    WindowsSystemTray(WindowsSystemTray&&) = delete;
    WindowsSystemTray& operator=(WindowsSystemTray&&) = delete;

    [[nodiscard]] bool initialize(std::string appName) override {
        appName_ = std::move(appName);

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = windowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "KeyflowTrayWindow";

        if (!RegisterClassEx(&wc)) {
            return false;
        }

        hwnd_ = CreateWindowEx(0, "KeyflowTrayWindow", "Keyflow", 0, 0, 0, 0, 0, HWND_MESSAGE,
                               nullptr, wc.hInstance, this);

        if (!hwnd_) {
            return false;
        }

        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd_;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 1;
        nid.hIcon = (HICON)LoadImage(nullptr, "keyboard-icon.ico", IMAGE_ICON, 0, 0,
                                     LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
        if (!nid.hIcon) {
            nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        }
        strncpy_s(nid.szTip, sizeof(nid.szTip), appName_.c_str(), _TRUNCATE);

        if (Shell_NotifyIcon(NIM_ADD, &nid)) {
            iconAdded_ = true;
            showNotification("Keyflow is Running",
                             "Keyboard remapper is active. Right-click tray icon to exit.");
            return true;
        }

        return false;
    }

    void showNotification(std::string_view title, std::string_view message) noexcept override {
        if (!iconAdded_)
            return;

        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd_;
        nid.uID = 1;
        nid.uFlags = NIF_INFO;
        strncpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle), title.data(), _TRUNCATE);
        strncpy_s(nid.szInfo, sizeof(nid.szInfo), message.data(), _TRUNCATE);
        nid.dwInfoFlags = NIIF_INFO;
        nid.uTimeout = 3000;

        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }

    [[nodiscard]] bool processMessages() noexcept override {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return true;
    }

  private:
    HWND hwnd_{nullptr};
    bool iconAdded_{false};
    std::string appName_;

    void cleanup() noexcept {
        if (iconAdded_) {
            NOTIFYICONDATA nid = {};
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd_;
            nid.uID = 1;
            Shell_NotifyIcon(NIM_DELETE, &nid);
            iconAdded_ = false;
        }

        if (hwnd_) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        WindowsSystemTray* self = nullptr;

        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<WindowsSystemTray*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<WindowsSystemTray*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (msg == WM_USER + 1) {
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);

                HMENU menu = CreatePopupMenu();
                AppendMenu(menu, MF_STRING, 1, "Status");
                AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenu(menu, MF_STRING, 2, "Exit Keyflow");

                SetForegroundWindow(hwnd);
                int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd,
                                         nullptr);
                DestroyMenu(menu);

                if (cmd == 1) {
                    if (self) {
                        MessageBox(hwnd,
                                   "Keyflow keyboard remapper is running.\n\n"
                                   "Right-click the tray icon to exit.",
                                   self->appName_.c_str(), MB_OK | MB_ICONINFORMATION);
                    }
                } else if (cmd == 2) {
                    PostQuitMessage(0);
                }
            }
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
};

} // namespace keyflow
