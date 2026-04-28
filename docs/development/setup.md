# 开发环境搭建

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 前置要求

### 1.1 编译工具

| 工具      | 最低版本    | 推荐版本    |
|---------|---------|---------|
| GCC     | 11      | 13      |
| Clang   | 14      | 17      |
| CMake   | 3.20    | 3.27+   |
| Ninja   | -       | 1.11+   |
| Git     | 2.30    | latest  |
| Make    | -       | -       |

**或** macOS：Xcode Command Line Tools + brew 装 cmake / ninja。

### 1.2 系统库

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install -y \
    build-essential cmake ninja-build git \
    libsqlite3-dev libcurl4-openssl-dev pkg-config \
    libssl-dev

# macOS
brew install cmake ninja sqlite curl pkg-config

# Arch
sudo pacman -S base-devel cmake ninja git sqlite curl
```

### 1.3 可选工具

```bash
# 静态分析
sudo apt install -y clang-tidy cppcheck

# 代码格式化
sudo apt install -y clang-format

# Sanitizer 已包含在 GCC/Clang

# 编译加速
sudo apt install -y ccache
```

---

## 2. 克隆项目

```bash
git clone https://github.com/<user>/bagu-cli.git
cd bagu-cli
```

---

## 3. 构建

### 3.1 第一次构建

```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

> 首次构建会下载第三方依赖（CLI11/spdlog/json/toml++ 等），约 1-3 分钟。

### 3.2 Debug 构建

```bash
mkdir build-debug && cd build-debug
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

Debug 构建会自动启用 AddressSanitizer。

### 3.3 全清重建

```bash
rm -rf build build-debug
```

---

## 4. 运行

```bash
./src/bagu --version
./src/bagu --help

# 初始化数据
./src/bagu init

# 导入八股
./src/bagu import /path/to/bagu-docs/

# 测试
./src/bagu list
```

---

## 5. 跑测试

```bash
cd build
ninja test
# 或
ctest --output-on-failure
```

带覆盖率：
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBAGU_COVERAGE=ON ..
ninja
ctest
gcovr --root .. --html-details -o coverage.html
```

---

## 6. 编辑器配置

### 6.1 VS Code

推荐扩展：
- C/C++ (Microsoft)
- CMake Tools
- clangd（替代 IntelliSense，更快更准）
- Better C++ Syntax

`.vscode/settings.json`：
```json
{
    "C_Cpp.intelliSenseEngine": "disabled",
    "clangd.path": "clangd",
    "cmake.configureOnOpen": true,
    "cmake.generator": "Ninja",
    "files.associations": {
        "*.h": "cpp"
    }
}
```

### 6.2 CLion

直接打开项目根目录，CLion 会自动识别 CMakeLists.txt。

### 6.3 vim/neovim

用 clangd LSP（coc-clangd 或 native LSP）。

需要 `compile_commands.json`，CMakeLists.txt 已开启自动生成。

---

## 7. 依赖国内加速

如果 GitHub 拉不动：

```cmake
# 在 CMakeLists.txt 中替换 URL
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://gitee.com/mirrors/CLI11.git   # 镜像
    GIT_TAG v2.4.2
)
```

或设置 git 代理：
```bash
git config --global url."https://ghproxy.com/https://github.com/".insteadOf "https://github.com/"
```

---

## 8. 常见问题

### Q1. `Could not find SQLite3`

```bash
sudo apt install libsqlite3-dev
```

### Q2. FetchContent 下载失败

- 检查网络
- 配置代理
- 用 `git protocol://` 替换 `https://`

### Q3. ASan 报 "container-overflow" 误报

```bash
export ASAN_OPTIONS=detect_container_overflow=0
```

### Q4. 编译慢

```bash
# 用 ninja 而不是 make
cmake -G Ninja ..

# 用 ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
```

### Q5. 找不到 `libcurl`

```bash
sudo apt install libcurl4-openssl-dev
```

### Q6. 调试时 print 不输出

确保 `--verbose` 开启，或修改 `spdlog::set_level(spdlog::level::debug)`。

---

## 9. Pre-commit 钩子

推荐安装 pre-commit（自动跑 format + lint）：

```bash
pip install pre-commit
pre-commit install
```

`.pre-commit-config.yaml`（在仓库根）：
```yaml
repos:
  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v18.1.8
    hooks:
      - id: clang-format
  - repo: local
    hooks:
      - id: cmake-check
        name: CMake configure check
        entry: bash -c 'cmake -B /tmp/check-build .'
        language: system
        files: CMakeLists.txt
```

---

## 10. 相关文档

- [编码规范](./coding-standards.md)
- [Git 工作流](./git-workflow.md)
- [构建说明](../operations/build.md)
- [调试指南](./debugging.md)
