#pragma once

#ifdef __linux__

#    include "LinuxKeycodes.h"
#    include "platform/IPlatform.h"

#    include <array>
#    include <atomic>
#    include <sys/types.h>

struct libevdev_uinput;

namespace keyflow {

namespace linux_backend {

inline constexpr char outputName[] = "Keyflow virtual keyboard";
inline constexpr char outputPhys[] = "keyflow/uinput";
inline constexpr int outputVendor = 0x4b46;
inline constexpr int outputProduct = 0x0001;

[[nodiscard]] bool isOwnedDevice(const libevdev* device) noexcept;
[[nodiscard]] bool isKeyboardDevice(const libevdev* device) noexcept;

// The same checked writer/ledger is used by uinput and nonprivileged adapter tests.
class OutputState {
  public:
    using Writer = int (*)(void*, unsigned int, unsigned int, int);
    OutputState() noexcept;
    void includeKeyboard(const libevdev* device) noexcept;
    [[nodiscard]] bool supports(uint16_t canonical) const noexcept;
    [[nodiscard]] bool supportsEvdev(uint16_t code) const noexcept;
    [[nodiscard]] int send(const KeyEvent& event, Writer writer, void* context) noexcept;
    [[nodiscard]] int releaseAll(Writer writer, void* context) noexcept;

  private:
    std::array<bool, KEY_MAX + 1> supported_{};
    std::array<bool, KEY_MAX + 1> held_{};
};

} // namespace linux_backend

// All device reads/writes and shutdown belong to the main thread. Only requestStop
// may run on another thread or in a signal handler (while this object is alive).
class LinuxHardwareIO final : public IHardwareIO {
  public:
    LinuxHardwareIO() noexcept;
    ~LinuxHardwareIO() override;
    LinuxHardwareIO(const LinuxHardwareIO&) = delete;
    LinuxHardwareIO& operator=(const LinuxHardwareIO&) = delete;
    LinuxHardwareIO(LinuxHardwareIO&&) = delete;
    LinuxHardwareIO& operator=(LinuxHardwareIO&&) = delete;

    [[nodiscard]] bool configureDevices(const std::vector<std::string>& devices) override;
    [[nodiscard]] bool initialize() noexcept override;
    // Checks the kernel devnode and all-up state, not a compositor handshake.
    // Desktop consumers must be validated separately before physical capture.
    [[nodiscard]] bool activate() noexcept override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<KeyEvent> waitForKey(int timeoutMS = 0) noexcept override;
    void sendKey(uint16_t scancode, bool isDown) noexcept override;
    [[nodiscard]] bool sendEvent(const KeyEvent& event) noexcept override;
    [[nodiscard]] bool supportsKey(uint16_t scancode) const noexcept override;
    [[nodiscard]] bool isInitialized() const noexcept override { return initialized_; }
    void requestStop() noexcept override;
    [[nodiscard]] bool hasFailed() const noexcept override { return failed_; }
    [[nodiscard]] std::string_view lastError() const noexcept override { return error_.data(); }
    [[nodiscard]] std::string_view outputDevicePath() const noexcept;

  private:
    struct Device {
        int fd = -1;
        libevdev* evdev = nullptr;
        dev_t identity = 0;
        uint16_t source = 0;
        bool grabbed = false;
    };
    [[nodiscard]] bool openDevices();
    [[nodiscard]] bool createOutput() noexcept;
    [[nodiscard]] bool verifyOutput() noexcept;
    [[nodiscard]] bool checkAllUp(Device& device, const char* path) noexcept;
    [[nodiscard]] bool fail(const char* operation, const char* path, int error = 0) noexcept;
    [[nodiscard]] std::optional<KeyEvent> readDevice(Device& device, bool& buffered) noexcept;
    static int writeOutput(void* context, unsigned int type, unsigned int code, int value);

    std::vector<std::string> paths_;
    std::vector<Device> devices_;
    linux_backend::OutputState outputState_;
    libevdev_uinput* output_ = nullptr;
    int wakeFd_ = -1;
    int wakeError_ = 0;
    std::atomic<bool> stop_{false};
    bool initialized_ = false;
    bool active_ = false;
    bool failed_ = false;
    std::array<char, 1024> error_{};
    size_t nextDevice_ = 0;
};

} // namespace keyflow

#endif
