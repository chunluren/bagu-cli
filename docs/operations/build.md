# 构建说明

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 快速构建（Release）

```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

产物：`build/src/bagu`

---

## 2. CMake 选项

| 选项                       | 默认     | 说明                       |
|--------------------------|--------|--------------------------|
| `CMAKE_BUILD_TYPE`        | Release| Debug / Release / RelWithDebInfo |
| `BAGU_BUILD_TESTS`        | ON     | 构建单元测试                  |
| `BAGU_BUILD_BENCH`        | OFF    | 构建性能基准                  |
| `BAGU_USE_SYSTEM_DEPS`    | OFF    | 系统已装的依赖优先用             |
| `BAGU_TSAN`               | OFF    | 启用 ThreadSanitizer        |
| `BAGU_COVERAGE`           | OFF    | 启用覆盖率采集（gcov）          |
| `BAGU_STATIC`             | OFF    | 静态链接                      |

```bash
cmake -DBAGU_BUILD_TESTS=ON -DBAGU_BUILD_BENCH=ON ..
```

---

## 3. 构建变体

### 3.1 Debug
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
# 自动启用 -O0 -g + ASan
```

### 3.2 RelWithDebInfo（推荐线上）
```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
# -O2 + -g，崩溃可调
```

### 3.3 静态链接（便于分发）
```bash
cmake -DBAGU_STATIC=ON -DCMAKE_BUILD_TYPE=Release ..
# 产物可直接拷贝到任意机器（不依赖 .so）
```

---

## 4. 编译加速

### 4.1 ccache
```bash
sudo apt install ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
ccache --max-size=5G
```

### 4.2 Ninja > Make
```bash
cmake -G Ninja ..   # 比 make 快 2-3 倍
```

### 4.3 并行
```bash
ninja -j$(nproc)
make -j$(nproc)
```

### 4.4 共享 _deps 缓存
项目内多个 build dir 共享：
```cmake
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/third_party/_deps)
```

---

## 5. 跨平台

### 5.1 Linux
- 主要平台
- GCC 11+ / Clang 14+

### 5.2 macOS
```bash
brew install cmake ninja sqlite curl
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### 5.3 Windows
```powershell
# 用 vcpkg 或 MSYS2
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

> Windows 是 best effort，主要用 WSL 测试。

---

## 6. 交叉编译

### 6.1 Linux ARM64
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake ..
```

`cmake/aarch64-linux-gnu.cmake`：
```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
```

---

## 7. 安装

```bash
sudo cmake --install build --prefix /usr/local
# 或
sudo make install
```

产物：
- `/usr/local/bin/bagu`
- `/usr/local/share/bagu/`（如有资源文件）

卸载：
```bash
sudo rm /usr/local/bin/bagu
```

---

## 8. 打包

### 8.1 tar.gz
```bash
cmake --build build --target package
# 产生 build/bagu-0.1.0-Linux-x86_64.tar.gz
```

### 8.2 .deb（Debian/Ubuntu）
```cmake
# CMakeLists.txt
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Li Yan <liyan@example.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsqlite3-0, libcurl4")
include(CPack)
```

```bash
cd build
cpack
# 产生 bagu_0.1.0_amd64.deb
sudo dpkg -i bagu_0.1.0_amd64.deb
```

### 8.3 .rpm（RHEL/Fedora）
```cmake
set(CPACK_GENERATOR "RPM")
```

### 8.4 AppImage（通用 Linux）
```bash
linuxdeploy --appdir AppDir --executable build/src/bagu --output appimage
```

### 8.5 Homebrew formula
```ruby
class Bagu < Formula
  desc "八股文档智能学习助手"
  homepage "https://github.com/<user>/bagu-cli"
  url "https://github.com/<user>/bagu-cli/archive/v0.1.0.tar.gz"
  sha256 "..."
  depends_on "cmake" => :build
  depends_on "sqlite"
  depends_on "curl"

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end
end
```

---

## 9. 故障排查

### Q1. `Could NOT find SQLite3`
```bash
sudo apt install libsqlite3-dev
```

### Q2. `error: ‘format’ is not a member of ‘std’`
GCC 13+ 才完整支持 `std::format`。降级到 GCC 11 时项目用 fmtlib 替代：
```cmake
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13")
    FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt GIT_TAG 10.2.1)
    FetchContent_MakeAvailable(fmt)
endif()
```

### Q3. FetchContent 拉不下来
- 检查网络
- 用国内镜像
- 用 `git config --global url."https://gitclone.com/".insteadOf "https://"`

### Q4. ASan 报错（macOS）
macOS 上 ASan 默认不全，可能需要：
```bash
export MallocNanoZone=0
```

---

## 10. 相关文档

- [开发环境](../development/setup.md)
- [CI/CD](./ci-cd.md)
- [发布流程](./release.md)
