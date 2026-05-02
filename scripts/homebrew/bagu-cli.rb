# Homebrew Formula for bagu-cli
#
# 安装：
#   brew tap chunluren/bagu
#   brew install bagu-cli
#
# 升级 formula 时需要更新两处：
#   - version
#   - 每个 url 对应的 sha256（用 `curl -sL <url> | shasum -a 256` 计算）
#
# 本地测试：
#   brew install --build-from-source ./scripts/homebrew/bagu-cli.rb
class BaguCli < Formula
  desc "八股文档智能学习助手 - Interview prep CLI tool"
  homepage "https://github.com/chunluren/bagu-cli"
  version "0.4.0"
  license "MIT"

  # 直接发预编译二进制（已嵌入 Web UI），避免用户装 Node + cmake 全套构建链
  on_macos do
    on_arm do
      url "https://github.com/chunluren/bagu-cli/releases/download/v0.4.0/bagu-v0.4.0-macos-arm64.tar.gz"
      sha256 "33b9c66c7dbc8581673b04ad2fe8a5c7e44471bc48b7eaa9be2d4540486f1709"
    end
    # Intel Mac：暂未发预编译；从源码构建（见 head 块）
  end

  on_linux do
    on_intel do
      url "https://github.com/chunluren/bagu-cli/releases/download/v0.4.0/bagu-v0.4.0-linux-x86_64.tar.gz"
      sha256 "c5c10bf2d8e2dc9f9206a867efa6103839f968c0c589f42ccf3f94b02951ddba"
    end
    # Linux ARM：暂未发预编译
  end

  # 没有匹配的预编译时回退到源码（要求 Node 20 + cmake + libcurl + openssl + sqlite）
  head "https://github.com/chunluren/bagu-cli.git", branch: "main"

  # 预编译包是 statically/dynamically 链接系统库的 ELF/Mach-O；运行时仍需这些库
  depends_on "openssl@3"
  depends_on "sqlite"
  uses_from_macos "curl"

  def install
    if build.head?
      # 源码构建：需先用 Node 构建前端，再 cmake C++
      ENV.prepend_path "PATH", Formula["node"].opt_bin if Formula["node"].any_version_installed?
      odie "Node 20+ required for HEAD source build" unless which("npm")

      cd "web" do
        system "npm", "ci"
        system "npm", "run", "build"
      end

      args = std_cmake_args + %w[
        -GNinja
        -DCMAKE_BUILD_TYPE=Release
        -DBAGU_BUILD_TESTS=OFF
      ]
      system "cmake", "-S", ".", "-B", "build", *args
      system "cmake", "--build", "build", "--parallel"
      bin.install "build/src/bagu"
      pkgshare.install "README.md", "LICENSE", "CHANGELOG.md"
    else
      # 预编译 tarball 已含 bagu + README + LICENSE + CHANGELOG
      bin.install "bagu"
      pkgshare.install "README.md", "LICENSE", "CHANGELOG.md"
    end
  end

  def caveats
    <<~EOS
      bagu-cli 数据存在 ~/.bagu/（可通过 BAGU_HOME 环境变量覆盖）。

      第一次使用：
        bagu init
        bagu import <path-to-markdown>
        bagu serve              # 浏览器打开 http://127.0.0.1:8780

      用户手册：
        #{opt_pkgshare}/docs/user-guide/
        或 https://github.com/chunluren/bagu-cli/tree/main/docs/user-guide
    EOS
  end

  test do
    # 1. 基本可执行性
    assert_match version.to_s, shell_output("#{bin}/bagu --version")

    # 2. init 创建数据目录
    ENV["BAGU_HOME"] = testpath/".bagu"
    system bin/"bagu", "init"
    assert_predicate testpath/".bagu/config.toml", :exist?

    # 3. import 一份最小 markdown
    (testpath/"sample.md").write <<~MD
      # Sample
      ## 第 1 章 Test
      **Q1. What is X?**
      Y is the answer.
    MD
    output = shell_output("#{bin}/bagu import #{testpath}/sample.md")
    assert_match "Total cards: 1", output

    # 4. list 看得到 topic
    assert_match "sample", shell_output("#{bin}/bagu list")

    # 5. HTTP server 启得来 + /api/health 200
    fork do
      exec bin/"bagu", "serve", "--port", "18821", "--bind", "127.0.0.1"
    end
    sleep 1
    begin
      assert_match "\"ok\":true", shell_output("curl -sf http://127.0.0.1:18821/api/health")
    ensure
      system "pkill", "-f", "bagu serve --port 18821"
    end
  end
end
