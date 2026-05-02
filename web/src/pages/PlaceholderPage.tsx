interface Props {
  title: string;
  hint: string;
}

/// 占位页（v0.4 才实现的页面）
export function PlaceholderPage({ title, hint }: Props) {
  return (
    <div className="text-center py-12">
      <h1 className="text-2xl font-semibold mb-3">{title}</h1>
      <p className="text-slate-500 max-w-md mx-auto">{hint}</p>
      <p className="text-xs text-slate-400 mt-6">
        当前版本暂未实现，请用 CLI 命令使用此功能
      </p>
    </div>
  );
}
