#pragma once

namespace bagu::cli {

/// 执行 `bagu init [--force]`
///
/// 初始化数据目录、写默认配置、（可选）创建空数据库。
///
/// @param force 已存在则覆盖
/// @return CLI 退出码
int run_init(bool force);

}  // namespace bagu::cli
