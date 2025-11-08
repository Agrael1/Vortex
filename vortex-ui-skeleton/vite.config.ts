import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig(({ mode }) => {
  const outDir = mode === 'rc'
    ? path.resolve(__dirname, '../rc')
    : 'dist';

  return {
    plugins: [react()],
    base: './',
    server: {
      host: '127.0.0.1',
      port: 5173,
      strictPort: true
    },
    build: {
      outDir,
      emptyOutDir: true
    },
    resolve: {
      alias: {
        '@': path.resolve(__dirname, './src')
      }
    }
  };
});
