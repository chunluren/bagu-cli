# C++ 编码规范

> **状态**：Approved
> **更新**：2026-04-29
> 基于 Google C++ Style Guide + 项目调整

---

## 1. 总原则

1. **简单 > 聪明** — 能用 5 行的别用 50 行
2. **明确 > 隐式** — 类型显式、错误显式
3. **常量 > 变量** — `const` / `constexpr` 默认
4. **栈 > 堆** — 能栈分配不堆
5. **RAII** — 资源用对象生命周期管理
6. **无异常**（项目级约定）— 用 `Result<T>` 显式传播错误

---

## 2. 文件组织

### 2.1 后缀
- 头文件：`.h`
- 实现：`.cpp`
- 模板/inline 定义：`.h` 或 `.hpp`

### 2.2 头文件保护
统一用 `#pragma once`：
```cpp
#pragma once
```

### 2.3 包含顺序

```cpp
// 1. 对应的头文件（cpp 第一行）
#include "my_module.h"

// 2. 标准库
#include <string>
#include <vector>

// 3. 第三方库
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

// 4. 本项目其他模块
#include "bagu/db/database.h"
#include "bagu/util/result.h"
```

每组内部按字母排序，组之间空行分隔。

### 2.4 头文件最小化
- 头文件用前向声明优先
- 只 include 必要的（不要"以防万一"）
- 实现细节放 cpp，不暴露在 h

---

## 3. 命名

### 3.1 通用规则
- 命名要**描述意图**，避免缩写
- 名字越大作用域越长，越要清晰

### 3.2 具体约定

| 类型              | 风格              | 示例                          |
|---------------|-----------------|-----------------------------|
| 文件             | snake_case      | `card_dao.cpp`              |
| 类 / 结构体        | PascalCase      | `CardDao`, `ReviewState`    |
| 枚举类型          | PascalCase      | `enum class CardType { ... }` |
| 枚举值           | kPascalCase     | `CardType::kQa`             |
| 函数 / 方法        | snake_case      | `get_due_cards()`           |
| 局部变量          | snake_case      | `int card_count;`           |
| 成员变量          | snake_case_     | `int count_;`               |
| 全局常量          | kPascalCase     | `constexpr int kMaxCards = 100;` |
| 宏              | UPPER_CASE      | `BAGU_LOG_LEVEL`            |
| 命名空间          | snake_case      | `namespace bagu::db`        |
| 模板参数          | PascalCase      | `template <typename T>`     |

### 3.3 例外
- 标准库风格的简单容器可用小写：`my_string` 即可
- 缩写：HTTP、URL、ID 等保持大写

---

## 4. 命名空间

### 4.1 顶层
所有代码在 `bagu` 命名空间下：
```cpp
namespace bagu {
    // ...
}
```

### 4.2 子模块
```cpp
namespace bagu::db {
    class Database;
}

namespace bagu::core {
    class SM2Algorithm;
}
```

### 4.3 禁止
- ❌ `using namespace std;`（在 .cpp 中也不要）
- ✅ `using std::string;` 局部使用 OK

---

## 5. 代码风格

### 5.1 缩进
- **4 空格**，不用 tab

### 5.2 行宽
- **100 字符**

### 5.3 大括号
**永远用大括号**，包括单行 if：
```cpp
// ✓
if (x > 0) {
    do_something();
}

// ✗
if (x > 0)
    do_something();   // 易出 bug
```

### 5.4 大括号位置
**K&R 风格**：
```cpp
void func() {
    if (cond) {
        // ...
    } else {
        // ...
    }
}
```

### 5.5 空格

```cpp
// 二元运算符两边
int sum = a + b;

// 函数调用括号紧贴
func(a, b, c);

// 控制语句关键字后
if (cond) { ... }
while (cond) { ... }
for (int i = 0; i < n; ++i) { ... }

// 指针/引用靠近类型
int* p;
int& r;
```

### 5.6 空行
- 函数之间一空行
- 逻辑段之间一空行
- 文件末尾一个空行

---

## 6. 类型

### 6.1 优先用现代类型

```cpp
// ✓
auto x = compute();
const std::string& name = obj.name();

// ✗ 啰嗦
std::string name = obj.name();   // 可能拷贝
```

### 6.2 类型别名
用 `using`，不用 `typedef`：
```cpp
// ✓
using CardId = int64_t;
using CardList = std::vector<Card>;

// ✗
typedef int64_t CardId;
```

### 6.3 整数类型
- 用 `int32_t` / `int64_t` 等具体类型，不用 `long` / `int`
- 唯一例外：循环变量可用 `int` / `size_t`

### 6.4 const / constexpr
**默认 const**，能 constexpr 就 constexpr：
```cpp
constexpr int kMaxCards = 100;          // 编译期常量
const auto& cards = get_cards();        // 不可变引用
```

---

## 7. 函数

### 7.1 函数大小
- 一个函数尽量 < 50 行
- 复杂逻辑拆小函数

### 7.2 参数

```cpp
// ✓ 输入用 const ref（非 POD）
void print(const std::string& s);
void print(int x);                    // POD 直接传值

// ✓ 输出用引用（不要用指针）
void compute(int input, int& out);

// ✓ 可选输出用 std::optional
std::optional<Card> find_by_id(int id);

// ✗ 输出参数尽量避免，优先返回值
```

### 7.3 返回值
- 优先返回值（NRVO 优化拷贝）
- 大对象用 `std::move`

