#include "util/notify.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>

#include "bagu/error.h"

namespace bagu::util {

namespace {

/// 把字符串转成 single-quoted shell 安全形式：'a'\''b' → 字面量 a'b
std::string shell_escape_single(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char c : s) {
        if (c == '\'') out.append("'\\''");
        else           out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

/// `which <cmd> >/dev/null 2>&1` → bool
bool which(const std::string& cmd) {
    std::string c = "command -v " + shell_escape_single(cmd) + " >/dev/null 2>&1";
    return std::system(c.c_str()) == 0;
}

}  // namespace

bool desktop_notify_available() {
#if defined(__APPLE__)
    return which("osascript");
#elif defined(__linux__)
    return which("notify-send");
#else
    return false;
#endif
}

Result<void> notify_desktop(const std::string& title, const std::string& body) {
#if defined(__APPLE__)
    if (!which("osascript")) {
        return make_err(E::kFileNotFound,
            "osascript 不可用（macOS 系统命令缺失？）");
    }
    // AppleScript 的 display notification "body" with title "title"
    // 注意：要双引号包字符串，里面双引号转义为 \"
    auto applescript_escape = [](const std::string& s) {
        std::string out; out.reserve(s.size() + 4);
        for (char c : s) {
            if (c == '"' || c == '\\') out.push_back('\\');
            out.push_back(c);
        }
        return out;
    };
    std::ostringstream cmd;
    cmd << "osascript -e " << shell_escape_single(
        std::string("display notification \"") + applescript_escape(body) +
        "\" with title \"" + applescript_escape(title) + "\"");
    int rc = std::system(cmd.str().c_str());
    if (rc != 0) {
        return make_err(E::kInternal,
            "osascript 退出码 " + std::to_string(rc));
    }
    return Result<void>::ok();
#elif defined(__linux__)
    if (!which("notify-send")) {
        return make_err(E::kFileNotFound,
            "notify-send 不可用（apt install libnotify-bin）");
    }
    std::ostringstream cmd;
    cmd << "notify-send --app-name=bagu --icon=accessories-text-editor "
        << shell_escape_single(title) << " "
        << shell_escape_single(body);
    int rc = std::system(cmd.str().c_str());
    // notify-send 在 headless / DBus 不通时也可能 rc=0；不强求精确
    if (rc != 0) {
        return make_err(E::kInternal,
            "notify-send 退出码 " + std::to_string(rc));
    }
    return Result<void>::ok();
#else
    (void)title; (void)body;
    return make_err(E::kNotImplemented,
        "当前平台暂不支持桌面通知（仅 Linux/macOS）");
#endif
}

}  // namespace bagu::util
