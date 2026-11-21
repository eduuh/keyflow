#ifdef __linux__

#    include "platform/linux/LinuxHardwareIO.h"

#    include <cerrno>
#    include <chrono>
#    include <fcntl.h>
#    include <gtest/gtest.h>
#    include <memory>
#    include <sys/eventfd.h>
#    include <sys/stat.h>
#    include <thread>
#    include <tuple>
#    include <unistd.h>
#    include <vector>

namespace keyflow::linux_backend {
namespace {

using Device = std::unique_ptr<libevdev, decltype(&libevdev_free)>;

TEST(LinuxOutputReadiness, WaitsForDelayedAccessWithoutFailingInitialization) {
    if (geteuid() == 0)
        GTEST_SKIP() << "Root bypasses the access restrictions exercised by this test";
    char path[] = "/tmp/keyflow-output-access-XXXXXX";
    const int original = mkstemp(path);
    ASSERT_GE(original, 0);
    ASSERT_EQ(fchmod(original, 0000), 0);
    std::atomic<bool> stop{false};
    std::thread permissions([original] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        EXPECT_EQ(fchmod(original, 0600), 0);
    });
    const int fd = openOutputNode(path, -1, stop);
    permissions.join();
    EXPECT_GE(fd, 0);
    if (fd >= 0)
        close(fd);
    close(original);
    unlink(path);
}

TEST(LinuxOutputReadiness, MissingNodeWaitIsBoundedAndCancellationAware) {
    char directory[] = "/tmp/keyflow-output-missing-XXXXXX";
    ASSERT_NE(mkdtemp(directory), nullptr);
    const std::string path = std::string(directory) + "/event";
    std::atomic<bool> stop{false};
    const int wake = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    ASSERT_GE(wake, 0);
    std::thread cancellation([&stop, wake] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        stop.store(true, std::memory_order_relaxed);
        const uint64_t value = 1;
        EXPECT_EQ(write(wake, &value, sizeof(value)), sizeof(value));
    });
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(openOutputNode(path.c_str(), wake, stop), -1);
    EXPECT_EQ(errno, ECANCELED);
    cancellation.join();
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(1));
    close(wake);
    stop.store(false, std::memory_order_relaxed);
    const auto retry = std::chrono::steady_clock::now();
    EXPECT_EQ(openOutputNode(path.c_str(), -1, stop), -1);
    EXPECT_EQ(errno, ENOENT);
    EXPECT_GE(std::chrono::steady_clock::now() - retry, std::chrono::seconds(1));
    EXPECT_LT(std::chrono::steady_clock::now() - retry, std::chrono::seconds(2));
    rmdir(directory);
}

Device keyboard() {
    Device device(libevdev_new(), libevdev_free);
    for (const unsigned int code : {KEY_A, KEY_Z, KEY_ENTER, KEY_SPACE, KEY_F13, KEY_LEFTCTRL}) {
        EXPECT_EQ(libevdev_enable_event_code(device.get(), EV_KEY, code, nullptr), 0);
    }
    return device;
}

struct RecordingWriter {
    std::vector<std::tuple<unsigned int, unsigned int, int>> events;
    int failAt = -1;
    static int write(void* context, unsigned int type, unsigned int code, int value) {
        auto& self = *static_cast<RecordingWriter*>(context);
        const int index = static_cast<int>(self.events.size());
        self.events.emplace_back(type, code, value);
        return index == self.failAt ? -EIO : 0;
    }
};

TEST(LinuxHardwareAdapter, DecodesEachSourcePressRepeatAndReleaseWithoutAggregation) {
    auto device = keyboard();
    const auto press = decodeKey(device.get(), input_event{{}, EV_KEY, KEY_LEFTCTRL, 1}, 1);
    const auto other = decodeKey(device.get(), input_event{{}, EV_KEY, KEY_LEFTCTRL, 1}, 2);
    const auto repeat = decodeKey(device.get(), input_event{{}, EV_KEY, KEY_LEFTCTRL, 2}, 1);
    const auto release = decodeKey(device.get(), input_event{{}, EV_KEY, KEY_LEFTCTRL, 0}, 1);
    ASSERT_TRUE(press);
    ASSERT_TRUE(other);
    ASSERT_TRUE(repeat);
    ASSERT_TRUE(release);
    EXPECT_EQ(press->scancode, 0x001d);
    EXPECT_FALSE(press->native);
    EXPECT_EQ(press->source, 1);
    EXPECT_EQ(other->source, 2);
    EXPECT_EQ(press->action, KeyAction::Press);
    EXPECT_EQ(repeat->action, KeyAction::Repeat);
    EXPECT_TRUE(repeat->isDown);
    EXPECT_EQ(release->action, KeyAction::Release);
    EXPECT_FALSE(release->isDown);
}

