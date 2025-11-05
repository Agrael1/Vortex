/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        ui: {
          bg: '#0f1115',
          panel: '#161a22',
          border: '#242b38',
          text: '#d1d5db',
          accent: '#7c9cf3'
        }
      }
    },
  },
  plugins: [],
}