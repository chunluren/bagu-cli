#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"

namespace bagu::service {

/// Anki-importable plain-text export
///
/// 输出格式（Anki "Notes in Plain Text" 直接导入）：
///   - 第一列：Front（问题）
///   - 第二列：Back（答案，含 markdown）
///   - 第三列：Tags（空格分隔；至少含 topic name）
///   - 列分隔符：TAB
///   - 行分隔符：LF
///   - 多行字段：用 `<br>` 替换换行（Anki 默认 HTML 渲染会识别）
///   - TAB 和 `<br>` 字面量：转义为占位符（极少见）
struct AnkiExportOptions {
    std::string topic;       // 空 = 全部主题
    bool include_section_cards = false;  // 默认只导 qa 卡，跳过纯章节占位卡
};

struct AnkiExportSummary {
    int total_cards = 0;
    int written = 0;
    int skipped = 0;
};

class ExportService {
public:
    explicit ExportService(db::Database& db) : db_(db) {}

    /// 导出 Anki txt 到输出流
    Result<AnkiExportSummary> export_anki(const AnkiExportOptions& opts,
                                          std::ostream& out);

private:
    db::Database& db_;
};

}  // namespace bagu::service
