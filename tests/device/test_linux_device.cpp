#include "app/OutputDispatcher.h"
#include "fixtures/PipelineHarness.h"
#include "platform/linux/LinuxHardwareIO.h"

#include <gtest/gtest.h>
#include <libevdev/libevdev-uinput.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <memory>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <vector>

namespace {

using namespace keyflow;
using Device = std::unique_ptr<libevdev, decltype(&libevdev_free)>;
using UInput =
    std::unique_ptr<libevdev_uinput, decltype(&libevdev_uinput_destroy)>;
using Frame = std::tuple<unsigned int, unsigned int, int>;

class LinuxDeviceTest : public ::testing::Test {
protected:
  void SetUp() override {
    Device description(libevdev_new(), libevdev_free);
    ASSERT_NE(description, nullptr);
    libevdev_set_name(description.get(), "Synthetic acceptance keyboard");
    for (const auto &mapping : linux_backend::keycodeMappings) {
      ASSERT_EQ(libevdev_enable_event_code(description.get(), EV_KEY,
                                           mapping.evdev, nullptr),
                0);
    }
    ASSERT_EQ(
        libevdev_enable_event_code(description.get(), EV_KEY, KEY_F13, nullptr),
        0);
    libevdev_uinput *source = nullptr;
    ASSERT_EQ(libevdev_uinput_create_from_device(
                  description.get(), LIBEVDEV_UINPUT_OPEN_MANAGED, &source),
              0);
    source_.reset(source);
    const char *path = libevdev_uinput_get_devnode(source_.get());
    ASSERT_NE(path, nullptr);
    ASSERT_TRUE(hardware_.configureDevices({path})) << hardware_.lastError();
    ASSERT_TRUE(hardware_.initialize()) << hardware_.lastError();

    outputFd_ = open(hardware_.outputDevicePath().data(),
                     O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    ASSERT_GE(outputFd_, 0)
        << "Explicit access to the test output node is required";
    libevdev *observer = nullptr;
    ASSERT_EQ(libevdev_new_from_fd(outputFd_, &observer), 0);
    observer_.reset(observer);
    ASSERT_STREQ(libevdev_get_phys(observer_.get()), "keyflow/uinput");
    // Nothing is injected until this exclusive consumer protects the desktop.
    ASSERT_EQ(libevdev_grab(observer_.get(), LIBEVDEV_GRAB), 0);
    ASSERT_TRUE(hardware_.activate()) << hardware_.lastError();
  }

  void TearDown() override {
    if (child_ > 0) {
      kill(child_, SIGKILL);
      while (waitpid(child_, nullptr, 0) < 0 && errno == EINTR) {
      }
      child_ = -1;
    }
    for (int descriptor : connection_) {
      if (descriptor >= 0) {
        close(descriptor);
      }
    }
    hardware_.shutdown();
    observer_.reset();
    if (outputFd_ >= 0) {
      close(outputFd_);
      outputFd_ = -1;
    }
    source_.reset();
  }

  void drive(OutputDispatcher &dispatcher, unsigned int code, int value) {
    ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_KEY, code, value),
              0);
    ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_SYN, SYN_REPORT, 0),
              0);
    const auto event = hardware_.waitForKey(1000);
    ASSERT_TRUE(event) << hardware_.lastError();
    ASSERT_TRUE(dispatcher.process(*event)) << hardware_.lastError();
  }

  std::vector<Frame> readFrames(size_t count) {
    std::vector<Frame> frames;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (frames.size() < count) {
      input_event event{};
      const int result = libevdev_next_event(observer_.get(),
                                             LIBEVDEV_READ_FLAG_NORMAL, &event);
      if (result == LIBEVDEV_READ_STATUS_SUCCESS) {
        frames.emplace_back(event.type, event.code, event.value);
        continue;
      }
      if (result != -EAGAIN) {
        ADD_FAILURE() << "Output consumer read failed: " << result;
        break;
      }
      const auto remaining =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              deadline - std::chrono::steady_clock::now());
      if (remaining.count() <= 0) {
        ADD_FAILURE() << "Timed out observing output";
        break;
      }
      pollfd descriptor{outputFd_, POLLIN, 0};
      if (poll(&descriptor, 1, static_cast<int>(remaining.count())) <= 0) {
        ADD_FAILURE() << "Output consumer did not become readable";
        break;
      }
    }
    return frames;
  }

  LinuxHardwareIO hardware_;
  UInput source_{nullptr, libevdev_uinput_destroy};
  Device observer_{nullptr, libevdev_free};
  int outputFd_ = -1;
  pid_t child_ = -1;
  int connection_[2] = {-1, -1};
};

