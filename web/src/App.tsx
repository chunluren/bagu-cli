import { BrowserRouter, Routes, Route } from 'react-router-dom';

import { AppLayout } from './components/AppLayout';
import { CardPage } from './pages/CardPage';
import { HomePage } from './pages/HomePage';
import { InterviewPage } from './pages/InterviewPage';
import { ReviewPage } from './pages/ReviewPage';
import { SearchPage } from './pages/SearchPage';
import { StatsPage } from './pages/StatsPage';
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
          <Route path="/review" element={<ReviewPage />} />
          <Route path="/interview" element={<InterviewPage />} />
          <Route path="/stats" element={<StatsPage />} />
        </Routes>
      </AppLayout>
    </BrowserRouter>
  );
}

export default App;
