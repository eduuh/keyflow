#ifdef __linux__

#include "platform/linux/LinuxPlatformInit.h"
#include "platform/linux/LinuxSingleInstanceLock.h"
#include "platform/linux/LinuxSystemTray.h"

#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace keyflow;

namespace {

class EnvironmentVariable {
public:
  explicit EnvironmentVariable(const char *name) : name_(name) {
    const char *previous = std::getenv(name_);
    existed_ = previous != nullptr;
    if (existed_) {
      previous_ = previous;
    }
  }
  ~EnvironmentVariable() {
    if (existed_) {
      setenv(name_, previous_.c_str(), 1);
    } else {
      unsetenv(name_);
    }
  }
  void set(const char *value) {
    if (value != nullptr) {
      ASSERT_EQ(setenv(name_, value, 1), 0);
    } else {
      ASSERT_EQ(unsetenv(name_), 0);
    }
  }

private:
  const char *name_;
  bool existed_;
  std::string previous_;
};

class ChildProcess {
public:
  explicit ChildProcess(pid_t pid) : pid_(pid) {}
  ~ChildProcess() {
    if (pid_ > 0) {
      kill(pid_, SIGKILL);
      reap();
    }
  }
  int reap() {
    int status = 0;
    pid_t result;
    do {
      result = waitpid(pid_, &status, 0);
    } while (result == -1 && errno == EINTR);
    EXPECT_EQ(result, pid_);
    pid_ = -1;
    return status;
  }

private:
  pid_t pid_;
};

class Pipe {
public:
  Pipe() { EXPECT_EQ(pipe2(descriptors_, O_CLOEXEC), 0); }
  ~Pipe() {
    close(descriptors_[0]);
    close(descriptors_[1]);
  }
  int reader() const { return descriptors_[0]; }
  int writer() const { return descriptors_[1]; }
  void closeWriter() {
    close(descriptors_[1]);
    descriptors_[1] = -1;
  }

private:
  int descriptors_[2] = {-1, -1};
};

class LinuxLockTest : public ::testing::Test {
protected:
  void SetUp() override {
    char pattern[] = "/tmp/keyflow-lifecycle-XXXXXX";
    const char *directory = mkdtemp(pattern);
    ASSERT_NE(directory, nullptr);
    directory_ = directory;
    runtime_.set(directory_.c_str());
  }
  void TearDown() override {
    if (!directory_.empty()) {
      std::error_code error;
      std::filesystem::remove_all(directory_, error);
      EXPECT_FALSE(error) << error.message();
    }
  }
  std::string lockPath() const { return directory_ + "/keyflow.lock"; }

