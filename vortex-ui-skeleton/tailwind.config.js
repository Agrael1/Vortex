/** @type {import('tailwindcss').Config} */
export default {
  darkMode: ['class'],
  content: ['./index.html', './src/**/*.{ts,tsx,js,jsx}'],
  safelist: [
    'mosaic-window-controls',
    'mosaic-default-control',
    'mosaic-default-control-replace',
    'mosaic-default-control-split',
    'mosaic-default-control-expand',
    'mosaic-default-control-close',
  ],
  theme: {
    extend: {
      colors: {
        background: '#01030b',
        foreground: '#f3f6ff',
        muted: '#9fb4d9',
        border: '#1d2740',
        input: '#0b1424',
        card: '#0b1424',
        'card-foreground': '#f3f6ff',
        ui: {
          bg: '#050914',
          panel: '#0b1424',
          surface: '#111c31',
          border: '#1d2740',
          accent: '#8fd3ff',
          accentSoft: '#7aa7ff',
          text: '#f3f6ff',
          muted: '#9fb4d9',
        },
      },
      borderRadius: {
        xl: '0.75rem',
        '2xl': '1rem',
      },
    },
  },
  plugins: [],
};
