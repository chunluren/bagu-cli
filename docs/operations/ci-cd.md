# CI/CD 流程

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 总体流程

```
开发者 push branch
        ↓
   PR Created
        ↓
┌─── GitHub Actions ───┐
│  ✓ Lint              │
│  ✓ Build (gcc/clang) │
│  ✓ Unit tests        │
│  ✓ Integration tests │
│  ✓ e2e tests         │
│  ✓ Coverage report   │
│  ✓ Benchmark diff    │
└──────────────────────┘
        ↓
   Code Review
        ↓
   Merge to main
        ↓
┌─── Release Pipeline ──┐
│  ✓ Tag → Build artifacts │
│  ✓ GitHub Release        │
│  ✓ Update changelog      │
└──────────────────────┘
```

---

## 2. Workflow 文件

### 2.1 `.github/workflows/ci.yml`（PR + push 触发）

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  lint:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install clang-format
        run: sudo apt-get install -y clang-format
      - name: Check format
        run: |
          find src tests -name '*.cpp' -o -name '*.h' | \
            xargs clang-format --dry-run --Werror

  build-test:
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-22.04, macos-13]
        compiler: [gcc, clang]
        build_type: [Release, Debug]
        exclude:
          - os: macos-13
            compiler: gcc

    steps:
      - uses: actions/checkout@v4

      - name: Install deps (Ubuntu)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential cmake ninja-build \
            libsqlite3-dev libcurl4-openssl-dev

      - name: Install deps (macOS)
        if: runner.os == 'macOS'
        run: brew install cmake ninja sqlite curl

      - name: Setup ccache
        uses: hendrikmuhs/ccache-action@v1
        with:
          key: ${{ matrix.os }}-${{ matrix.compiler }}-${{ matrix.build_type }}

      - name: Configure
        run: |
          if [ "${{ matrix.compiler }}" = "clang" ]; then
            export CC=clang CXX=clang++
          fi
          cmake -G Ninja -B build \
            -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
            -DBAGU_BUILD_TESTS=ON \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

      - name: Build
        run: cmake --build build --parallel

      - name: Run unit tests
        run: |
          cd build
          ctest --output-on-failure --label-regex unit

      - name: Run integration tests
        run: |
          cd build
          ctest --output-on-failure --label-regex integration

      - name: Run e2e tests
        if: runner.os == 'Linux'
        run: bash tests/e2e/run_all.sh

  coverage:
    runs-on: ubuntu-22.04
    needs: build-test
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: |
          sudo apt-get install -y \
            build-essential cmake ninja-build \
            libsqlite3-dev libcurl4-openssl-dev gcovr

      - name: Build with coverage
        run: |
          cmake -G Ninja -B build \
            -DCMAKE_BUILD_TYPE=Debug \
            -DBAGU_BUILD_TESTS=ON \
            -DBAGU_COVERAGE=ON
          cmake --build build

      - name: Run tests
        run: cd build && ctest

      - name: Generate coverage
        run: |
          gcovr --root . --xml -o coverage.xml \
            --exclude 'build/_deps/*' \
            --exclude 'tests/*'

      - name: Upload to Codecov
        uses: codecov/codecov-action@v4
        with:
          files: coverage.xml
          fail_ci_if_error: true

  bench:
    runs-on: ubuntu-22.04
    if: github.event_name == 'pull_request'
    steps:
      - uses: actions/checkout@v4
      - name: Build benchmarks
        run: |
          cmake -G Ninja -B build \
            -DCMAKE_BUILD_TYPE=Release \
            -DBAGU_BUILD_BENCH=ON
          cmake --build build --target bagu_bench

      - name: Run benchmarks
        run: |
          ./build/tests/bench/bagu_bench \
            --benchmark_format=json \
            --benchmark_out=bench.json

      - name: Compare with main
        uses: benchmark-action/github-action-benchmark@v1
        with:
          tool: 'googlecpp'
          output-file-path: bench.json
          alert-threshold: '110%'
          fail-on-alert: true
```

### 2.2 `.github/workflows/release.yml`（tag 触发）

```yaml
name: Release

on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  build-release:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        include:
          - os: ubuntu-22.04
            artifact: bagu-linux-x86_64
          - os: macos-13
            artifact: bagu-macos-x86_64
          - os: macos-14
            artifact: bagu-macos-arm64

    steps:
      - uses: actions/checkout@v4

      - name: Install deps
        run: |
          if [ "${{ runner.os }}" = "Linux" ]; then
            sudo apt-get install -y libsqlite3-dev libcurl4-openssl-dev cmake ninja-build
          else
            brew install cmake ninja sqlite curl
          fi

      - name: Build (static)
        run: |
          cmake -G Ninja -B build \
            -DCMAKE_BUILD_TYPE=Release \
            -DBAGU_STATIC=ON
          cmake --build build

      - name: Package
        run: |
          cd build
          cpack -G TGZ
          mv bagu-*.tar.gz ../${{ matrix.artifact }}.tar.gz

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: ${{ matrix.artifact }}
          path: ${{ matrix.artifact }}.tar.gz

  release:
    runs-on: ubuntu-22.04
    needs: build-release
    steps:
      - uses: actions/checkout@v4
      - name: Download all artifacts
        uses: actions/download-artifact@v4
        with:
          path: artifacts/

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v1
        with:
          files: artifacts/*/*.tar.gz
          generate_release_notes: true
          body_path: CHANGELOG.md
```

---

## 3. 分支保护规则

GitHub Settings → Branches → Add rule for `main`：

- ✓ Require a pull request before merging
  - ✓ Require approvals: 1
  - ✓ Dismiss stale approvals on new commits
- ✓ Require status checks to pass before merging
  - 必须通过：lint / build-test / coverage
- ✓ Require linear history（禁止 merge commit）
- ✓ Include administrators
- ✗ Allow force pushes
- ✗ Allow deletions

---

## 4. 本地预 CI 检查

提交前自查：
```bash
# scripts/precheck.sh
#!/bin/bash
set -e

echo "1. clang-format..."
find src tests -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror

echo "2. Build Release..."
cmake -G Ninja -B build-pre -DCMAKE_BUILD_TYPE=Release
cmake --build build-pre

echo "3. Run tests..."
cd build-pre && ctest --output-on-failure

echo "All checks passed!"
```

---

## 5. 监控指标

CI 自身的健康度：
- **PR 平均 CI 时间**：目标 < 5 分钟
- **CI 失败率**：目标 < 5%
- **flaky tests**：发现立即修

---

## 6. CI 优化技巧

### 6.1 cache
```yaml
- uses: actions/cache@v4
  with:
    path: |
      build/_deps
      ~/.cache/ccache
    key: deps-${{ hashFiles('CMakeLists.txt') }}
```

### 6.2 并行 job
矩阵中的 OS / 编译器并行跑。

### 6.3 path filters
仅 docs 修改不跑全 CI：
```yaml
on:
  pull_request:
    paths-ignore:
      - 'docs/**'
      - '*.md'
```

### 6.4 Concurrency
取消旧 PR 的 CI：
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

---

## 7. 相关文档

- [构建说明](./build.md)
- [发布流程](./release.md)
- [测试策略](../testing/strategy.md)
