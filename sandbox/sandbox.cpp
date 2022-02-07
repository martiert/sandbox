#include <iostream>
#include <fstream>
#include <filesystem>

#include <unistd.h>
#include <dlfcn.h>

#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <string.h>

namespace fs = std::filesystem;

namespace
{

int (*actual_main)(int, char**, char**) = nullptr;

template<typename F>
F dlsym_next(const char * symbol)
{
    return reinterpret_cast<F>(dlsym(RTLD_NEXT, symbol));
}

void setgroups_deny()
{
    std::ofstream fs("/proc/self/setgroups");
    fs << "deny";
}

void do_not_inherit_from_root()
{
    if (mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) != 0) {
        std::cerr << "Failed making filesystem private: " << strerror(errno) << std::endl;
        _exit(1);
    }
}

void set_mapping(std::string const & filename, uid_t inside_ns, uid_t outside_ns)
{
    std::ofstream fs(filename);
    fs << inside_ns << " " << outside_ns << " " << 1;
}

void create_tmpfs(fs::path const & rootfs)
{
    if (mount("tmpfs", rootfs.c_str(), "tmpfs", 0, NULL) != 0) {
        std::cerr << "Failed creating tmpfs at " << rootfs << " " << strerror(errno) << std::endl;
        _exit(1);
    }
}

void do_bind_mount(fs::path const & source, fs::path const & dest)
{
    if (mount(source.c_str(), dest.c_str(), nullptr, MS_REC | MS_BIND, nullptr) != 0) {
        std::cerr << "Failed mounting " << source << " to " << dest << " " << strerror(errno) << std::endl;
        _exit(1);
    }
}

void bind_mount(fs::path const & basepath, std::string const & name)
{
    fs::path to = basepath / name;

    if (!fs::create_directories(to)) {
        std::cerr << "Failed create directory " << to << ": " << strerror(errno) << std::endl;
        _exit(1);
    }

    fs::path from("/");
    from = from / name;
    do_bind_mount(from, to);
}

int pivot_root(char const * new_root, char const * put_old)
{
    return syscall(SYS_pivot_root, new_root, put_old);
}

void create_procfs()
{
    if (!fs::create_directories("/proc")) {
        std::cerr << "Failed create /proc directory: " << strerror(errno) << std::endl;
        _exit(1);
    }

    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        std::cerr << "Failed mounting /proc: " << strerror(errno) << std::endl;
        _exit(1);
    }
}

void set_new_root(fs::path const & rootfs)
{
    auto old_root = rootfs / "oldroot";
    fs::create_directories(old_root);
    pivot_root(rootfs.c_str(), old_root.c_str());

    auto value = chdir("/");
    (void) value;
    create_procfs();
    if (umount2("/oldroot", MNT_DETACH) != 0)
        std::cerr << "Failed umounting oldroot: " << strerror(errno) << std::endl;
    fs::remove("/oldroot");
}

int my_main(int argc, char ** argv, char ** argenv)
{
    std::cout << "!!!!!! Running sandboxed !!!!!!" << std::endl;
    uid_t uid = geteuid();
    uid_t gid = getegid();

    unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID);
    do_not_inherit_from_root();

    setgroups_deny();
    set_mapping("/proc/self/uid_map", 0, uid);
    set_mapping("/proc/self/gid_map", 0, gid);

    fs::path rootfs("/tmp");
    create_tmpfs(rootfs);
    bind_mount(rootfs, "run");
    bind_mount(rootfs, "sys");

    pid_t child = fork();
    if (child == 0) {
        unshare(CLONE_NEWNET);

        set_new_root(rootfs);
        int result = actual_main(argc, argv, argenv);
        _exit(result);
    }

    int status = -1;
    waitpid(child, &status, 0);
    return status;
}

}


extern "C" {

int __libc_start_main(int (*main) (int, char**, char **),
                        int argc,
                        char * * ubp_av,
                        void (*init) (void),
                        void (*fini) (void),
                        void (*rtld_fini) (void),
                        void (* stack_end))
{
    actual_main = main;
    auto actual_start_main = dlsym_next<int (*)(int (*)(int,char**,char**), int, char**, void(*)(void), void(*)(void),void(*)(void), void*)>("__libc_start_main");
    actual_start_main(my_main, argc, ubp_av, init, fini, rtld_fini, stack_end);

    return 0;
}

}