TEST(LinuxHardwareAdapter, NativePassthroughRequiresSelectedKeyboardCapabilityAndValidValue) {
    auto device = keyboard();
    const auto event = decodeKey(device.get(), input_event{{}, EV_KEY, KEY_F13, 1}, 7);
    ASSERT_TRUE(event);
    EXPECT_TRUE(event->native);
    EXPECT_EQ(event->scancode, KEY_F13);
    EXPECT_EQ(event->source, 7);
    EXPECT_FALSE(decodeKey(device.get(), input_event{{}, EV_KEY, KEY_F14, 1}, 7));
    EXPECT_FALSE(decodeKey(device.get(), input_event{{}, EV_KEY, BTN_LEFT, 1}, 7));
    EXPECT_FALSE(decodeKey(device.get(), input_event{{}, EV_KEY, KEY_F13, 3}, 7));
    EXPECT_FALSE(decodeKey(device.get(), input_event{{}, EV_KEY, KEY_F13, -1}, 7));
    EXPECT_FALSE(decodeKey(device.get(), input_event{{}, EV_KEY, KEY_F13, 1}, 0));
    EXPECT_FALSE(decodeKey(device.get(), input_event{{}, EV_MSC, MSC_SCAN, 1}, 7));
}

TEST(LinuxHardwareAdapter, ExcludesOwnedVirtualDevicesByNamePhysOrIdentity) {
    auto name = keyboard();
    libevdev_set_name(name.get(), "Keyflow virtual keyboard old attachment");
    EXPECT_TRUE(isOwnedDevice(name.get()));
    auto phys = keyboard();
    libevdev_set_phys(phys.get(), "keyflow/uinput/another-session");
    EXPECT_TRUE(isOwnedDevice(phys.get()));
    auto identity = keyboard();
    libevdev_set_name(identity.get(), "different name");
    libevdev_set_id_bustype(identity.get(), BUS_VIRTUAL);
    libevdev_set_id_vendor(identity.get(), outputVendor);
    libevdev_set_id_product(identity.get(), outputProduct);
    EXPECT_TRUE(isOwnedDevice(identity.get()));
    auto physical = keyboard();
    libevdev_set_name(physical.get(), "ordinary keyboard");
    EXPECT_FALSE(isOwnedDevice(physical.get()));
}

TEST(LinuxHardwareAdapter, MouseOnlyAndOwnedSourcesCannotAdvertiseNativeKeys) {
    OutputState state;
    Device mouse(libevdev_new(), libevdev_free);
    ASSERT_EQ(libevdev_enable_event_code(mouse.get(), EV_KEY, BTN_LEFT, nullptr), 0);
    ASSERT_EQ(libevdev_enable_event_code(mouse.get(), EV_KEY, KEY_F13, nullptr), 0);
    EXPECT_FALSE(isKeyboardDevice(mouse.get()));
    state.includeKeyboard(mouse.get());
    EXPECT_FALSE(state.supportsEvdev(KEY_F13));
    auto owned = keyboard();
    libevdev_set_phys(owned.get(), outputPhys);
    state.includeKeyboard(owned.get());
    EXPECT_FALSE(state.supportsEvdev(KEY_F13));
    auto physical = keyboard();
    state.includeKeyboard(physical.get());
    EXPECT_TRUE(state.supportsEvdev(KEY_F13));
    EXPECT_FALSE(state.supportsEvdev(BTN_LEFT));
    EXPECT_FALSE(state.supports(KEY_F13));
}

TEST(LinuxHardwareAdapter, OutputWritesExplicitPressRepeatReleaseAndCompleteFrames) {
    OutputState state;
    RecordingWriter writer;
    ASSERT_EQ(state.send(KeyEvent(SC_RCTRL, KeyAction::Press), writer.write, &writer), 0);
    ASSERT_EQ(state.send(KeyEvent(SC_RCTRL, KeyAction::Repeat), writer.write, &writer), 0);
    ASSERT_EQ(state.send(KeyEvent(SC_RCTRL, KeyAction::Release), writer.write, &writer), 0);
    const decltype(writer.events) expected = {{EV_KEY, KEY_RIGHTCTRL, 1}, {EV_SYN, SYN_REPORT, 0},
                                              {EV_KEY, KEY_RIGHTCTRL, 2}, {EV_SYN, SYN_REPORT, 0},
                                              {EV_KEY, KEY_RIGHTCTRL, 0}, {EV_SYN, SYN_REPORT, 0}};
    EXPECT_EQ(writer.events, expected);
    ASSERT_EQ(state.releaseAll(writer.write, &writer), 0);
    EXPECT_EQ(writer.events, expected);
}

