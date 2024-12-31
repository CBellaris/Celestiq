import { app, BrowserWindow, ipcMain, protocol } from 'electron'
import path from "path"
import {readMarkdown, readJson} from "./main_test/tools.js";
import {setupWorkflow} from "./main_test/setupWorkflow.js";
import Store from "./main_test/userConfig.js"
import fs from 'fs'

const createWindow = () => {
  const win = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      preload: path.resolve(process.cwd(), './preload/preload.js'), // 指定预加载脚本
      contextIsolation: true, // 强制隔离上下文
      enableRemoteModule: false, // 禁用 remote 模块
      webSecurity: false
    }
  })

  win.loadURL('http://localhost:5173/')
  win.webContents.openDevTools()
}

app.setPath('userData', path.resolve(process.cwd(), './ElectronData'))
app.whenReady().then(() => {
  protocol.handle('local', async (request) => {
    const url = decodeURIComponent(request.url.replace('local://', '')); // 去掉协议头并解码
    const filePath = path.resolve(process.cwd(), url); // 拼接为绝对路径

    // 常见 MIME 类型映射
    const mimeTypes = {
        '.html': 'text/html',
        '.js': 'application/javascript',
        '.css': 'text/css',
        '.json': 'application/json',
        '.png': 'image/png',
        '.jpg': 'image/jpeg',
        '.jpeg': 'image/jpeg',
        '.gif': 'image/gif',
        '.svg': 'image/svg+xml',
        '.mp4': 'video/mp4',
        '.webm': 'video/webm',
        '.ogg': 'video/ogg',
        '.mp3': 'audio/mpeg',
        '.wav': 'audio/wav',
        '.txt': 'text/plain',
        '.pdf': 'application/pdf',
    };

    try {
      const stats = await fs.promises.stat(filePath);
      const extension = path.extname(filePath).toLowerCase(); // 获取文件扩展名
      const mimeType = mimeTypes[extension] || 'application/octet-stream'; // 根据扩展名动态设置 MIME 类型

      const range = request.headers.get('range'); // 获取 range 请求头
      if (range) {
        const [startStr, endStr] = range.replace(/bytes=/, '').split('-');
        const start = parseInt(startStr, 10);
        const end = endStr ? parseInt(endStr, 10) : stats.size - 1;
        const chunkSize = end - start + 1;
        const fileStream = fs.createReadStream(filePath, { start, end });

        return new Response(fileStream, {
          status: 206, // 部分内容状态码
          headers: {
            'Content-Range': `bytes ${start}-${end}/${stats.size}`,
            'Accept-Ranges': 'bytes',
            'Content-Length': chunkSize,
            'Content-Type': mimeType, // 根据文件类型动态设置
          },
        });
      } else {
        const data = await fs.promises.readFile(filePath);

        return new Response(data, {
          headers: {
            'Content-Length': stats.size,
            'Content-Type': mimeType, // 根据文件类型动态设置
          },
        });
        }
    } catch (error) {
      console.error(`无法读取文件: ${filePath}`, error);

      // 错误情况下返回 404 状态
      return new Response('File not found', { status: 404 });
    }
  });
  createWindow()

  readMarkdown(ipcMain)
  readJson(ipcMain)
  setupWorkflow(ipcMain)
  Store(ipcMain)
})

app.on('window-all-closed', () => {
  // eslint-disable-next-line no-undef
  if (process.platform !== 'darwin') app.quit()
})
