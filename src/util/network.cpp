#include "util/network.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace bagu::util {

namespace {

/// 名字看着像虚拟网桥 / docker / loopback / WSL 内部 → 跳过
bool is_uninteresting_iface(const std::string& name) {
    static const std::vector<std::string> prefixes = {
        "lo", "docker", "veth", "br-", "virbr", "tun", "tap",
        "vmnet", "podman", "cni",
    };
    for (const auto& p : prefixes) {
        if (name.compare(0, p.size(), p) == 0) return true;
    }
    return false;
}

}  // namespace

std::vector<std::string> local_ipv4_addresses() {
    std::vector<std::string> out;
#if defined(__linux__) || defined(__APPLE__)
    struct ifaddrs* ifs = nullptr;
    if (getifaddrs(&ifs) != 0) return out;

    for (auto* p = ifs; p; p = p->ifa_next) {
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
        std::string name = p->ifa_name ? p->ifa_name : "";
        if (is_uninteresting_iface(name)) continue;
        char buf[INET_ADDRSTRLEN] = {};
        auto* sin = reinterpret_cast<struct sockaddr_in*>(p->ifa_addr);
        if (!inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) continue;
        std::string ip = buf;
        // 跳过明显是 link-local / 内部 fakeIP（Clash 等）
        if (ip.compare(0, 4, "169.") == 0) continue;
        if (ip.compare(0, 6, "198.18") == 0) continue;  // Clash fakeIP 范围
        if (ip == "127.0.0.1") continue;
        out.push_back(std::move(ip));
    }
    freeifaddrs(ifs);

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
#endif
    return out;
}

}  // namespace bagu::util