TEST(LinuxHardwareAdapter, NativeUnknownKeyKeepsBalancedOutputAndSentinelsDoNotEmit) {
    auto device = keyboard();
    OutputState state;
    state.includeKeyboard(device.get());
    RecordingWriter writer;
    EXPECT_EQ(state.send(KeyEvent(SC_NOP, KeyAction::Press), writer.write, &writer), 0);
    EXPECT_EQ(state.send(KeyEvent(SC_UNMAPPED, KeyAction::Press), writer.write, &writer), -EINVAL);
    EXPECT_EQ(state.send(KeyEvent(KEY_F14, KeyAction::Press, 1, true), writer.write, &writer),
              -EINVAL);
    EXPECT_EQ(state.send(KeyEvent(BTN_LEFT, KeyAction::Press, 1, true), writer.write, &writer),
              -EINVAL);
    EXPECT_EQ(state.send(KeyEvent(KEY_A, KeyAction::Press, 1, true), writer.write, &writer),
              -EINVAL);
    EXPECT_TRUE(writer.events.empty());
    ASSERT_EQ(state.send(KeyEvent(KEY_F13, KeyAction::Press, 1, true), writer.write, &writer), 0);
    ASSERT_EQ(state.send(KeyEvent(KEY_F13, KeyAction::Repeat, 1, true), writer.write, &writer), 0);
    ASSERT_EQ(state.releaseAll(writer.write, &writer), 0);
    const decltype(writer.events) expected = {{EV_KEY, KEY_F13, 1}, {EV_SYN, SYN_REPORT, 0},
                                              {EV_KEY, KEY_F13, 2}, {EV_SYN, SYN_REPORT, 0},
                                              {EV_KEY, KEY_F13, 0}, {EV_SYN, SYN_REPORT, 0}};
    EXPECT_EQ(writer.events, expected);
}

TEST(LinuxHardwareAdapter, FailedPressFrameRetainsOwnershipForCleanup) {
    OutputState state;
    RecordingWriter writer;
    writer.failAt = 1;
    EXPECT_EQ(state.send(KeyEvent(SC_A, KeyAction::Press), writer.write, &writer), -EIO);
    ASSERT_EQ(state.releaseAll(writer.write, &writer), 0);
    const decltype(writer.events) expected = {
        {EV_KEY, KEY_A, 1}, {EV_SYN, SYN_REPORT, 0}, {EV_KEY, KEY_A, 0}, {EV_SYN, SYN_REPORT, 0}};
    EXPECT_EQ(writer.events, expected);
}

TEST(LinuxHardwareAdapter, FailedKeyWriteAndFailedReleaseFrameAreRetriedDuringCleanup) {
    OutputState state;
    RecordingWriter writer;
    writer.failAt = 0;
    EXPECT_EQ(state.send(KeyEvent(SC_A, KeyAction::Press), writer.write, &writer), -EIO);
    writer.failAt = 2;
    EXPECT_EQ(state.releaseAll(writer.write, &writer), -EIO);
    writer.failAt = -1;
    ASSERT_EQ(state.releaseAll(writer.write, &writer), 0);
    const decltype(writer.events) expected = {{EV_KEY, KEY_A, 1},
                                              {EV_KEY, KEY_A, 0},
                                              {EV_SYN, SYN_REPORT, 0},
                                              {EV_KEY, KEY_A, 0},
                                              {EV_SYN, SYN_REPORT, 0}};
    EXPECT_EQ(writer.events, expected);
}

TEST(LinuxHardwareAdapter, CleanupAttemptsAllHeldKeysDespiteOneWriteFailure) {
    OutputState state;
    RecordingWriter writer;
    ASSERT_EQ(state.send(KeyEvent(SC_A, KeyAction::Press), writer.write, &writer), 0);
    ASSERT_EQ(state.send(KeyEvent(SC_B, KeyAction::Press), writer.write, &writer), 0);
    writer.failAt = 4;
    EXPECT_EQ(state.releaseAll(writer.write, &writer), -EIO);
    EXPECT_EQ(writer.events.back(),
              (std::make_tuple<unsigned int, unsigned int, int>(EV_SYN, 0, 0)));
    EXPECT_EQ(writer.events[5],
              (std::make_tuple<unsigned int, unsigned int, int>(EV_KEY, KEY_B, 0)));
    writer.failAt = -1;
    EXPECT_EQ(state.releaseAll(writer.write, &writer), 0);
    EXPECT_EQ(writer.events[7],
              (std::make_tuple<unsigned int, unsigned int, int>(EV_KEY, KEY_A, 0)));
}

