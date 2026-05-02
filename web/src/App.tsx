import { BrowserRouter, Routes, Route } from 'react-router-dom';

import { AppLayout } from './components/AppLayout';
import { CardPage } from './pages/CardPage';
import { HomePage } from './pages/HomePage';
import { PlaceholderPage } from './pages/PlaceholderPage';
import { SearchPage } from './pages/SearchPage';
import { TopicPage } from './pages/TopicPage';

function App() {
  return (
    <BrowserRouter>
      <AppLayout>
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/topics/:name" element={<TopicPage />} />
          <Route path="/cards/:id" element={<CardPage />} />
          <Route path="/search" element={<SearchPage />} />
          <Route
            path="/review"
            element={
              <PlaceholderPage
                title="复习模式"
                hint="复习页将在 Sprint 3 实现，含 SM-2 评分按钮、键盘快捷键、移动端触屏。"
              />
            }
          />
          <Route
            path="/stats"
            element={
              <PlaceholderPage
                title="学习统计"
                hint="统计页将在 v0.4 实现，含连续打卡、热力图、薄弱卡片。"
              />
            }
          />
        </Routes>
      </AppLayout>
    </BrowserRouter>
  );
}

export default App;
