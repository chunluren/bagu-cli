# Runbook — 怎么跑 / 改 / 停 / 测

> 维护者快速查手册。一切操作都基于本地仓库 `/home/ly/workspaces/bagu-cli/`。

---

## 1. 怎么跑

### 1.1 端用户：装好就用

```bash
brew tap chunluren/bagu
brew install bagu-cli

bagu init
bagu import ~/bagu-docs/
bagu serve                       # http://127.0.0.1:8780
```

### 1.2 自己开发：从源码

#### 第一次构建（C++ + 前端 + 嵌入资源）

```bash
cd /home/ly/workspaces/bagu-cli

# 1) 前端
cd web
npm ci                            # 第一次拉依赖
npm run build                     # 产物 web/dist 会被 cmake 嵌入
cd ..

# 2) C++（首次会 FetchContent 拉依赖，约 1-3 分钟）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBAGU_BUILD_TESTS=ON
cmake --build build -j

# 3) 跑
./build/src/bagu --version        # bagu 1.2.x
./build/src/bagu serve --port 8780
```

#### 切换 Debug / Release

```bash
# Debug（默认；含调试符号 ~40MB）
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Release（用于发布，~3MB）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

#### 隔离的临时实例

```bash
# 不污染真实 ~/.bagu
export BAGU_HOME=/tmp/bagu-experiment
./build/src/bagu init
./build/src/bagu import ~/bagu-docs/
./build/src/bagu serve --port 19000
```

### 1.3 前端 dev mode（HMR 热更新）

```bash
# 1) 后端跑起来（提供 API）
./build/src/bagu serve --port 8780

# 2) 前端 vite dev server（另一个终端）
cd web
npm run dev                       # 监听 5173；vite.config.ts 已配 proxy: /api → :8780
# 浏览器开 http://localhost:5173 — 修改 .tsx 即时刷新
```

### 1.4 e2e（Playwright）

```bash
cd web
npm run e2e                       # 25 个测试用例，~6 秒
npm run e2e:ui                    # UI 模式调试

