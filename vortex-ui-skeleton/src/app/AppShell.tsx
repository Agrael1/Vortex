import { BrowserRouter, HashRouter, Routes, Route, Navigate } from 'react-router-dom';
import { DockLayout } from '@/app/layout/DockLayout';
import { Splash } from '@/app/routes/Splash';
import { Hub } from '@/app/routes/Hub';

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
  const RouterComponent = shouldUseHashRouter() ? HashRouter : BrowserRouter;

  return (
    <RouterComponent>
      <Routes>
        <Route path="/" element={<Navigate to="/hub" replace />} />
        <Route path="/splash" element={<Splash />} />
        <Route path="/hub" element={<Hub />} />
        <Route path="/editor" element={<DockLayout />} />
      </Routes>
    </RouterComponent>
  );
}