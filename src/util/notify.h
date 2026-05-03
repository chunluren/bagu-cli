#pragma once

#include <string>

#include "bagu/result.h"

namespace bagu::util {

/// 跨平台桌面通知。失败返回错误（一般是没装相关命令）。
///
/// 平台 → 命令：
///   Linux   notify-send <title> <body>
///   macOS   osascript -e 'display notification ...'
///   Windows powershell -Command "[System.Windows.Forms..."（暂未实现，回退打印）
///
/// 注意：headless / SSH session 下没 X11 / Wayland，notify-send 会静默失败 →
/// 命令本身退出码 0 也算成功。我们只能尽力。
Result<void> notify_desktop(const std::string& title, const std::string& body);

/// 检测当前平台是否能发桌面通知（命令是否存在）
bool desktop_notify_available();

}  // namespace bagu::util
