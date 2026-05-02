import { test as base, expect } from '@playwright/test';

/// 自定义 fixture：在测试前清空 BAGU_HOME 数据库（确保确定性）
///
/// 注意：这只清 db 本身，不重启 server。serve 内部仍持有 db 句柄，
/// 所以我们改写策略：每个 test file 假定一个干净 DB（在 globalSetup 里初始化），
/// 不在测试中 import / mutate。复习 / 面试这种"会写库"的测试要慎用。
export const test = base;
export { expect };
