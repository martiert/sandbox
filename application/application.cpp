#include <sys/types.h>
#include <sys/capability.h>

#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

namespace {

void print_caps()
{
    std::cout << "\n=== Capabilities ===" << std::endl;
    cap_t caps = cap_get_proc();
    ssize_t length;
    auto str = cap_to_text(caps, &length);
    std::cout << str << std::endl;
    cap_free(str);
    cap_free(caps);
}

void print_pid_and_user()
{
    std::cout << "\n=== User/PID info ===" << std::endl;
    pid_t pid = getpid();
    uid_t uid = geteuid();
    uid_t gid = getegid();

    std::cout << "uid: " << uid << std::endl;
    std::cout << "gid: " << gid << std::endl;
    std::cout << "pid: " << pid << std::endl;
}

void print_directories(fs::path path)
{
    std::cout << "\n=== Directories ===" << std::endl;
    for (auto const & p : fs::directory_iterator(path))
        std::cout << p << std::endl;
}

void print_ip(ifaddrs * interface)
{
    int family = interface->ifa_addr->sa_family;
    if (family != AF_INET && family != AF_INET6)
        return;

    char host[NI_MAXHOST];
    int result = getnameinfo(interface->ifa_addr,
            (family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6),
            host, NI_MAXHOST,
            NULL, 0, NI_NUMERICHOST);
    if (result != 0) {
        std::cerr << "getnameinfo() failed: " << gai_strerror(result) << std::endl;
        return;
    }

    std::cout << "\t" << host << std::endl;
}

void print_interfaces(std::multimap<std::string, ifaddrs*> const & if_map)
{
    std::string interface_name;

    for (auto const & interface : if_map) {
        if (interface.first != interface_name) {
            std::cout << interface.first << std::endl;
            interface_name = interface.first;
        }

        print_ip(interface.second);
    }
}

void print_network_interfaces()
{
    std::cout << "\n=== Network interfaces ===" << std::endl;
    ifaddrs * interfaces;
    if (getifaddrs(&interfaces) == -1) {
        std::cerr << "Failed getting interfaces" << std::endl;
        return;
    }

    std::multimap<std::string, ifaddrs*> if_map;
    for (ifaddrs * interface = interfaces; interface != nullptr; interface = interface->ifa_next) {
        if_map.insert(std::make_pair(std::string(interface->ifa_name), interface));
    }

    print_interfaces(if_map);
    freeifaddrs(interfaces);
}

void list_processes()
{
    std::cout << "\n=== Running processes ===" << std::endl;
    for (auto const & p : fs::directory_iterator("/proc")) {
        auto filename = p.path().filename();
        try {
            int pid = std::stoi(filename);
            std::cout << pid << std::endl;
        } catch(std::invalid_argument) {}
    }
}

}

int main(int argc, char ** argv)
{
    std::cerr << "Running application" << std::endl;
    print_pid_and_user();
    print_directories("/");
    print_caps();
    print_network_interfaces();
    list_processes();
}
