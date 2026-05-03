# 导出题库

> v1.1+ · 把 bagu 的卡片导出成可被其他工具吃的格式。

## Anki

[Anki](https://apps.ankiweb.net/) 是最流行的间隔重复软件之一。bagu 可以把卡片导成 Anki "Notes in Plain Text" 格式，三步导入。

### 1. 在 bagu 里导出

```bash
# 全部主题
bagu export anki -o ~/Desktop/bagu-all.txt

# 单个主题
bagu export anki --topic mysql -o ~/Desktop/bagu-mysql.txt

# 也包含章节占位卡（默认仅 qa 卡）
bagu export anki --topic mysql --include-section -o ~/Desktop/bagu-full.txt

# 不指定 -o 时输出到 stdout，方便管道
bagu export anki --topic redis | grep -v '^#' | wc -l
```

完成后会在 stderr 打印：
```
[export anki] total=86 written=30 skipped=56 → /Users/you/Desktop/bagu-mysql.txt
```

### 2. 在 Anki 里导入

1. Anki 主界面 → **File** → **Import**
2. 选刚导出的 `.txt` 文件
3. Anki 会自动识别 `#separator:tab` / `#html:true` / `#columns` / `#tags column` 标头
4. 选择目标 deck（建议为每个 topic 建一个 deck，例如 "bagu::mysql"）
5. **Note Type**：选 "Basic" 或自建一个含 Front/Back/Tags 三字段的 type
6. 点击 **Import**

字段映射：

| Anki 字段 | bagu 字段 | 备注 |
|---|---|---|
| Front | `card.question` | TAB 转 4 空格、换行转 `<br>` |
| Back | `card.answer` | 同上；Markdown 在 Anki 里靠 `#html:true` 渲染（粗体 / 链接生效，列表 / 表格不生效） |
| Tags | `bagu` + topic name + 原 tags | 空格分隔；逗号 / TAB 自动转空格 |

### 3. 第一次同步

如果你用 AnkiWeb：导入后 → **Sync**，手机 / iPad 即可拿到。

## REST API

`bagu serve` 暴露 `/api/export/anki`：

```bash
curl 'http://127.0.0.1:8780/api/export/anki?topic=mysql' -o bagu-mysql.txt
curl 'http://127.0.0.1:8780/api/export/anki' -o bagu-all.txt   # 全部
curl 'http://127.0.0.1:8780/api/export/anki?topic=mysql&include_section=1'
```

响应头：
- `Content-Disposition: attachment; filename="bagu-anki-mysql.txt"`
- `X-Bagu-Export-Total: <总卡片数>`
- `X-Bagu-Export-Written: <实际导出数>`

## 设计取舍

### 为什么不直接发 .apkg？
`.apkg` 本质是 SQLite + zip + 一套复杂的 schema（含 Anki 内部 collection / models / cards / revlog 等）。生成需要：
- 引入 zip 库
- 翻译 SM-2 状态到 Anki 自己的算法字段
- 维护 schema 兼容（Anki 升级时偶尔变）

收益不大（Anki 自家 import txt 已经无损保留题面 + 答案 + tags），所以选 `.txt`。

### 复习历史不会被带过去
导出的是「题面 + 答案 + 标签」，不带 SM-2 间隔 / 难度 / 错误统计。Anki 那边把每张卡当新卡处理。

如果你想完整迁移到 Anki 接管复习，建议：
1. 在 bagu 里把卡片"过一遍"（让 Anki 重新评估难度从零开始也合理）
2. 或者两边并存：bagu 用作 markdown 知识库 + 搜索 + AI 面试，Anki 负责复习

### 为什么 markdown 答案在 Anki 里只显示部分格式
`#html:true` 让 Anki 把答案当 HTML 渲染。markdown 里的 `**bold**` / `[link](url)` 会显示为字面量字符（因为它们不是 HTML）。要修：

```bash
# 用 pandoc 把每条答案先转 HTML
# （TODO：等 v1.x 加 --html 选项内建 markdown→HTML）
```

短期 workaround：在 Anki 里给 Note Type 加一个自定义渲染 JS（用 marked.js）。
