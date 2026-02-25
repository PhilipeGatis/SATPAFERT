/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        bg: 'var(--bg)',
        card: 'var(--card)',
        accent: 'var(--accent)',
        accent2: 'var(--accent2)',
        warn: 'var(--warn)',
        danger: 'var(--danger)',
        text: 'var(--text)',
        muted: 'var(--muted)',
        border: 'var(--border)',
      }
    },
  },
  plugins: [],
}
