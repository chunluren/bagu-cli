# 桌面通知（`bagu remind`）

> v1.2+ · 每天定时弹一条桌面通知，提醒今天还有多少卡片要复习。

## 一次性测试

```bash
bagu remind --dry-run        # 看通知会写什么，不真发
bagu remind                   # 真的弹通知（需 X11/Wayland/macOS GUI session）
bagu remind --topic mysql     # 只算 mysql 主题
bagu remind --threshold 5     # 待复习 <5 张时不发
```

## 平台依赖

| 平台 | 命令 | 安装 |
|---|---|---|
| Linux | `notify-send` | `sudo apt install libnotify-bin` |
| macOS | `osascript` | 系统自带 |
| Windows | （暂未实现） | — |

`headless` / SSH session 下没桌面，命令本身可能成功但不显示通知 — 这是正常的，bagu 也无法识别。

## 接 cron（Linux/macOS）

每天上午 9 点弹一次：

```bash
crontab -e
# 添加：
0 9 * * * DISPLAY=:0 /usr/local/bin/bagu remind --quiet
```

注意：
- `DISPLAY=:0`：cron 默认无 DISPLAY，X11 不知道往哪个屏幕弹；macOS 不需要
- `--quiet`：不写日志（避免 cron 邮件骚扰）
- 完整路径：cron 的 PATH 通常很短

## 接 systemd timer（Linux 推荐）

更可控、更现代。

`~/.config/systemd/user/bagu-remind.service`：
```ini
[Unit]
Description=bagu daily review reminder

[Service]
Type=oneshot
ExecStart=/usr/local/bin/bagu remind --quiet
# 用户 graphical session 已起的环境
PassEnvironment=DBUS_SESSION_BUS_ADDRESS XDG_RUNTIME_DIR DISPLAY WAYLAND_DISPLAY
```

`~/.config/systemd/user/bagu-remind.timer`：
```ini
[Unit]
Description=Run bagu remind daily

[Timer]
OnCalendar=*-*-* 09:00:00
Persistent=true        # 错过的也补发（休眠醒来）

[Install]
WantedBy=timers.target
```

启用：
```bash
systemctl --user daemon-reload
systemctl --user enable --now bagu-remind.timer
systemctl --user list-timers | grep bagu
```

测试：
```bash
systemctl --user start bagu-remind.service
journalctl --user -u bagu-remind -n 20
```

## 接 launchd（macOS）

`~/Library/LaunchAgents/com.bagu.remind.plist`：
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
  <string>com.bagu.remind</string>
  <key>ProgramArguments</key>
  <array>
    <string>/usr/local/bin/bagu</string>
    <string>remind</string>
    <string>--quiet</string>
  </array>
  <key>StartCalendarInterval</key>
  <dict>
    <key>Hour</key><integer>9</integer>
    <key>Minute</key><integer>0</integer>
  </dict>
  <key>RunAtLoad</key>
  <false/>
</dict>
</plist>
```

加载：
```bash
launchctl load ~/Library/LaunchAgents/com.bagu.remind.plist
launchctl start com.bagu.remind   # 手动触发一次
```

## 退出码

| 退出码 | 含义 |
|---|---|
| 0 | 成功（通知已发 / 阈值未达不发） |
| 7 (`E::kFileNotFound`) | 通知命令不存在（缺 notify-send 等） |
| 4 (`E::kDbNotFound`) | 没 init / import |
| 5 (`E::kInternal`) | 通知命令本身失败 |

## 常见问题

### Q: cron 跑了但没看到通知
A: 99% 是 DISPLAY/DBUS 环境问题。Linux 推荐用 systemd user timer（自动继承 graphical session 的 env）。

### Q: 通知发出去了，但点击没反应
A: 当前实现没绑动作，纯展示。点击行为下版本可加（注入"打开 http://localhost:8780/review"）。

### Q: 想换通知文案怎么办
A: 当前是 hardcoded 中文。Roadmap 里有 i18n 任务；短期 fork 改 `src/cli/remind_cmd.cpp` 的 `title_ss` / `body_ss`。

### Q: 想配多个时间点（早晚各一次）
A: cron 写两行 / systemd timer 多条 OnCalendar / launchd 数组多条。
