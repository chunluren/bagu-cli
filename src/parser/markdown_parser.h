#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bagu/result.h"

namespace bagu::parser {

/// 解析后的章节
struct ParsedChapter {
    int chapter_no = 0;        // 章号（## 第 N 章 → N；### N.M → N*100+M）
    int level = 2;             // 2=## / 3=###
    std::string name;          // 章节标题（去掉编号前缀）
    int parent_chapter_no = 0; // 顶级章节的 chapter_no（### 时设置）
    int source_line = 0;
};

/// 解析后的卡片
struct ParsedCard {
    std::string question;
    std::string answer;
    int chapter_no = 0;        // 所属顶级章节（##）
    std::string card_type;     // "qa"（题库 Q/A）/ "section"（小节）
    int source_line = 0;
};

/// 整篇 markdown 解析结果
struct ParsedDocument {
    std::string topic_name;          // 从文件名推（不含 .md，转小写连字符）
    std::string title;               // 第一个 # 标题
    std::vector<ParsedChapter> chapters;
    std::vector<ParsedCard> cards;
};

/// Markdown 解析器
///
/// 解析规则（针对八股文档结构）：
///   1. 第一个 `# XXX` → ParsedDocument.title
///   2. `## 第 N 章 YYY` → ParsedChapter (level=2, no=N)
///   3. `### N.M YYY` → ParsedChapter (level=3, no=N*100+M, parent=N)
///   4. 三级标题及其下面的正文 → 一张 "section" 类型 card（question=标题，answer=正文）
///   5. **Q数字. 问题？** 答案... → 一张 "qa" 类型 card
///      （在题库章节"## 第 11 章 面试题库"附近最常出现）
///   6. 代码块（```...```）内的内容不抽取
class MarkdownParser {
public:
    /// 解析文件
    Result<ParsedDocument> parse_file(const std::filesystem::path& path);

    /// 解析字符串
    /// @param content markdown 内容
    /// @param filename 推断 topic_name 用（可空，则 topic_name 为空）
    Result<ParsedDocument> parse_string(std::string_view content,
                                       std::string filename = "");
};

/// 工具：从文件名推断 topic_name
/// "MySQL 八股文档.md" → "mysql"
/// "redis-interview.md" → "redis-interview"
/// "cpp-network-interview.md" → "cpp-network-interview"
std::string topic_name_from_filename(const std::string& filename);

}  // namespace bagu::parser