TEST_F(LinuxDeviceTest,
       RuntimeFixtureKeepsExplicitRepeatAndReleaseAfterTriggerRelease) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  OutputDispatcher dispatcher(harness.pipeline(), hardware_);
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_RIGHTALT, 1));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_K, 1));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_RIGHTALT, 0));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_K, 2));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_K, 0));
  const std::vector<Frame> expected = {
      {EV_KEY, KEY_LEFT, 1}, {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_LEFT, 2}, {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_LEFT, 0}, {EV_SYN, SYN_REPORT, 0}};
  EXPECT_EQ(readFrames(expected.size()), expected);
  EXPECT_EQ(harness.currentModifiers(), 0u);
}

TEST_F(LinuxDeviceTest, DesignerFixtureRetainsItsDistinctNumberRowLayer) {
  auto harness = keyflow::testing::PipelineHarness::fromConfigFile(
      KEYFLOW_DESIGNER_CONFIG_PATH);
  OutputDispatcher dispatcher(harness.pipeline(), hardware_);
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_LEFTALT, 1));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_K, 1));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_LEFTALT, 0));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_K, 2));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_K, 0));
  const std::vector<Frame> expected = {
      {EV_KEY, KEY_1, 1}, {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_1, 2}, {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_1, 0}, {EV_SYN, SYN_REPORT, 0}};
  EXPECT_EQ(readFrames(expected.size()), expected);
}

TEST_F(LinuxDeviceTest, NativePassthroughAndShutdownBalanceOwnedOutputs) {
  Pipeline pipeline;
  OutputDispatcher dispatcher(pipeline, hardware_);
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_F13, 1));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_F13, 0));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_LEFTCTRL, 1));
  ASSERT_NO_FATAL_FAILURE(drive(dispatcher, KEY_A, 1));
  const std::vector<Frame> initial = {
      {EV_KEY, KEY_F13, 1},      {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_F13, 0},      {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_LEFTCTRL, 1}, {EV_SYN, SYN_REPORT, 0},
      {EV_KEY, KEY_A, 1},        {EV_SYN, SYN_REPORT, 0}};
  EXPECT_EQ(readFrames(initial.size()), initial);
  dispatcher.shutdown();
  const std::vector<Frame> expected = {{EV_KEY, KEY_A, 0},
                                       {EV_SYN, SYN_REPORT, 0},
                                       {EV_KEY, KEY_LEFTCTRL, 0},
                                       {EV_SYN, SYN_REPORT, 0}};
  EXPECT_EQ(readFrames(expected.size()), expected);
}

TEST_F(LinuxDeviceTest, SelectedSourceLossStopsTheWholeSession) {
  source_.reset();
  EXPECT_FALSE(hardware_.waitForKey(1000));
  EXPECT_TRUE(hardware_.hasFailed());
  EXPECT_FALSE(hardware_.isInitialized());
  EXPECT_TRUE(hardware_.outputDevicePath().empty());
}

TEST_F(LinuxDeviceTest, OwnVirtualOutputCannotBeSelectedAsInput) {
  LinuxHardwareIO other;
  ASSERT_TRUE(
      other.configureDevices({std::string(hardware_.outputDevicePath())}));
  EXPECT_FALSE(other.initialize());
  EXPECT_NE(other.lastError().find("Keyflow-owned"), std::string_view::npos);
  EXPECT_FALSE(other.isInitialized());
}

TEST_F(LinuxDeviceTest, ASecondCapturerCannotGrabTheSelectedSource) {
  LinuxHardwareIO other;
  ASSERT_TRUE(
      other.configureDevices({libevdev_uinput_get_devnode(source_.get())}));
  ASSERT_TRUE(other.initialize()) << other.lastError();
  EXPECT_FALSE(other.activate());
  EXPECT_TRUE(other.hasFailed());
  EXPECT_FALSE(other.isInitialized());
  EXPECT_TRUE(hardware_.isInitialized());
}

TEST_F(LinuxDeviceTest, CancellationWakesAnActiveIndefiniteWait) {
  std::thread stopper([this] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    hardware_.requestStop();
  });
  const auto start = std::chrono::steady_clock::now();
  const auto event = hardware_.waitForKey(0);
  stopper.join();
  EXPECT_FALSE(event);
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(1));
  EXPECT_FALSE(hardware_.hasFailed());
}

TEST_F(LinuxDeviceTest, HeldSourceAtStartupRefusesCaptureAndRollsBackOutput) {
  ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_KEY, KEY_A, 1), 0);
  ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_SYN, SYN_REPORT, 0),
            0);
  LinuxHardwareIO other;
  ASSERT_TRUE(
      other.configureDevices({libevdev_uinput_get_devnode(source_.get())}));
  ASSERT_TRUE(other.initialize()) << other.lastError();
  EXPECT_FALSE(other.activate());
  EXPECT_NE(other.lastError().find("release every"), std::string_view::npos);
  EXPECT_FALSE(other.isInitialized());
  EXPECT_TRUE(other.outputDevicePath().empty());
  EXPECT_TRUE(hardware_.isInitialized());
  ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_KEY, KEY_A, 0), 0);
  ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_SYN, SYN_REPORT, 0),
            0);
}

