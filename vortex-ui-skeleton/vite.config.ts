import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig(({ mode }) => {
  const outDir = mode === 'rc'
    ? path.resolve(__dirname, '../rc/ui')
    : 'dist';

  return {
    plugins: [react()],
    base: './',
    build: { outDir, emptyOutDir: true },
    resolve: { alias: { '@': path.resolve(__dirname, './src') } }
  };
});
