import { BrowserRouter, HashRouter, Routes, Route, Navigate } from 'react-router-dom';
import { Splash } from '@/app/routes/Splash';
import { Hub } from '@/app/routes/Hub';
import { EditorGate } from '@/app/routes/EditorGate';
import { useProjectPersistence } from '@state/hooks/useProjectPersistence';
import { useEngineAlerts } from '@state/hooks/useEngineAlerts';
import { useEnginePersistenceEvents } from '@state/hooks/useEnginePersistenceEvents';
import { useConsoleBridge } from '@state/hooks/useConsoleFeed';
import { NotificationTray } from '@/app/components/NotificationTray';
import { IntroOverlay } from '@/app/components/IntroOverlay';

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
  useEngineAlerts();
  useEnginePersistenceEvents();
  useConsoleBridge();
  const RouterComponent = shouldUseHashRouter() ? HashRouter : BrowserRouter;

  return (
    <RouterComponent>
      <IntroOverlay />
      <NotificationTray />
      <Routes>
        <Route path="/" element={<Navigate to="/splash" replace />} />
        <Route path="/splash" element={<Splash />} />
        <Route path="/hub" element={<Hub />} />
        <Route path="/editor" element={<EditorGate />} />
      </Routes>
    </RouterComponent>
  );
}
