#include "LinuxHardwareIO.h"

#ifdef __linux__

#    include <algorithm>
#    include <cerrno>
#    include <chrono>
#    include <cstdio>
#    include <cstring>
#    include <fcntl.h>
#    include <libevdev/libevdev-uinput.h>
#    include <limits>
#    include <poll.h>
#    include <sys/eventfd.h>
#    include <sys/ioctl.h>
#    include <sys/stat.h>
#    include <unistd.h>

namespace keyflow {

namespace linux_backend {

bool isOwnedDevice(const libevdev* device) noexcept {
    const char* name = libevdev_get_name(device);
    const char* phys = libevdev_get_phys(device);
    return (name && std::string_view(name).substr(0, 7) == "Keyflow") ||
           (phys && std::string_view(phys).substr(0, 8) == "keyflow/") ||
           (libevdev_get_id_bustype(device) == BUS_VIRTUAL &&
            libevdev_get_id_vendor(device) == outputVendor &&
            libevdev_get_id_product(device) == outputProduct);
}

bool isKeyboardDevice(const libevdev* device) noexcept {
    // Accept keyboard attachments, not mouse/gamepad-only or media-button-only nodes.
    return libevdev_has_event_code(device, EV_KEY, KEY_A) &&
           libevdev_has_event_code(device, EV_KEY, KEY_Z) &&
           libevdev_has_event_code(device, EV_KEY, KEY_ENTER) &&
           libevdev_has_event_code(device, EV_KEY, KEY_SPACE);
}

OutputState::OutputState() noexcept {
    for (const auto& mapping : keycodeMappings)
        supported_[mapping.evdev] = true;
}

void OutputState::includeKeyboard(const libevdev* device) noexcept {
    if (!isKeyboardDevice(device) || isOwnedDevice(device))
        return;
    for (uint16_t code = 1; code <= KEY_MAX; ++code) {
        if (isKeyboardCode(code) && libevdev_has_event_code(device, EV_KEY, code))
            supported_[code] = true;
    }
}

bool OutputState::supports(uint16_t canonical) const noexcept {
    const auto code = toEvdev(canonical);
    return code && supported_[*code];
}

bool OutputState::supportsEvdev(uint16_t code) const noexcept {
    return code <= KEY_MAX && supported_[code];
}

int OutputState::send(const KeyEvent& event, Writer writer, void* context) noexcept {
    if (!event.native && event.scancode == SC_NOP)
        return 0;
    const auto code =
        event.native ? std::optional<uint16_t>(event.scancode) : toEvdev(event.scancode);
    if (!code || !supportsEvdev(*code) ||
        (event.native && (!isKeyboardCode(*code) || toCanonical(*code))))
        return -EINVAL;
    int value = 0;
    switch (event.action) {
        case KeyAction::Press:
            value = 1;
            break;
        case KeyAction::Repeat:
            if (!held_[*code])
                return -EINVAL;
            value = 2;
            break;
        case KeyAction::Release:
            break;
        default:
            return -EINVAL;
    }
    // A write can partially succeed; retain possible ownership until a complete up frame.
    if (value == 1)
        held_[*code] = true;
    int result = writer(context, EV_KEY, *code, value);
    if (result < 0)
        return result;
    result = writer(context, EV_SYN, SYN_REPORT, 0);
    if (result == 0 && value == 0)
        held_[*code] = false;
    return result;
}

int OutputState::releaseAll(Writer writer, void* context) noexcept {
    int firstError = 0;
    for (uint16_t code = 1; code <= KEY_MAX; ++code) {
        if (!held_[code])
            continue;
        int result = writer(context, EV_KEY, code, 0);
        if (result == 0)
            result = writer(context, EV_SYN, SYN_REPORT, 0);
        if (result == 0)
            held_[code] = false;
        else if (firstError == 0)
            firstError = result;
    }
    return firstError;
}

} // namespace linux_backend

LinuxHardwareIO::LinuxHardwareIO() noexcept : wakeFd_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
    static_assert(std::atomic<bool>::is_always_lock_free);
    if (wakeFd_ < 0)
        wakeError_ = errno;
}

LinuxHardwareIO::~LinuxHardwareIO() {
    shutdown();
    if (wakeFd_ >= 0)
        close(wakeFd_);
}