  EnvironmentVariable runtime_{"XDG_RUNTIME_DIR"};
  std::string directory_;
};

TEST_F(LinuxLockTest, RejectsUnavailableRuntimeDirectory) {
  LinuxSingleInstanceLock lock;
  runtime_.set(nullptr);
  EXPECT_FALSE(lock.acquire());
  runtime_.set("");
  EXPECT_FALSE(lock.acquire());
  runtime_.set("relative");
  EXPECT_FALSE(lock.acquire());
  runtime_.set((directory_ + "/missing").c_str());
  EXPECT_FALSE(lock.acquire());
}

TEST_F(LinuxLockTest, RejectsRuntimeDirectoryAccessibleToOtherUsers) {
  ASSERT_EQ(chmod(directory_.c_str(), 0755), 0);
  LinuxSingleInstanceLock lock;
  EXPECT_FALSE(lock.acquire());
  EXPECT_FALSE(std::filesystem::exists(lockPath()));
}

TEST_F(LinuxLockTest, RejectsRuntimeDirectorySymlinkIncludingTrailingSlash) {
  const auto link = directory_ + "/runtime";
  ASSERT_EQ(symlink(directory_.c_str(), link.c_str()), 0);
  runtime_.set(link.c_str());
  LinuxSingleInstanceLock lock;
  EXPECT_FALSE(lock.acquire());
  runtime_.set((link + "/").c_str());
  EXPECT_FALSE(lock.acquire());
  runtime_.set((link + "/.").c_str());
  EXPECT_FALSE(lock.acquire());
  EXPECT_FALSE(std::filesystem::exists(lockPath()));
}

TEST_F(LinuxLockTest, AcceptsPrivateRuntimeDirectoryWithTrailingSlash) {
  runtime_.set((directory_ + "/").c_str());
  LinuxSingleInstanceLock lock;
  EXPECT_TRUE(lock.acquire());
}

TEST_F(LinuxLockTest, RejectsUnsafeExistingLockWithoutChangingContents) {
  const int descriptor = open(lockPath().c_str(), O_CREAT | O_WRONLY, 0644);
  ASSERT_NE(descriptor, -1);
  ASSERT_EQ(write(descriptor, "owner", 5), 5);
  close(descriptor);
  ASSERT_EQ(chmod(lockPath().c_str(), 0644), 0);

  LinuxSingleInstanceLock lock;
  EXPECT_FALSE(lock.acquire());
  EXPECT_EQ(std::filesystem::file_size(lockPath()), 5u);
  struct stat status{};
  ASSERT_EQ(stat(lockPath().c_str(), &status), 0);
  EXPECT_EQ(status.st_mode & 0777, 0644u);
}

TEST_F(LinuxLockTest, RejectsSymlinkAndHardLinkedLockFiles) {
  const auto target = directory_ + "/target";
  const int descriptor = open(target.c_str(), O_CREAT | O_WRONLY, 0600);
  ASSERT_NE(descriptor, -1);
  ASSERT_EQ(write(descriptor, "owner", 5), 5);
  close(descriptor);
  ASSERT_EQ(symlink(target.c_str(), lockPath().c_str()), 0);
  LinuxSingleInstanceLock lock;
  EXPECT_FALSE(lock.acquire());
  ASSERT_EQ(unlink(lockPath().c_str()), 0);
  ASSERT_EQ(link(target.c_str(), lockPath().c_str()), 0);
  EXPECT_FALSE(lock.acquire());
  EXPECT_EQ(std::filesystem::file_size(target), 5u);
}

TEST_F(LinuxLockTest, RejectsNonRegularLockWithoutBlocking) {
  ASSERT_EQ(mkfifo(lockPath().c_str(), 0600), 0);
  LinuxSingleInstanceLock lock;
  EXPECT_FALSE(lock.acquire());
}

TEST_F(LinuxLockTest, RejectsDuplicateAndReacquiresAfterIdempotentRelease) {
  LinuxSingleInstanceLock owner;
  LinuxSingleInstanceLock contender;
  ASSERT_TRUE(owner.acquire());
  EXPECT_TRUE(owner.acquire());
  EXPECT_FALSE(contender.acquire());
  owner.release();
  owner.release();
  EXPECT_TRUE(contender.acquire());
  EXPECT_FALSE(owner.acquire());
}

TEST_F(LinuxLockTest, DestructorReleasesOwnershipWithoutRemovingInode) {
  ino_t inode;
  {
    LinuxSingleInstanceLock owner;
    ASSERT_TRUE(owner.acquire());
    struct stat status{};
    ASSERT_EQ(stat(lockPath().c_str(), &status), 0);
    inode = status.st_ino;
    EXPECT_EQ(status.st_mode & 0777, 0600u);
  }
  LinuxSingleInstanceLock next;
  ASSERT_TRUE(next.acquire());
  struct stat status{};
  ASSERT_EQ(stat(lockPath().c_str(), &status), 0);
  EXPECT_EQ(status.st_ino, inode);
}

TEST_F(LinuxLockTest,
       ExistingContenderDescriptorStillExcludesNewOwnersAfterRelease) {
  LinuxSingleInstanceLock owner;
  ASSERT_TRUE(owner.acquire());
  const int contender = open(lockPath().c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_NE(contender, -1);
  EXPECT_EQ(flock(contender, LOCK_EX | LOCK_NB), -1);
  owner.release();
  EXPECT_EQ(flock(contender, LOCK_EX | LOCK_NB), 0);
  LinuxSingleInstanceLock next;
  EXPECT_FALSE(next.acquire());
  close(contender);
  EXPECT_TRUE(next.acquire());
}

TEST_F(LinuxLockTest, DuplicateDoesNotKillOwnerAndOwnershipRecoversAfterDeath) {
  Pipe ready;
  const pid_t pid = fork();
  ASSERT_NE(pid, -1);
  if (pid == 0) {
    LinuxSingleInstanceLock owner;
    const char acquired = owner.acquire() ? '1' : '0';
    if (write(ready.writer(), &acquired, 1) != 1) {
      _exit(2);
    }
    for (;;) {
      pause();
    }
  }
  ChildProcess child(pid);
  ready.closeWriter();
  char acquired = '0';
  ASSERT_EQ(read(ready.reader(), &acquired, 1), 1);
  ASSERT_EQ(acquired, '1');
  LinuxSingleInstanceLock contender;
  EXPECT_FALSE(contender.acquire());
  EXPECT_EQ(kill(pid, 0), 0);
  ASSERT_EQ(kill(pid, SIGKILL), 0);
  const int status = child.reap();
  ASSERT_TRUE(WIFSIGNALED(status));
  EXPECT_EQ(WTERMSIG(status), SIGKILL);
  EXPECT_TRUE(contender.acquire());
}

TEST_F(LinuxLockTest, ExecDoesNotRetainInheritedOwnership) {
  LinuxSingleInstanceLock owner;
  ASSERT_TRUE(owner.acquire());
  Pipe executed;
  const pid_t pid = fork();
  ASSERT_NE(pid, -1);
  if (pid == 0) {
    execl("/bin/sleep", "sleep", "30", static_cast<char *>(nullptr));
    const char failed = '0';
    [[maybe_unused]] const auto written = write(executed.writer(), &failed, 1);
    _exit(2);
  }
  ChildProcess child(pid);
  executed.closeWriter();
  char failed = '0';
  ASSERT_EQ(read(executed.reader(), &failed, 1), 0);
  ASSERT_EQ(kill(pid, 0), 0);
  owner.release();
  LinuxSingleInstanceLock contender;
  EXPECT_TRUE(contender.acquire());
}

int callbackCount = 0;
int replacementCount = 0;
volatile std::sig_atomic_t restoredSignalCount = 0;

void shutdownCallback() { ++callbackCount; }
void replacementCallback() { ++replacementCount; }
void originalHandler(int /*signal*/) { restoredSignalCount = 1; }

class LinuxSignalTest : public ::testing::Test {
protected:
  void SetUp() override {
    callbackCount = 0;
    replacementCount = 0;
    restoredSignalCount = 0;
    struct sigaction action{};
    action.sa_handler = originalHandler;
    sigemptyset(&action.sa_mask);
    ASSERT_EQ(sigaction(SIGINT, &action, &interrupt_), 0);
    ASSERT_EQ(sigaction(SIGTERM, &action, &terminate_), 0);
    ASSERT_EQ(sigaction(SIGABRT, nullptr, &abort_), 0);
  }
  void TearDown() override {
    EXPECT_EQ(sigaction(SIGINT, &interrupt_, nullptr), 0);
    EXPECT_EQ(sigaction(SIGTERM, &terminate_, nullptr), 0);
    EXPECT_EQ(sigaction(SIGABRT, &abort_, nullptr), 0);
  }

  struct sigaction interrupt_{};
  struct sigaction terminate_{};
  struct sigaction abort_{};
};

TEST_F(LinuxSignalTest, DeliversShutdownOnlyWhenPumpedAndOnlyOnce) {
  LinuxPlatformInit platform;
  platform.installSignalHandlers(shutdownCallback);
  platform.processSignals();
  EXPECT_EQ(callbackCount, 0);
  ASSERT_EQ(raise(SIGINT), 0);
  ASSERT_EQ(raise(SIGTERM), 0);
  EXPECT_EQ(callbackCount, 0);
  platform.processSignals();
  EXPECT_EQ(callbackCount, 1);
  ASSERT_EQ(raise(SIGTERM), 0);
  platform.processSignals();
  EXPECT_EQ(callbackCount, 1);
}

TEST_F(LinuxSignalTest,
       ReinstallationUpdatesCallbackWithoutLosingPendingShutdownOrRedelivery) {
  LinuxPlatformInit platform;
  platform.installSignalHandlers(shutdownCallback);
  ASSERT_EQ(raise(SIGTERM), 0);
  platform.installSignalHandlers(replacementCallback);
  platform.processSignals();
  EXPECT_EQ(callbackCount, 0);
  EXPECT_EQ(replacementCount, 1);
  platform.installSignalHandlers(shutdownCallback);
  ASSERT_EQ(raise(SIGINT), 0);
  platform.processSignals();
  EXPECT_EQ(callbackCount, 0);
}

TEST_F(LinuxSignalTest,
       DestructionDropsPendingCallbackAndRestoresOriginalHandlers) {
  {
    LinuxPlatformInit platform;
    platform.installSignalHandlers(shutdownCallback);
    ASSERT_EQ(raise(SIGINT), 0);
  }
  ASSERT_EQ(raise(SIGTERM), 0);
  EXPECT_EQ(restoredSignalCount, 1);
  EXPECT_EQ(callbackCount, 0);
  LinuxPlatformInit next;
  next.installSignalHandlers(replacementCallback);
  next.processSignals();
  EXPECT_EQ(replacementCount, 0);
  ASSERT_EQ(raise(SIGTERM), 0);
  next.processSignals();
  EXPECT_EQ(replacementCount, 1);
}

TEST_F(LinuxSignalTest, NullCallbackDisarmsAndAllowsFreshInstallation) {
  LinuxPlatformInit platform;
  platform.installSignalHandlers(shutdownCallback);
  ASSERT_EQ(raise(SIGINT), 0);
  platform.installSignalHandlers(nullptr);
  platform.installSignalHandlers(nullptr);
  platform.processSignals();
  EXPECT_EQ(callbackCount, 0);
  ASSERT_EQ(raise(SIGINT), 0);
  EXPECT_EQ(restoredSignalCount, 1);
  platform.installSignalHandlers(replacementCallback);
  platform.processSignals();
  EXPECT_EQ(replacementCount, 0);
  ASSERT_EQ(raise(SIGTERM), 0);
  platform.processSignals();
  EXPECT_EQ(replacementCount, 1);
}

TEST_F(LinuxSignalTest, SecondAdapterCannotStealOrDisarmLiveOwner) {
  LinuxPlatformInit owner;
  owner.installSignalHandlers(shutdownCallback);
  {
    LinuxPlatformInit contender;
    contender.installSignalHandlers(replacementCallback);
    ASSERT_EQ(raise(SIGTERM), 0);
    contender.processSignals();
    EXPECT_EQ(replacementCount, 0);
  }
  owner.processSignals();
  EXPECT_EQ(callbackCount, 1);
}

TEST_F(LinuxSignalTest,
       DestructionPreservesHandlerInstalledByAnotherComponent) {
  {
    LinuxPlatformInit platform;
    platform.installSignalHandlers(shutdownCallback);
    struct sigaction action{};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    ASSERT_EQ(sigaction(SIGINT, &action, nullptr), 0);
  }
  struct sigaction current{};
  ASSERT_EQ(sigaction(SIGINT, nullptr, &current), 0);
  EXPECT_EQ(current.sa_handler, SIG_IGN);
  ASSERT_EQ(raise(SIGTERM), 0);
  EXPECT_EQ(restoredSignalCount, 1);
}

TEST_F(LinuxSignalTest, LeavesAbortHandlerUnchanged) {
  LinuxPlatformInit platform;
  platform.installSignalHandlers(shutdownCallback);
  struct sigaction current{};
  ASSERT_EQ(sigaction(SIGABRT, nullptr, &current), 0);
  EXPECT_EQ(current.sa_handler, abort_.sa_handler);
  EXPECT_EQ(current.sa_flags, abort_.sa_flags);
}

std::unique_ptr<LinuxPlatformInit> selfDestroyingPlatform;
void destroyPlatformCallback() {
  ++callbackCount;
  selfDestroyingPlatform.reset();
}

TEST_F(LinuxSignalTest, ShutdownCallbackCanDestroyAdapterSafely) {
  selfDestroyingPlatform = std::make_unique<LinuxPlatformInit>();
  selfDestroyingPlatform->installSignalHandlers(destroyPlatformCallback);
  ASSERT_EQ(raise(SIGTERM), 0);
  selfDestroyingPlatform->processSignals();
  EXPECT_EQ(selfDestroyingPlatform, nullptr);
  EXPECT_EQ(callbackCount, 1);
  ASSERT_EQ(raise(SIGINT), 0);
  EXPECT_EQ(restoredSignalCount, 1);
}

TEST(LinuxPlatformInitTest,
     FatalDiagnosticUsesStringViewLengthAndDoesNotChangeWorkingDirectory) {
  const auto cwd = std::filesystem::current_path();
  LinuxPlatformInit platform;
  ::testing::internal::CaptureStderr();
  platform.showFatalError(std::string_view("fatal suffix", 5));
  EXPECT_EQ(::testing::internal::GetCapturedStderr(), "Keyflow: fatal\n");
  EXPECT_EQ(std::filesystem::current_path(), cwd);
}

TEST(LinuxSystemTrayTest,
     HeadlessAdapterKeepsRunningWithoutDisplayEnvironment) {
  EnvironmentVariable display("DISPLAY");
  EnvironmentVariable wayland("WAYLAND_DISPLAY");
  EnvironmentVariable session("DBUS_SESSION_BUS_ADDRESS");
  display.set(nullptr);
  wayland.set(nullptr);
  session.set(nullptr);
  LinuxSystemTray tray;
  EXPECT_TRUE(tray.initialize("Keyflow"));
  tray.showNotification("Keyflow", "Running headless");
  EXPECT_TRUE(tray.processMessages());
  EXPECT_TRUE(tray.processMessages());
}

} // namespace

#endif
