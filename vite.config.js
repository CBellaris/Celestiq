import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  // 修改 base 配置，确保资源路径正确
  base: './', // 重要：设置为相对路径
  build: {
    outDir: 'dist', // 输出目录，默认是 dist
    emptyOutDir: true, // 构建前清理输出目录
    rollupOptions: {
      output: {
        assetFileNames: 'assets/[name]-[hash].[ext]', // 资源文件命名
        chunkFileNames: 'assets/[name]-[hash].js', // chunk 文件命名
        entryFileNames: 'assets/[name]-[hash].js', // 入口文件命名
      },
    },
  },
})
