#pragma once

#include <filesystem>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"

namespace bagu::service {

/// 单个文件的导入统计
struct FileImportResult {
    std::string file_name;
    std::string topic_name;
    int chapters_added = 0;
    int cards_added = 0;
    bool skipped = false;        // 内容未变跳过
    bool updated = false;        // 已存在但内容变了
    std::string error;           // 非空表示失败
};

/// 整次导入的统计
struct ImportSummary {
    int total_files = 0;
    int succeeded = 0;
    int skipped = 0;
    int failed = 0;
    int total_cards = 0;
    std::vector<FileImportResult> files;
    double elapsed_seconds = 0.0;
};

/// 导入选项
struct ImportOptions {
    bool force = false;     // 即使内容未变也重新解析入库
};

/// 导入服务
///
/// 调度链路：扫描路径 → 计算 hash 判重 → 解析 markdown → 事务写库
///
/// @code
/// ImportService svc(db);
/// auto r = svc.import_path("/path/to/docs");
/// @endcode
class ImportService {
public:
    explicit ImportService(db::Database& db) : db_(db) {}

    /// 兼容别名
    using Options = ImportOptions;

    /// 导入单个文件或目录（递归）
    Result<ImportSummary> import_path(const std::filesystem::path& path,
                                     ImportOptions opts = {});

private:
    db::Database& db_;

    FileImportResult import_file(const std::filesystem::path& path,
                                 const ImportOptions& opts);
};

}  // namespace bagu::service
