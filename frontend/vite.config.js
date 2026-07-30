import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    port: 3000,
    proxy: {
      '/repositories': 'http://127.0.0.1:8080',
      '/jobs': 'http://127.0.0.1:8080',
      '/analysis': 'http://127.0.0.1:8080',
      '/health': 'http://127.0.0.1:8080',
    }
  }
})
