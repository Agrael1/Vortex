import { BrowserRouter, HashRouter, Routes, Route, Navigate } from 'react-router-dom';
import { Splash } from '@/app/routes/Splash';
import { Hub } from '@/app/routes/Hub';
import { EditorGate } from '@/app/routes/EditorGate';
import { useProjectPersistence } from '@state/hooks/useProjectPersistence';

const shouldUseHashRouter = () => {
  if (typeof window === 'undefined') {
    return false;
  }

  const { protocol, host } = window.location;

  if (protocol === 'file:' || protocol === 'vortex:') {
    return true;
  }

  return (protocol === 'http:' || protocol === 'https:') && host === 'vortex';
};

export function AppShell() {
  useProjectPersistence();
  const RouterComponent = shouldUseHashRouter() ? HashRouter : BrowserRouter;

  return (
    <RouterComponent>
      <Routes>
        <Route path="/" element={<Navigate to="/splash" replace />} />
        <Route path="/splash" element={<Splash />} />
        <Route path="/hub" element={<Hub />} />
        <Route path="/editor" element={<EditorGate />} />
      </Routes>
    </RouterComponent>
  );
}
