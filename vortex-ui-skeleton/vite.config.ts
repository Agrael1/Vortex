import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig(({ mode }) => {
  const outDir = mode === 'rc' ? path.resolve(__dirname, '../rc/ui') : 'dist';

  return {
    plugins: [react()],
    base: './',
    build: { outDir, emptyOutDir: true },
    resolve: {
      alias: {
        '@': path.resolve(__dirname, './src'),
        '@state': path.resolve(__dirname, './src/state'),
        '@services': path.resolve(__dirname, './src/services'),
        '@bridge': path.resolve(__dirname, './src/bridge'),
      },
    },
    test: {
      globals: true,
      environment: 'jsdom',
      setupFiles: './vitest.setup.ts',
      coverage: {
        provider: 'v8',
        reporter: ['text', 'html'],
      },
    },
  };
});
