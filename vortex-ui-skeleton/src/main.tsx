import React from 'react';
import ReactDOM from 'react-dom/client';
import { AppShell } from '@/app/AppShell';
import '@xyflow/react/dist/style.css';
import './index.css';

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <AppShell />
  </React.StrictMode>,
);