bool LinuxHardwareIO::configureDevices(const std::vector<std::string>& devices) {
    if (initialized_ || !devices_.empty())
        return fail("configure devices before initialize", "", EBUSY);
    if (devices.empty())
        return fail("select at least one keyboard with --device", "", EINVAL);
    if (devices.size() > std::numeric_limits<uint16_t>::max())
        return fail("too many selected keyboards", "", EINVAL);
    for (const auto& path : devices) {
        if (path.empty() || path.front() != '/' || path.find('\0') != std::string::npos)
            return fail("--device must be an absolute input-device path", path.c_str(), EINVAL);
    }
    paths_ = devices;
    return true;
}

bool LinuxHardwareIO::fail(const char* operation, const char* path, int error) noexcept {
    if (!failed_) {
        std::snprintf(error_.data(), error_.size(), "%s%s%s%s%s", operation, path[0] ? ": " : "",
                      path, error ? ": " : "", error ? std::strerror(error) : "");
    }
    failed_ = true;
    shutdown();
    return false;
}

bool LinuxHardwareIO::openDevices() {
    devices_.reserve(paths_.size());
    for (size_t index = 0; index < paths_.size(); ++index) {
        if (stop_.load(std::memory_order_relaxed)) {
            shutdown();
            return false;
        }
        devices_.push_back(Device{});
        auto& device = devices_.back();
        const char* path = paths_[index].c_str();
        device.fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (device.fd < 0)
            return fail("cannot open selected keyboard; check explicit device access", path, errno);
        struct stat info{};
        if (fstat(device.fd, &info) < 0)
            return fail("cannot identify selected keyboard", path, errno);
        if (!S_ISCHR(info.st_mode))
            return fail("selected path is not an evdev character device", path, ENODEV);
        device.identity = info.st_rdev;
        for (size_t previous = 0; previous < index; ++previous) {
            if (devices_[previous].identity == device.identity)
                return fail("same keyboard selected more than once", path, EINVAL);
        }
        const int result = libevdev_new_from_fd(device.fd, &device.evdev);
        if (result < 0)
            return fail("cannot decode selected evdev keyboard", path, -result);
        if (linux_backend::isOwnedDevice(device.evdev))
            return fail("refusing Keyflow-owned virtual input", path, EINVAL);
        if (!linux_backend::isKeyboardDevice(device.evdev))
            return fail("selected device lacks keyboard capabilities", path, EINVAL);
        device.source = static_cast<uint16_t>(index + 1);
        outputState_.includeKeyboard(device.evdev);
    }
    return true;
}

bool LinuxHardwareIO::createOutput() noexcept {
    libevdev* description = libevdev_new();
    if (!description)
        return fail("cannot allocate virtual keyboard description", "", ENOMEM);
    libevdev_set_name(description, linux_backend::outputName);
    libevdev_set_phys(description, linux_backend::outputPhys);
    libevdev_set_id_bustype(description, BUS_VIRTUAL);
    libevdev_set_id_vendor(description, linux_backend::outputVendor);
    libevdev_set_id_product(description, linux_backend::outputProduct);
    int result = 0;
    for (uint16_t code = 1; code <= KEY_MAX; ++code) {
        if (!outputState_.supportsEvdev(code))
            continue;
        result = libevdev_enable_event_code(description, EV_KEY, code, nullptr);
        if (result < 0)
            break;
    }
    // Do not enable EV_REP: physical value-2 events are the sole repeat generator.
    if (result == 0)
        result =
            libevdev_uinput_create_from_device(description, LIBEVDEV_UINPUT_OPEN_MANAGED, &output_);
    libevdev_free(description);
    if (result < 0)
        return fail("cannot create uinput keyboard; check explicit /dev/uinput access", "",
                    -result);
    return verifyOutput();
}

bool LinuxHardwareIO::verifyOutput() noexcept {
    const char* path = libevdev_uinput_get_devnode(output_);
    if (!path)
        return fail("uinput has no identifiable evdev devnode", "", ENODEV);
    const int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        return fail("cannot verify uinput devnode; check input-node access", path, errno);
    libevdev* device = nullptr;
    int result = libevdev_new_from_fd(fd, &device);
    bool matches = false;
    if (result == 0) {
        const char* name = libevdev_get_name(device);
        const char* phys = libevdev_get_phys(device);
        matches = name && std::string_view(name) == linux_backend::outputName && phys &&
                  std::string_view(phys) == linux_backend::outputPhys &&
                  libevdev_get_id_bustype(device) == BUS_VIRTUAL &&
                  libevdev_get_id_vendor(device) == linux_backend::outputVendor &&
                  libevdev_get_id_product(device) == linux_backend::outputProduct &&
                  !libevdev_has_event_type(device, EV_REP);
        for (uint16_t code = 1; matches && code <= KEY_MAX; ++code) {
            if (outputState_.supportsEvdev(code) && !libevdev_has_event_code(device, EV_KEY, code))
                matches = false;
        }
    }
    if (device)
        libevdev_free(device);
    close(fd);
    if (!matches)
        return fail("uinput devnode identity/capabilities not ready", path,
                    result < 0 ? -result : ENODEV);
    return true;
}

