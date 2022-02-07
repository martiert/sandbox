#include <iostream>

#include <dlfcn.h>

namespace
{

template<typename F>
F dlsym_next(const char * symbol)
{
    return reinterpret_cast<F>(dlsym(RTLD_NEXT, symbol));
}

}


extern "C" {

static int (*actual_main)(int, char**, char**) = nullptr;

static int my_main(int argc, char ** argv, char ** argenv)
{
    std::cout << "!!!!!! Running sandboxed !!!!!!" << std::endl;
    actual_main(argc, argv, argenv);
    return 0;
}

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
