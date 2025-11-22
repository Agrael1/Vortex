import { useEffect } from 'react';
import { useLocation, useNavigate } from 'react-router-dom';
import { useRecoilValue } from 'recoil';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { projectPathAtom } from '@state/atoms/project';
import { DockLayout } from '@/app/layout/DockLayout';

export function EditorGate() {
  const status = useRecoilValue(persistenceStatusAtom);
  const projectPath = useRecoilValue(projectPathAtom);
  const nav = useNavigate();
  const location = useLocation();

  useEffect(() => {
    if (!status.isHydrated) return;
    if (projectPath) return;
    nav('/hub', { replace: true, state: { from: location.pathname } });
  }, [location.pathname, nav, projectPath, status.isHydrated]);

  if (!status.isHydrated) {
    return (
      <div className="w-screen h-screen flex flex-col items-center justify-center gap-4 bg-ui-bg text-gray-300">
        <span className="inline-block h-10 w-10 animate-spin rounded-full border-2 border-ui-border border-t-transparent" />
        <div className="text-center">
          <p className="text-lg font-medium">Preparing editor…</p>
          <p className="text-sm text-gray-500">Loading project state</p>
        </div>
      </div>
    );
  }

  if (!projectPath) {
    return (
      <div className="w-screen h-screen flex flex-col items-center justify-center gap-6 bg-ui-bg text-gray-200">
        <div className="space-y-2 text-center">
          <p className="text-xl font-semibold">Project not selected</p>
          <p className="text-sm text-gray-500">Redirecting you back to the Hub…</p>
        </div>
        <button
          type="button"
          onClick={() => nav('/hub', { replace: true })}
          className="rounded-lg border border-ui-border px-4 py-2 text-sm text-gray-100 hover:border-blue-500/60 hover:text-blue-200"
        >
          Go to Hub
        </button>
      </div>
    );
  }

  return <DockLayout />;
}
