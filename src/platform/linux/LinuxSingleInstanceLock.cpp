#include "platform/linux/LinuxSingleInstanceLock.h"

#ifdef __linux__

#    include <cstdio>
#    include <cstdlib>
#    include <cstring>
#    include <fcntl.h>
#    include <limits.h>
#    include <sys/file.h>
#    include <sys/stat.h>
#    include <unistd.h>

namespace keyflow {
namespace {

constexpr const char* lockName = "keyflow.lock";

bool fail(const char* message) noexcept {
    std::fprintf(stderr, "Keyflow: %s\n", message);
    return false;
}

int openDirectoryWithoutSymlinks(char* path) noexcept {
    int directory = open("/", O_PATH | O_DIRECTORY | O_CLOEXEC);
    char* position = nullptr;
    char* component = strtok_r(path, "/", &position);
    // Walk every component: O_NOFOLLOW on the full path alone allows link/..
    // and link/. to bypass the runtime directory's symlink check.
    while (directory != -1 && component != nullptr) {
        const int next =
            openat(directory, component, O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        close(directory);
        directory = next;
        component = strtok_r(nullptr, "/", &position);
    }
    return directory;
}

int openRuntimeDirectory() noexcept {
    const char* runtime = std::getenv("XDG_RUNTIME_DIR");
    if (runtime == nullptr || runtime[0] != '/') {
        fail("XDG_RUNTIME_DIR must name an absolute, private directory owned by this user");
        return -1;
    }
    char path[PATH_MAX];
    const auto length = std::strlen(runtime);
    if (length >= sizeof(path)) {
        fail("XDG_RUNTIME_DIR path is too long");
        return -1;
    }
    std::memcpy(path, runtime, length + 1);
    const int directory = openDirectoryWithoutSymlinks(path);
    if (directory == -1) {
        fail("cannot open XDG_RUNTIME_DIR (missing, inaccessible or a symlink)");
        return -1;
    }
    struct stat status{};
    if (fstat(directory, &status) != 0 || !S_ISDIR(status.st_mode) || status.st_uid != geteuid() ||
        (status.st_mode & 07777) != 0700) {
        close(directory);
        fail("unsafe XDG_RUNTIME_DIR: require user ownership and mode 0700");
        return -1;
    }
    return directory;
}

} // namespace

LinuxSingleInstanceLock::~LinuxSingleInstanceLock() {
    release();
}

bool LinuxSingleInstanceLock::acquire() noexcept {
    if (descriptor_ != -1) {
        return true;
    }
    const int directory = openRuntimeDirectory();
    if (directory == -1) {
        return false;
    }
    const int descriptor =
        openat(directory, lockName, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0600);
    if (descriptor == -1) {
        close(directory);
        return fail("cannot open private keyflow.lock in XDG_RUNTIME_DIR");
    }
    struct stat status{};
    const bool safe = fstat(descriptor, &status) == 0 && S_ISREG(status.st_mode) &&
                      status.st_uid == geteuid() && (status.st_mode & 07777) == 0600 &&
                      status.st_nlink == 1;
    if (!safe || flock(descriptor, LOCK_EX | LOCK_NB) != 0) {
        close(descriptor);
        close(directory);
        return fail(safe ? "another Keyflow instance owns this user's runtime lock"
                         : "unsafe keyflow.lock: require a private, user-owned regular file");
    }
    struct stat namedStatus{};
    const bool unchanged = fstatat(directory, lockName, &namedStatus, AT_SYMLINK_NOFOLLOW) == 0 &&
                           namedStatus.st_dev == status.st_dev &&
                           namedStatus.st_ino == status.st_ino;
    close(directory);
    if (!unchanged) {
        close(descriptor);
        return fail("keyflow.lock changed while acquiring ownership");
    }
    descriptor_ = descriptor;
    return true;
}

void LinuxSingleInstanceLock::release() noexcept {
    if (descriptor_ != -1) {
        // Never unlink: other contenders must continue locking the same inode.
        close(descriptor_);
        descriptor_ = -1;
    }
}

} // namespace keyflow

#endif
