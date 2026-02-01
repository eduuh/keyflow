#pragma once

// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on

#include <string>
#include <string_view>

namespace keyflow {

/**
 * @brief System tray icon manager
 *
 * Creates a system tray icon with a right-click menu for exit.
 * Used in both Debug and Release builds for better user experience.
 */
class SystemTray {
  public:
    SystemTray() = default;

    ~SystemTray() noexcept { cleanup(); }

    // Disable copy (HWND is not copyable)
    SystemTray(const SystemTray&) = delete;
    SystemTray& operator=(const SystemTray&) = delete;

    // Enable move
    SystemTray(SystemTray&& other) noexcept
        : hwnd_(other.hwnd_), iconAdded_(other.iconAdded_), appName_(std::move(other.appName_)) {
        other.hwnd_ = nullptr;
        other.iconAdded_ = false;
    }

    SystemTray& operator=(SystemTray&& other) noexcept {
        if (this != &other) {
            cleanup();
            hwnd_ = other.hwnd_;
            iconAdded_ = other.iconAdded_;
            appName_ = std::move(other.appName_);
            other.hwnd_ = nullptr;
            other.iconAdded_ = false;
        }
        return *this;
    }

    /**
     * @brief Initialize system tray icon
     * @param appName Application name for tooltip
     * @return true if successful
     */
    [[nodiscard]] bool initialize(std::string appName) {
        appName_ = std::move(appName);
        // Create hidden window for message handling
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

        // Add system tray icon
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd_;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 1;
        // Load custom keyboard icon
        nid.hIcon = (HICON)LoadImage(nullptr, "keyboard-icon.ico", IMAGE_ICON, 0, 0,
                                     LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
        if (!nid.hIcon) {
            nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // Fallback to default
        }
        strncpy_s(nid.szTip, sizeof(nid.szTip), appName_.c_str(), _TRUNCATE);

        if (Shell_NotifyIcon(NIM_ADD, &nid)) {
            iconAdded_ = true;

            // Show startup notification
            showNotification("Keyflow is Running",
                             "Keyboard remapper is active. Right-click tray icon to exit.");

            return true;
        }

        return false;
    }

    /**
     * @brief Show a notification balloon
     */
    void showNotification(std::string_view title, std::string_view message) noexcept {
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
        nid.uTimeout = 3000; // 3 seconds

        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }

    /**
     * @brief Process Windows messages (call from main loop)
     * @return false if exit requested
     */
    [[nodiscard]] bool processMessages() noexcept {
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
        SystemTray* self = nullptr;

        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<SystemTray*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<SystemTray*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (msg == WM_USER + 1) { // Tray icon message
            if (lParam == WM_RBUTTONUP) {
                // Show context menu
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
                    // Show status
                    if (self) {
                        MessageBox(hwnd,
                                   "Keyflow keyboard remapper is running.\n\n"
                                   "Right-click the tray icon to exit.",
                                   self->appName_.c_str(), MB_OK | MB_ICONINFORMATION);
                    }
                } else if (cmd == 2) {
                    // Exit
                    PostQuitMessage(0);
                }
            }
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
};

} // namespace keyflow