bool LinuxHardwareIO::initialize() noexcept {
    if (initialized_)
        return !failed_;
    if (failed_)
        return false;
    if (stop_.load(std::memory_order_relaxed))
        return false;
    if (paths_.empty())
        return fail("select at least one keyboard with --device", "", EINVAL);
    if (wakeFd_ < 0)
        return fail("cannot create cancellation eventfd", "", wakeError_);
    outputState_ = linux_backend::OutputState{};
    try {
        if (!openDevices())
            return false;
        if (stop_.load(std::memory_order_relaxed)) {
            shutdown();
            return false;
        }
        if (!createOutput())
            return false;
    } catch (...) {
        return fail("cannot allocate selected keyboard resources", "", ENOMEM);
    }
    initialized_ = true;
    return true;
}

bool LinuxHardwareIO::checkAllUp(Device& device, const char* path) noexcept {
    // Drain queued startup events, then query the kernel's current state rather
    // than treating libevdev's cached state as proof that all physical keys are up.
    input_event event{};
    int result = 0;
    for (int count = 0; count < 1024; ++count) {
        if (stop_.load(std::memory_order_relaxed)) {
            shutdown();
            return false;
        }
        result = libevdev_next_event(device.evdev, LIBEVDEV_READ_FLAG_NORMAL, &event);
        if (result != LIBEVDEV_READ_STATUS_SUCCESS)
            break;
    }
    if (result != -EAGAIN)
        return fail("cannot establish all-up state; restart session", path,
                    result < 0 ? -result : EOVERFLOW);
    std::array<unsigned char, (KEY_MAX + 8) / 8> keys{};
    if (ioctl(device.fd, EVIOCGKEY(keys.size()), keys.data()) < 0)
        return fail("cannot query selected keyboard held state", path, errno);
    if (std::any_of(keys.begin(), keys.end(), [](unsigned char byte) { return byte != 0; }))
        return fail("release every selected keyboard key and restart before capture", path, EBUSY);
    return true;
}

bool LinuxHardwareIO::activate() noexcept {
    if (!initialized_ || failed_ || stop_.load(std::memory_order_relaxed))
        return false;
    if (active_)
        return true;
    if (!verifyOutput())
        return false;
    for (size_t index = 0; index < devices_.size(); ++index) {
        auto& device = devices_[index];
        const char* path = paths_[index].c_str();
        struct stat info{};
        if (stat(path, &info) < 0)
            return fail("selected keyboard path disappeared before activation", path, errno);
        if (!S_ISCHR(info.st_mode) || info.st_rdev != device.identity)
            return fail("selected keyboard path changed before activation", path, ENODEV);
        if (!checkAllUp(device, path))
            return false;
    }
    for (size_t index = 0; index < devices_.size(); ++index) {
        auto& device = devices_[index];
        const char* path = paths_[index].c_str();
        const int result = libevdev_grab(device.evdev, LIBEVDEV_GRAB);
        if (result < 0)
            return fail("cannot exclusively grab selected keyboard; another capturer may own it",
                        path, -result);
        device.grabbed = true;
        // Recheck after grabbing: a key may have gone down during startup.
        if (!checkAllUp(device, path))
            return false;
    }
    active_ = true;
    return true;
}

int LinuxHardwareIO::writeOutput(void* context, unsigned int type, unsigned int code, int value) {
    return libevdev_uinput_write_event(static_cast<libevdev_uinput*>(context), type, code, value);
}

void LinuxHardwareIO::shutdown() noexcept {
    active_ = false;
    initialized_ = false;
    if (output_) {
        const int result = outputState_.releaseAll(writeOutput, output_);
        if (result < 0 && !failed_) {
            std::snprintf(error_.data(), error_.size(),
                          "cannot release uinput keys during cleanup: %s", std::strerror(-result));
            failed_ = true;
        }
        libevdev_uinput_destroy(output_);
        output_ = nullptr;
    }
    for (auto& device : devices_) {
        if (device.grabbed)
            libevdev_grab(device.evdev, LIBEVDEV_UNGRAB);
        if (device.evdev)
            libevdev_free(device.evdev);
        if (device.fd >= 0)
            close(device.fd);
    }
    devices_.clear();
    nextDevice_ = 0;
}