# CI 也跑这套，见 .github/workflows/ci.yml
```

---

## 2. 怎么改

### 2.1 文件位置速查

```
src/                              C++ 源码
├── main.cpp                      CLI 入口（subcommand 注册 + 路由）
├── cli/                          每个 subcommand 一个 .cpp（init/import/list/...）
├── core/sm2.cpp                  SuperMemo-2 算法（纯函数）
├── db/                           SQLite + DAO + 迁移
│   ├── database.cpp              连接 + 事务 + migrate
│   ├── migrations.cpp            schema v1-v4 SQL
│   └── *_dao.cpp                 topic / chapter / card / review / interview
├── http/                         cpp-httplib 路由
│   ├── http_server.cpp           主路由 + meta/topic/card/search/review/stats/export
│   ├── interview_routes.cpp      /api/interview/* SSE
│   └── embedded_assets.h         前端资源嵌入接口
├── llm/                          LLM 客户端（OpenAI / Ollama / Claude）
├── parser/markdown_parser.cpp    手写 markdown → Q&A 卡解析
├── service/                      业务逻辑（编排 DAO）
└── util/                         path / hash / http / notify / network

web/src/
├── App.tsx                       React Router 路由表
├── api/{client.ts,types.ts}      fetch wrapper + SSE + 类型镜像
├── components/                   AppLayout / ThemeToggle / MarkdownRenderer / Heatmap / ScoreButtons
├── hooks/{useApi.ts,useTheme.ts}
├── pages/                        Home / Topic / Card / Search / Review / Interview / Stats / ...
└── pwa.ts                        service worker 注册

include/bagu/                     公共头（version / error / result）
docs/                             架构 / ADR / 用户手册 / 运维（本文档在这）
tests/unit/                       GoogleTest 单元测试
web/e2e/                          Playwright 端到端
scripts/
├── gen_embedded_assets.cmake     web/dist → C++ unsigned char[] 数组
└── homebrew/bagu-cli.rb          Homebrew formula

.github/workflows/{ci,release}.yml
```

### 2.2 加一个新 CLI 子命令

参考最近添加的 `bagu pause`：

1. `src/cli/pause_cmd.{h,cpp}` — 实现
2. `src/CMakeLists.txt` — 加到 `BAGU_LIB_SOURCES`（条件编译别忘）
3. `src/main.cpp` — `app.add_subcommand(...)` + 路由分发

### 2.3 加一个新 HTTP 路由

参考 `register_export_routes`：

1. `src/http/http_server.h` — 加 `register_<name>_routes()` 声明
2. `src/http/http_server.cpp` — 实现 + 在 `register_routes()` 里调用
3. 用 `send_json(res, 200, json{...})` 或 `send_error(res, ...)` 统一格式

### 2.4 加一个新前端页面

1. `web/src/pages/Foo.tsx` — 实现
2. `web/src/App.tsx` — `<Route path="/foo" element={<Foo />} />`
3. `web/src/components/AppLayout.tsx` — 加导航项（如需）
4. `cd web && npm run build` → `cmake --build build -j` 嵌入到二进制

### 2.5 改 schema

1. `src/db/migrations.cpp` — 写新 `kMigrationXXXX` 字符串，加到 `register_all_migrations`
2. 升 `tests/unit/db/migrations_test.cpp` 的版本断言
3. 升 `web/e2e/home.spec.ts` 的 `schema_version` 断言（如果改了）
4. 旧 DB 升级路径要在 service 层兜底（参考 v3→v4 stable_key backfill）

### 2.6 改 LLM provider 默认

`src/llm/factory.cpp` — provider 名 → 默认 base_url + model 的映射。

### 2.7 改 SM-2 参数

`src/core/sm2.cpp` — 算法常量都在这（initial_interval / ease_floor / ease_delta_for_score 等）。

### 2.8 让前端在自己电脑上能装 PWA

需要 `https` 或 `localhost`。`http://192.168.x.y:8780` 浏览器拒装。
要解决：用 `caddy reverse-proxy localhost:8780` + 自签证书，或者 ssh tunnel 到 localhost。

---

## 3. 怎么停

### 3.1 前台运行的 `bagu serve`

直接 `Ctrl+C`（SIGINT），优雅关闭。

### 3.2 后台运行的 `bagu serve`

```bash
# 用 jobs / fg
jobs           # 看 [1]+ Running ./bagu serve
fg %1          # 切到前台再 Ctrl+C
# 或者直接
kill %1

# 用 PID
ps -ef | grep "bagu serve"
kill <PID>          # SIGTERM, 优雅
kill -9 <PID>       # SIGKILL, 暴力（避免）

# 按命令名
pkill -f "bagu serve --port 8780"
```

### 3.3 装在 systemd / launchd / cron 的

```bash
# systemd user timer
systemctl --user stop bagu-remind.timer
systemctl --user disable bagu-remind.timer

# launchd
launchctl unload ~/Library/LaunchAgents/com.bagu.remind.plist

# cron
crontab -e        # 注释或删那一行
```

### 3.4 卸载完全干净

```bash
brew uninstall bagu-cli
brew untap chunluren/bagu

# 数据
rm -rf ~/.bagu/

# 仓库
rm -rf /home/ly/workspaces/bagu-cli/build
```

---

## 4. 怎么测

### 4.1 C++ 单元测试

```bash
cd /home/ly/workspaces/bagu-cli/build
ctest                              # 跑全部，206 个用例 ~7s
ctest --output-on-failure          # 失败时打印 stdout/stderr
ctest -R Import                    # 名字含 Import 的（regex）
ctest -R "ConfigLoader|Stats"      # 多个 fixture
ctest -j 8                         # 并行（默认 1）
ctest -V                           # verbose
```

直接跑测试二进制（细控制 / 复用一次启动）：
```bash
./tests/bagu_tests --gtest_filter='ImportServiceTest.*'
./tests/bagu_tests --gtest_filter='-*Slow*'        # 排除
./tests/bagu_tests --gtest_list_tests              # 列全部
./tests/bagu_tests --gtest_repeat=10               # 反复跑找 flaky
```

### 4.2 Playwright e2e

```bash
cd web
npm run e2e                                        # 默认 chromium，~6s
npm run e2e:ui                                     # UI 调试模式

# 单个文件
npx playwright test e2e/theme.spec.ts
# 单个用例
npx playwright test -g "三态主题"

# 失败时看截图 / trace
ls test-results/
npx playwright show-trace test-results/<failed>/trace.zip

# 失败 HTML 报告
npx playwright show-report
```

环境注意：本机有 Clash 代理（`http_proxy=127.0.0.1:7897`）会拦 localhost。
playwright.config.ts 已配 `--no-proxy-server`；CI 没问题。

### 4.3 手动 smoke（HTTP）

```bash
# 起 server 在隔离 BAGU_HOME
BAGU_HOME=/tmp/bagu-smoke ./build/src/bagu init
BAGU_HOME=/tmp/bagu-smoke ./build/src/bagu serve --port 18999 &

# 关键端点
curl -s http://127.0.0.1:18999/api/health
curl -s http://127.0.0.1:18999/api/topics
curl -s http://127.0.0.1:18999/api/stats/overall
curl -s 'http://127.0.0.1:18999/api/search?q=MVCC'
curl -s http://127.0.0.1:18999/api/review/due-summary
curl -s http://127.0.0.1:18999/api/llm/profiles

# 导出
curl -s 'http://127.0.0.1:18999/api/export/anki?topic=mysql' | head
curl -s 'http://127.0.0.1:18999/api/export/csv?topic=mysql'  | head

# pause
curl -X POST http://127.0.0.1:18999/api/review/1/suspend \
     -H 'Content-Type: application/json' -d '{"suspended":true}'

# 收尾
pkill -f 'bagu serve --port 18999'
```

### 4.4 性能 / 内存

```bash
# strace 启动
strace -c ./build/src/bagu --version

# valgrind 内存（Debug 构建）
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j
valgrind --tool=memcheck --leak-check=full ./build-debug/src/bagu list

# 启动时间
time ./build/src/bagu --version

# 二进制体积
ls -lh build/src/bagu
# 嵌入资源数（应该 12）
strings build/src/bagu | grep -c manifest+json   # 1 = 含 manifest mime 字符串
```

### 4.5 schema / 数据库直查

```bash
sqlite3 ~/.bagu/bagu.db
.tables
.schema card
SELECT name, version FROM schema_version;
SELECT COUNT(*) FROM card;
SELECT topic_id, COUNT(*) FROM card GROUP BY topic_id;
SELECT card_id, suspended FROM review WHERE suspended = 1;
.quit

# 备份
sqlite3 ~/.bagu/bagu.db ".backup '/tmp/bagu-backup.db'"

# 重置
rm ~/.bagu/bagu.db
bagu import ~/bagu-docs/
```

### 4.6 调试日志

```bash
SPDLOG_LEVEL=debug bagu serve         # 全局 debug
SPDLOG_LEVEL=trace bagu interview --topic mysql   # 看 LLM 请求
bagu --verbose <command>               # CLI 等价
```

### 4.7 CI 触发 / 重跑

```bash
# 看最近 5 个 CI run
gh run list --limit 5

# 看具体一个 run
gh run view <run-id>
gh run view --job <job-id> --log-failed

# 重跑失败的 job
gh run rerun <run-id> --failed

# 取消正在跑的
gh run cancel <run-id>
```

---

## 5. 发版速查

```bash
# 1) bump version
$EDITOR include/bagu/version.h         # X.Y.Z 改

# 2) CHANGELOG: [Unreleased] → [X.Y.Z]
$EDITOR CHANGELOG.md
$EDITOR README.md                       # tests badge / version badge

# 3) commit + tag
git commit -am "release: vX.Y.Z — <slogan>"
git tag -a vX.Y.Z -m "vX.Y.Z — <slogan>"
git push origin main
git push origin vX.Y.Z                  # 触发 release.yml

# 4) 等 ~5 min release.yml 完成
gh run list --workflow=release.yml --limit 1

# 5) 算 SHA256 更新 Homebrew tap
mkdir /tmp/v && cd /tmp/v
gh release download vX.Y.Z -R chunluren/bagu-cli
shasum -a 256 *.tar.gz                  # 复制两个 SHA

# 6) 改 in-tree formula
$EDITOR scripts/homebrew/bagu-cli.rb    # version + 两个 sha256

# 7) 同步到 tap repo
git clone https://github.com/chunluren/homebrew-bagu.git /tmp/h
cp scripts/homebrew/bagu-cli.rb /tmp/h/Formula/
cd /tmp/h && git add . && git commit -m "bump bagu-cli to vX.Y.Z" && git push

# 8) 提交 in-tree formula 更新
cd /home/ly/workspaces/bagu-cli
git commit -am "chore(homebrew): in-tree formula bump to vX.Y.Z"
git push
```

---

## 6. 常见小坑

### Q: cmake 报 web/dist 缺文件
→ 改了前端但忘 build。`cd web && npm run build && cd .. && cmake --build build -j`。

### Q: `bagu serve` 起来了但前端 404
→ 没 reconfigure cmake。`cmake -B build` 一次让 GLOB_RECURSE 重新扫 dist。

### Q: e2e 全 ConnectionRefused
→ Clash 代理拦 localhost。已用 `--no-proxy-server` 修掉，仍有问题：
`NO_PROXY=localhost,127.0.0.1,::1 npm run e2e`。

### Q: `bagu pause 1` 报 FOREIGN KEY
→ card_id=1 不存在。先 `sqlite3 ~/.bagu/bagu.db "SELECT id FROM card LIMIT 5"` 查真实 ID。

### Q: 单测改 schema 后失败
→ `tests/unit/db/migrations_test.cpp` 和 `tests/unit/http/http_server_test.cpp` 里的
`schema_version == N` 断言要同步升。

### Q: Release.yml 卡 macOS x86 不出
→ Intel runner 排队。release.yml 已只保 macos-arm64 + linux-x86_64。

---

## 7. 一行总结

```bash
# 全套 dev cycle
cd web && npm run build && cd .. && cmake --build build -j && (cd build && ctest) && BAGU_HOME=/tmp/dev ./build/src/bagu serve
```

记不住就翻这文档。
