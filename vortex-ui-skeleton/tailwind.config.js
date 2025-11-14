/** @type {import('tailwindcss').Config} */
export default {
  darkMode: ['class'],
  content: [
    './index.html',
    './src/**/*.{ts,tsx,js,jsx}',
  ],
  theme: {
    extend: {
      colors: {
        background: 'hsl(240 10% 3.9%)',
        foreground: 'hsl(0 0% 98%)',
        muted: 'hsl(240 3.7% 15.9%)',
        border: 'hsl(240 3.7% 15.9%)',
        input: 'hsl(240 3.7% 15.9%)',
        card: 'hsl(240 10% 3.9%)',
        'card-foreground': 'hsl(0 0% 98%)',
        ui: {
          bg: '#080b12',
          panel: '#111a26',
          border: '#1f2a3a',
          accent: '#60a5fa',
        },
      },
      borderRadius: {
        xl: '0.75rem',
        '2xl': '1rem',
      },
    },
  },
  plugins: [],
}