void LinuxHardwareIO::requestStop() noexcept {
    const int savedErrno = errno;
    stop_.store(true, std::memory_order_relaxed);
    if (wakeFd_ >= 0) {
        const uint64_t value = 1;
        // A full eventfd is already readable. Bounded polling also observes stop_.
        const auto ignored = write(wakeFd_, &value, sizeof(value));
        (void)ignored;
    }
    errno = savedErrno;
}

std::optional<KeyEvent> LinuxHardwareIO::readDevice(Device& device, bool& buffered) noexcept {
    for (int count = 0; count < 64; ++count) {
        if (stop_.load(std::memory_order_relaxed))
            return std::nullopt;
        input_event event{};
        const int result = libevdev_next_event(device.evdev, LIBEVDEV_READ_FLAG_NORMAL, &event);
        if (result == -EAGAIN || result == -EINTR)
            return std::nullopt;
        if (result == LIBEVDEV_READ_STATUS_SYNC ||
            (result == LIBEVDEV_READ_STATUS_SUCCESS && event.type == EV_SYN &&
             event.code == SYN_DROPPED)) {
            (void)fail("selected keyboard overflow (SYN_DROPPED); restart session",
                       paths_[device.source - 1].c_str(), EOVERFLOW);
            return std::nullopt;
        }
        if (result < 0) {
            (void)fail("selected keyboard lost or read failed; restart session",
                       paths_[device.source - 1].c_str(), -result);
            return std::nullopt;
        }
        const auto key = linux_backend::decodeKey(device.evdev, event, device.source);
        if (key)
            return key;
    }
    buffered = true;
    return std::nullopt;
}

std::optional<KeyEvent> LinuxHardwareIO::waitForKey(int timeoutMS) noexcept {
    if (!active_ || failed_ || stop_.load(std::memory_order_relaxed))
        return std::nullopt;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(timeoutMS, 0));
    try {
        std::vector<pollfd> descriptors(devices_.size() + 1);
        descriptors[0] = {wakeFd_, POLLIN, 0};
        for (size_t index = 0; index < devices_.size(); ++index)
            descriptors[index + 1] = {devices_[index].fd, POLLIN, 0};
        while (!stop_.load(std::memory_order_relaxed)) {
            // libevdev may already have buffered events; poll alone cannot see those.
            bool buffered = false;
            for (size_t offset = 0; offset < devices_.size(); ++offset) {
                const size_t index = (nextDevice_ + offset) % devices_.size();
                const auto event = readDevice(devices_[index], buffered);
                if (failed_ || stop_.load(std::memory_order_relaxed))
                    return std::nullopt;
                if (event) {
                    nextDevice_ = (index + 1) % devices_.size();
                    return event;
                }
            }
            int wait = buffered ? 0 : 50;
            if (timeoutMS > 0) {
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());
                if (remaining.count() <= 0)
                    return std::nullopt;
                wait = static_cast<int>(std::min<int64_t>(wait, remaining.count()));
            }
            const int result = poll(descriptors.data(), descriptors.size(), wait);
            if (result < 0 && errno != EINTR) {
                (void)fail("selected keyboard poll failed; restart session", "", errno);
                return std::nullopt;
            }
            for (size_t index = 1; index < descriptors.size(); ++index) {
                if (descriptors[index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    (void)fail("selected keyboard disconnected; restart session",
                               paths_[index - 1].c_str(), ENODEV);
                    return std::nullopt;
                }
            }
        }
    } catch (...) {
        (void)fail("cannot allocate keyboard poll resources", "", ENOMEM);
    }
    return std::nullopt;
}

bool LinuxHardwareIO::supportsKey(uint16_t scancode) const noexcept {
    return outputState_.supports(scancode);
}

std::string_view LinuxHardwareIO::outputDevicePath() const noexcept {
    const char* path = output_ ? libevdev_uinput_get_devnode(output_) : nullptr;
    return path ? std::string_view(path) : std::string_view{};
}

bool LinuxHardwareIO::sendEvent(const KeyEvent& event) noexcept {
    if (!event.native && event.scancode == SC_NOP)
        return true;
    if (!active_ || failed_ || stop_.load(std::memory_order_relaxed))
        return false;
    const int result = outputState_.send(event, writeOutput, output_);
    if (result < 0)
        return fail("uinput key/frame write failed; session stopped", "", -result);
    return true;
}

void LinuxHardwareIO::sendKey(uint16_t scancode, bool isDown) noexcept {
    (void)sendEvent(KeyEvent(scancode, isDown));
}

} // namespace keyflow

#endif
