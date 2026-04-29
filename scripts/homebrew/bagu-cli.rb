# Homebrew Formula for bagu-cli
#
# 安装：
#   brew tap chunluren/bagu
#   brew install bagu-cli
#
# 或本地测试：
#   brew install --build-from-source ./scripts/homebrew/bagu-cli.rb
#
# 提交到官方 homebrew-core 后无需 tap 直接 brew install bagu-cli。

class BaguCli < Formula
  desc "八股文档智能学习助手 - Interview prep CLI tool"
  homepage "https://github.com/chunluren/bagu-cli"
  url "https://github.com/chunluren/bagu-cli/archive/refs/tags/v0.2.0.tar.gz"
  # 注意：发布时需要替换为实际 release 的 tarball SHA256
  # 计算方式：curl -sL <url> | shasum -a 256
  sha256 "TODO_REPLACE_WITH_ACTUAL_SHA256"
  license "MIT"
  head "https://github.com/chunluren/bagu-cli.git", branch: "main"

  depends_on "cmake" => :build
  depends_on "ninja" => :build
  depends_on "pkg-config" => :build
  depends_on "curl"
  depends_on "openssl@3"
  depends_on "sqlite"

  def install
    args = std_cmake_args + %W[
      -GNinja
      -DCMAKE_BUILD_TYPE=Release
      -DBAGU_BUILD_TESTS=OFF
    ]
    system "cmake", "-S", ".", "-B", "build", *args
    system "cmake", "--build", "build", "--parallel"
    bin.install "build/src/bagu"

    # 文档
    pkgshare.install "README.md", "LICENSE", "CHANGELOG.md"
    (pkgshare/"docs").install Dir["docs/*"]
  end

  test do
    # 基本可执行性
    assert_match "bagu", shell_output("#{bin}/bagu --version")

    # 初始化 + 简单导入流程
    ENV["BAGU_HOME"] = testpath/".bagu"
    system bin/"bagu", "init"
    assert_predicate testpath/".bagu/config.toml", :exist?
    assert_predicate testpath/".bagu/bagu.db", :exist?

    # 导入一个最简 markdown
    (testpath/"sample.md").write <<~MD
      # Sample
      ## 第 1 章 Test
      **Q1. What is X?** Y is the answer.
    MD
    output = shell_output("#{bin}/bagu import #{testpath}/sample.md")
    assert_match "1 cards", output

    # list 应能看到这个 topic
    list_output = shell_output("#{bin}/bagu list")
    assert_match "sample", list_output
  end
end