```cpp
std::vector<Card> get_cards() {
    std::vector<Card> result;
    // ...
    return result;   // NRVO，不会拷贝
}
```

### 7.4 noexcept
不抛异常的函数标 `noexcept`：
```cpp
size_t size() const noexcept { return size_; }
```

---

## 8. 类设计

### 8.1 默认成员函数

```cpp
class Foo {
public:
    Foo() = default;
    ~Foo() = default;

    Foo(const Foo&) = delete;          // 默认禁拷贝
    Foo& operator=(const Foo&) = delete;

    Foo(Foo&&) = default;              // 允许移动
    Foo& operator=(Foo&&) = default;
};
```

### 8.2 成员顺序
```cpp
class Foo {
public:
    // 1. 类型
    using IdType = int64_t;
    enum class Kind { ... };

    // 2. 静态常量
    static constexpr int kMax = 100;

    // 3. 构造 / 析构
    Foo();
    ~Foo();

    // 4. 公开方法
    void do_something();

private:
    // 5. 私有方法
    void helper();

    // 6. 数据成员
    int count_;
};
```

### 8.3 单一职责
一个类做一件事。功能多了拆开。

---

## 9. 错误处理

### 9.1 不用异常
项目级约定：**禁用 C++ 异常**。

### 9.2 用 Result<T>

```cpp
Result<Card> CardDao::find_by_id(int64_t id) {
    auto stmt = db_->prepare("SELECT ... FROM card WHERE id = ?");
    stmt.bind(1, id);

    if (!stmt.step()) {
        return Result<Card>::err({E::kCardNotFound, "card not found"});
    }
    return Result<Card>::ok(parse_card(stmt));
}

// 调用方
auto r = card_dao.find_by_id(42);
if (r.is_err()) {
    spdlog::error("查找失败: {}", r.error().message);
    return;
}
const Card& card = r.value();
```

### 9.3 资源用 RAII
```cpp
// ✓ RAII
class FileGuard {
    FILE* f_;
public:
    explicit FileGuard(const char* path) : f_(fopen(path, "r")) {}
    ~FileGuard() { if (f_) fclose(f_); }
};

// ✗ 手动管理
FILE* f = fopen(path, "r");
// ...
fclose(f);   // 容易漏
```

---

## 10. 内存

### 10.1 智能指针
- 独占所有权 → `std::unique_ptr`
- 共享所有权 → `std::shared_ptr`
- 弱引用 → `std::weak_ptr`
- **裸指针只用于"不拥有"的引用**

### 10.2 创建
```cpp
auto p = std::make_unique<Foo>(args);    // ✓
auto s = std::make_shared<Foo>(args);    // ✓

// ✗ 别 new
Foo* p = new Foo();
```

### 10.3 容器
- 默认 `std::vector`
- 频繁查 `std::unordered_map`
- 有序 `std::map`
- 队列 `std::deque`
- 栈 `std::stack`

---

## 11. 注释

### 11.1 doxygen
公开 API 必须有：
```cpp
/// @brief 获取今日待复习的卡片
///
/// @param topic 主题名（空表示所有主题）
/// @param limit 最大返回数
/// @return 卡片列表，按优先级排序
std::vector<Card> get_due_cards(
    const std::optional<std::string>& topic,
    int limit);
```

### 11.2 实现注释
解释 **为什么**，不解释 **是什么**：
```cpp
// ✓ 解释为什么
// SQLite 的 WAL 模式下并发读性能更好
db_.execute("PRAGMA journal_mode = WAL");

// ✗ 重复代码
db_.execute("PRAGMA journal_mode = WAL");   // 设置 WAL 模式
```

### 11.3 TODO
```cpp
// TODO(liyan, 2026-05): 优化为批量查询
```

---

## 12. 现代 C++ 推荐

### 12.1 范围 for
```cpp
// ✓
for (const auto& card : cards) { ... }

// ✗
for (size_t i = 0; i < cards.size(); ++i) { ... }
```

### 12.2 结构化绑定
```cpp
for (const auto& [key, value] : map) { ... }
auto [a, b] = std::make_pair(1, 2);
```

### 12.3 字符串字面量
```cpp
using namespace std::literals;
auto s = "hello"sv;     // string_view
auto t = "world"s;      // string
```

### 12.4 std::optional
```cpp
std::optional<int> find(const std::string& key);

if (auto v = find("foo"); v.has_value()) {
    use(*v);
}
```

### 12.5 std::variant
```cpp
std::variant<int, std::string> value;
std::visit([](auto&& v) { use(v); }, value);
```

---

## 13. 自动化检查

### 13.1 clang-format
项目根目录 `.clang-format`：
```yaml
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
DerivePointerAlignment: false
PointerAlignment: Left
```

提交前自动 format：
```bash
clang-format -i src/**/*.{cpp,h}
```

### 13.2 clang-tidy
```bash
clang-tidy src/**/*.cpp -- -std=c++20
```

### 13.3 编译警告
```cmake
add_compile_options(-Wall -Wextra -Wpedantic -Werror)
```

---

## 14. Code Review 检查清单

代码 review 必看：
- [ ] 命名清晰、符合规范
- [ ] 无 `using namespace std;`
- [ ] 错误处理用 Result，不抛异常
- [ ] 资源用 RAII
- [ ] 没有魔法数字（用 constexpr 命名常量）
- [ ] 公开 API 有 doxygen
- [ ] 单元测试覆盖
- [ ] 编译无警告
- [ ] 通过 clang-format

---

## 15. 相关文档

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [架构总览](../architecture/overview.md)
