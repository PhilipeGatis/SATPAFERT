import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import viteCompression from 'vite-plugin-compression'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    react(),
    viteCompression({
      algorithm: 'gzip', // ESPAsyncWebServer native support
      ext: '.gz',
      deleteOriginFile: true // Delete bloated uncompressed HTML/JS/CSS to save ESP32 flash memory
    })
  ],
  build: {
    outDir: '../data', // Spit the finalized UI right where PlatformIO LittleFS looks
    emptyOutDir: true,
    target: 'esnext',
    modulePreload: { polyfill: false }, // Avoid obsolete code bloat
    assetsInlineLimit: 100000,
    rollupOptions: {
      output: {
        // Flat file output to simplify ESP32 serving (no nested subfolders)
        entryFileNames: `assets/[name].js`,
        chunkFileNames: `assets/[name].js`,
        assetFileNames: `assets/[name].[ext]`
      }
    }
  }
})