TEST(LinuxHardwareAdapter, RepeatWithoutAHeldOutputIsRejectedWithoutInjection) {
    OutputState state;
    RecordingWriter writer;
    EXPECT_EQ(state.send(KeyEvent(SC_A, KeyAction::Repeat), writer.write, &writer), -EINVAL);
    EXPECT_TRUE(writer.events.empty());
}

TEST(LinuxHardwareAdapter, CleanupReleasesOrdinaryKeysBeforeModifiers) {
    OutputState state;
    RecordingWriter writer;
    ASSERT_EQ(state.send(KeyEvent(SC_LCTRL, KeyAction::Press), writer.write, &writer), 0);
    ASSERT_EQ(state.send(KeyEvent(SC_A, KeyAction::Press), writer.write, &writer), 0);
    writer.events.clear();
    ASSERT_EQ(state.releaseAll(writer.write, &writer), 0);
    const decltype(writer.events) expected = {{EV_KEY, KEY_A, 0},
                                              {EV_SYN, SYN_REPORT, 0},
                                              {EV_KEY, KEY_LEFTCTRL, 0},
                                              {EV_SYN, SYN_REPORT, 0}};
    EXPECT_EQ(writer.events, expected);
}

TEST(LinuxHardwareLifecycle, MissingSelectionFailsWithoutOpeningDevicesAndErrorSurvivesShutdown) {
    LinuxHardwareIO hardware;
    EXPECT_FALSE(hardware.initialize());
    EXPECT_TRUE(hardware.hasFailed());
    EXPECT_FALSE(hardware.isInitialized());
    const std::string error(hardware.lastError());
    EXPECT_NE(error.find("--device"), std::string::npos);
    hardware.shutdown();
    hardware.shutdown();
    EXPECT_EQ(hardware.lastError(), error);
    EXPECT_FALSE(hardware.activate());
}

TEST(LinuxHardwareLifecycle, OutputPreflightRejectsSentinelsAndUnsupportedConfiguredKeys) {
    LinuxHardwareIO hardware;
    EXPECT_TRUE(hardware.supportsKey(SC_PAUSE));
    EXPECT_TRUE(hardware.supportsKey(SC_RCTRL));
    EXPECT_FALSE(hardware.supportsKey(SC_NOP));
    EXPECT_FALSE(hardware.supportsKey(SC_UNMAPPED));
    EXPECT_FALSE(hardware.supportsKey(KEY_F13));
    EXPECT_TRUE(hardware.sendEvent(KeyEvent(SC_NOP, true)));
    EXPECT_FALSE(hardware.sendEvent(KeyEvent(SC_A, true)));
}

TEST(LinuxHardwareLifecycle, CancellationPreservesErrnoAndUninitializedWaitsNeverBlock) {
    LinuxHardwareIO hardware;
    errno = EBUSY;
    hardware.requestStop();
    EXPECT_EQ(errno, EBUSY);
    EXPECT_FALSE(hardware.waitForKey(0));
    EXPECT_FALSE(hardware.waitForKey(-1));
    EXPECT_FALSE(hardware.waitForKey(50));
    EXPECT_FALSE(hardware.hasFailed());
}

TEST(LinuxHardwareLifecycle, RelativeDevicePathsAreRejectedBeforeAnyDeviceAccess) {
    LinuxHardwareIO hardware;
    EXPECT_FALSE(hardware.configureDevices({"event0"}));
    EXPECT_TRUE(hardware.hasFailed());
    EXPECT_NE(hardware.lastError().find("absolute"), std::string_view::npos);
}

TEST(LinuxHardwareLifecycle, CancelledStartupDoesNotOpenConfiguredDevices) {
    LinuxHardwareIO hardware;
    ASSERT_TRUE(hardware.configureDevices({"/dev/input/not-opened-by-unit-test"}));
    hardware.requestStop();
    EXPECT_FALSE(hardware.initialize());
    EXPECT_FALSE(hardware.isInitialized());
    EXPECT_FALSE(hardware.hasFailed());
    EXPECT_TRUE(hardware.outputDevicePath().empty());
}

} // namespace
} // namespace keyflow::linux_backend

#endif