TEST_F(LinuxDeviceTest, SelectedQueueOverflowStopsInsteadOfReplayingLostInput) {
  for (int index = 0; index < 65536; ++index) {
    ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_KEY, KEY_A,
                                          index % 2 == 0 ? 1 : 0),
              0);
    ASSERT_EQ(libevdev_uinput_write_event(source_.get(), EV_SYN, SYN_REPORT, 0),
              0);
  }
  EXPECT_FALSE(hardware_.waitForKey(1000));
  EXPECT_TRUE(hardware_.hasFailed());
  EXPECT_NE(hardware_.lastError().find("SYN_DROPPED"), std::string_view::npos);
  EXPECT_FALSE(hardware_.isInitialized());
  EXPECT_TRUE(hardware_.outputDevicePath().empty());
}

TEST_F(LinuxDeviceTest,
       AbruptOwnerDeathRemovesHeldOutputAndReleasesSourceGrab) {
  hardware_.shutdown();
  observer_.reset();
  close(outputFd_);
  outputFd_ = -1;
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, connection_),
            0);
  child_ = fork();
  ASSERT_GE(child_, 0);
  if (child_ == 0) {
    close(connection_[0]);
    LinuxHardwareIO childHardware;
    if (!childHardware.configureDevices(
            {libevdev_uinput_get_devnode(source_.get())}) ||
        !childHardware.initialize()) {
      _exit(2);
    }
    const std::string path(childHardware.outputDevicePath());
    if (send(connection_[1], path.data(), path.size(), MSG_NOSIGNAL) !=
        static_cast<ssize_t>(path.size())) {
      _exit(3);
    }
    char ready = 0;
    if (recv(connection_[1], &ready, 1, 0) != 1 || ready != 'G' ||
        !childHardware.activate() ||
        !childHardware.sendEvent(KeyEvent(SC_A, KeyAction::Press))) {
      _exit(4);
    }
    if (send(connection_[1], "P", 1, MSG_NOSIGNAL) != 1) {
      _exit(5);
    }
    for (;;) {
      pause();
    }
  }
  close(connection_[1]);
  connection_[1] = -1;
  pollfd channel{connection_[0], POLLIN, 0};
  ASSERT_GT(poll(&channel, 1, 3000), 0);
  char path[512]{};
  const auto length = recv(connection_[0], path, sizeof(path) - 1, 0);
  ASSERT_GT(length, 0);
  outputFd_ = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  ASSERT_GE(outputFd_, 0);
  libevdev *observer = nullptr;
  ASSERT_EQ(libevdev_new_from_fd(outputFd_, &observer), 0);
  observer_.reset(observer);
  ASSERT_STREQ(libevdev_get_phys(observer_.get()), "keyflow/uinput");
  ASSERT_EQ(libevdev_grab(observer_.get(), LIBEVDEV_GRAB), 0);
  ASSERT_EQ(send(connection_[0], "G", 1, MSG_NOSIGNAL), 1);
  ASSERT_GT(poll(&channel, 1, 3000), 0);
  char pressed = 0;
  ASSERT_EQ(recv(connection_[0], &pressed, 1, 0), 1);
  ASSERT_EQ(pressed, 'P');
  const std::vector<Frame> expected = {{EV_KEY, KEY_A, 1},
                                       {EV_SYN, SYN_REPORT, 0}};
  ASSERT_EQ(readFrames(expected.size()), expected);
  ASSERT_EQ(kill(child_, SIGKILL), 0);
  int status = 0;
  ASSERT_EQ(waitpid(child_, &status, 0), child_);
  child_ = -1;
  ASSERT_TRUE(WIFSIGNALED(status));
  EXPECT_EQ(WTERMSIG(status), SIGKILL);
  int version = 0;
  EXPECT_EQ(ioctl(outputFd_, EVIOCGVERSION, &version), -1);
  EXPECT_EQ(errno, ENODEV);
  close(connection_[0]);
  connection_[0] = open(libevdev_uinput_get_devnode(source_.get()),
                        O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  ASSERT_GE(connection_[0], 0);
  libevdev *input = nullptr;
  ASSERT_EQ(libevdev_new_from_fd(connection_[0], &input), 0);
  Device recovered(input, libevdev_free);
  ASSERT_EQ(libevdev_grab(recovered.get(), LIBEVDEV_GRAB), 0);
  EXPECT_EQ(libevdev_grab(recovered.get(), LIBEVDEV_UNGRAB), 0);
}

} // namespace
