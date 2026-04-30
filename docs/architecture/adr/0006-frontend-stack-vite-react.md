# ADR-0006: 前端选 Vite + React + TypeScript + Tailwind

- **状态**: Proposed
- **日期**: 2026-04-30
- **作者**: Li Yan

## 背景

按 [ADR-0004](./0004-add-http-server-mode.md) 决定做 Web UI。需要选定前端技术栈。

需求：
- 现代 dev 体验（HMR、TypeScript）
- 生产构建小（< 300KB gzip）
- 资源能编译期内嵌到 bagu 二进制
- 不依赖任何 CDN（满足"离线优先"）
- 学习成本低（项目作者前端经验有限）

## 决策

**选用 Vite + React + TypeScript + Tailwind CSS。**

- 构建：Vite 5+
- 框架：React 18 (hooks)
- 类型：TypeScript 5+（strict）
- 样式：Tailwind CSS 3+（无运行时 JS）
- 数据：fetch + 自实现轻量 hooks（不引 SWR / React Query，避免 bundle 膨胀）
- 路由：react-router-dom 6+
- markdown：marked + highlight.js
- 图标：lucide-react

## 备选方案

| 方案 | 优点 | 缺点 | 不选理由 |
|----|----|----|----|
| **Vite + React + TS + Tailwind** | 生态成熟、bundle 小、HMR 快 | React 心智模型 | — |
| Vue 3 + Vite | 模板直观 | 作者更熟悉 React | 偏好 |
| Svelte / SvelteKit | bundle 最小、性能极好 | 生态相对小 | React 招聘 / 学习更通用 |
| Next.js | SSR 能力 | 个人工具不需要 SSR | 过重 |
| 纯 HTML + htmx | 最小 | 复杂交互（review/interview）不便 | 与 SSE / SPA 路由难配合 |
| 纯 Web Components | 无依赖 | 工程化弱 | 工作量大 |

## 后果

### 正面

- **HMR 开发** — 改代码秒级反馈
- **bundle 小** — Tailwind tree-shake + Vite code-split，估算 < 250 KB gzip
- **TypeScript 强类型** — API 类型直接 mirror 后端 model
- **生态丰富** — 任何 UI 组件 / 图表 npm 都能装
- **未来好招人** — 这套技术栈是行业标配

### 负面

- **新增 npm/Node.js 工具链** — 用户构建时多一步 `npm install`
- **学习成本** — React hooks / Tailwind 类名

### 中性

- Tailwind 类名长 → 用 IDE 自动补全 / Prettier 缓解
- React 18 默认 strict mode 可能让组件渲染两次 → 测试环境可关

## 实施

### 项目初始化

```bash
cd /home/ly/workspaces/bagu-cli
npm create vite@latest web -- --template react-ts
cd web
npm install
npm install -D tailwindcss postcss autoprefixer
npx tailwindcss init -p
npm install marked highlight.js lucide-react react-router-dom
```

### 关键配置

**`vite.config.ts`：**
```ts
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/api': { target: 'http://127.0.0.1:8780', changeOrigin: true },
    },
  },
  build: {
    outDir: 'dist',
    rollupOptions: {
      output: {
        manualChunks: {
          'react': ['react', 'react-dom', 'react-router-dom'],
          'markdown': ['marked', 'highlight.js'],
        },
      },
    },
  },
});
```

**`tailwind.config.ts`：**
```ts
export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  darkMode: 'media',
  theme: { extend: {} },
};
```

### 资源内嵌（CMake 集成）

```cmake
# scripts/embed_assets.cmake
file(GLOB_RECURSE WEB_ASSETS web/dist/*)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/embedded_assets.h
    COMMAND ${CMAKE_COMMAND}
        -DDIR=${CMAKE_SOURCE_DIR}/web/dist
        -DOUT=${CMAKE_BINARY_DIR}/embedded_assets.h
        -P ${CMAKE_SOURCE_DIR}/scripts/gen_embedded_assets.cmake
    DEPENDS ${WEB_ASSETS}
)
```

`gen_embedded_assets.cmake` 把每个文件转成 C++ `unsigned char[]` 数组 + 路径表。

### 路由设计

```
/                  → HomePage（主题列表）
/topics/:name      → TopicPage（章节树）
/cards/:id          → CardPage（详情）
/search             → SearchPage
/review             → ReviewPage
/interview          → InterviewPage
/stats              → StatsPage
```

## 相关链接

- [Vite 文档](https://vitejs.dev/)
- [Tailwind CSS](https://tailwindcss.com/)
- [ADR-0004](./0004-add-http-server-mode.md)
- [web-ui-overview.md](../web-ui-overview.md)
