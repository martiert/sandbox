#include <iostream>
#include <fstream>

#include <unistd.h>
#include <dlfcn.h>

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

void set_mapping(std::string const & filename, uid_t inside_ns, uid_t outside_ns)
{
    std::ofstream fs(filename);
    fs << inside_ns << " " << outside_ns << " " << 1;
}

int my_main(int argc, char ** argv, char ** argenv)
{
    std::cout << "!!!!!! Running sandboxed !!!!!!" << std::endl;
    uid_t uid = geteuid();
    uid_t gid = getegid();

    unshare(CLONE_NEWUSER);
    setgroups_deny();
    set_mapping("/proc/self/uid_map", 0, uid);
    set_mapping("/proc/self/gid_map", 0, gid);

    return actual_main(argc, argv, argenv);
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
