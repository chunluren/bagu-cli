# ADR-0003: 用 CMake FetchContent 管理依赖

- **状态**: Accepted
- **日期**: 2026-04-29
- **作者**: Li Yan

## 背景

bagu-cli 依赖多个 C++ 库：CLI11、spdlog、nlohmann/json、toml++、FTXUI、cmark 等。

需要一种依赖管理方案，目标：
- 用户编译时无需先装任何包管理器
- 跨平台（Linux/Mac/Windows）
- 与 CMake 无缝集成
- 版本可控

## 决策

**使用 CMake `FetchContent` 拉取所有非系统依赖。**

只有 SQLite3 和 libcurl 用 `find_package`（这两个一般系统自带）。

## 备选方案

| 方案           | 优点                            | 缺点                              | 不选理由                          |
|--------------|--------------------------------|---------------------------------|--------------------------------|
| **FetchContent** | CMake 内置、声明式、零外部工具          | 首次编译要联网下载                    | —                              |
| vcpkg        | 包多、官方支持                       | 用户必须先装、CI 配置复杂                | 引入额外学习成本                     |
| Conan        | 多平台预编译包                       | 同 vcpkg                          | 同上                             |
| git submodule | 无需联网（除 clone 时）               | 版本管理麻烦、嵌套提交                  | FetchContent 更现代                |
| 系统包         | 最快                              | 各发行版差异大、版本不统一                 | 用户体验差                         |
| 手动下载       | 完全可控                           | 维护成本极高                          | 太低效                            |

## 后果

### 正面

- **零依赖** ——`git clone && cd build && cmake .. && make` 即可
- **版本锁定**——通过 `GIT_TAG` 固定
- **跨平台**——CMake 自身处理
- **CI 简单**——不需要安装任何包管理器

### 负面

- **首次构建慢**——要 git clone 几个 repo（约 1-3 分钟）
- **占用磁盘**——每个项目独立缓存
- **离线不能用**——可缓存 `_deps/` 目录绕过

### 中性

- 升级依赖只需改 `GIT_TAG`
- 可以同时支持 `find_package` 作为 fallback

## 实施

CMakeLists.txt 模板：
```cmake
include(FetchContent)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE       # 只拉最新 commit，节省时间
)

FetchContent_MakeAvailable(CLI11)
```

加速建议：
```cmake
# 用 ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# 共享 _deps 缓存（避免每个 build dir 都重新拉）
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/third_party/_deps)
```

## 相关链接

- [CMake FetchContent 文档](https://cmake.org/cmake/help/latest/module/FetchContent.html)
