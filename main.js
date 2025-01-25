import { app, BrowserWindow, ipcMain, protocol ,shell } from 'electron'
import path from "path"
import { resolveAppPath } from './pathManager.js';
import {readMarkdown, readJson} from "./main_test/tools.js";
import {imageHandler} from "./main_test/imageHandler.js";
import {Qwen2VLHandler} from "./main_test/Qwen2VL.js";
import {storeOperations} from "./main_test/userConfig.js"
import {handleFlask} from "./main_test/handleFlask.js";

import {run_cmd_handler} from "./main_components/run_cmd.js"
import fs from 'fs'

let mainWindow = null; // 用于保存主窗口实例

const createWindow = () => {
  mainWindow = new BrowserWindow({
    //width: 1050,
    width: 1300,
    height: 800,
    frame: false, // 移除原生标题栏
    webPreferences: {
      preload: resolveAppPath('./preload/preload.js'), // 指定预加载脚本
      contextIsolation: true, // 强制隔离上下文
      enableRemoteModule: false, // 禁用 remote 模块
      // webSecurity: false
    }
  })
  mainWindow.loadURL('http://localhost:5173/')
  //mainWindow.loadFile(resolveAppPath('dist/index.html'))  // 加载构建后的 Vue 前端页面
  mainWindow.webContents.openDevTools()
}

app.setPath('userData', resolveAppPath('./ElectronData'))
app.whenReady().then(() => {
  protocol.handle('local', async (request) => {
    const url = decodeURIComponent(request.url.replace('local://', '')); // 去掉协议头并解码
    function fixDriveLetterAndResolvePath(url) {
      // 检查路径是否以字母开头，后面跟着'/'，这可能是丢失了冒号的Windows绝对路径
      const driveLetterPattern = /^[A-Za-z](?=\/)/;
      if (driveLetterPattern.test(url)) {
          // 在盘符后添加冒号，并将所有斜杠替换为正确的分隔符
          const fixedPath = url.replace(driveLetterPattern, match => match + ':');
          const normalizedPath = path.normalize(fixedPath.split('/').join(path.sep));
          return normalizedPath;
      }

      // 如果不是上述情况，则是相对于项目的路径
      return resolveAppPath(url);
    }
    const filePath = fixDriveLetterAndResolvePath(url);

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

  // 在主进程注册事件监听
  setupMainProcessEvents()
  imageHandler(ipcMain)

  // 注册函数事件
  readMarkdown(ipcMain)
  readJson(ipcMain)
  storeOperations(ipcMain)
  Qwen2VLHandler(ipcMain)
  handleFlask(ipcMain)

  run_cmd_handler(ipcMain)

  // 用于打开外部页面
  ipcMain.on('open-external', (event, url) => {
    shell.openExternal(url);
  });
})

app.on('window-all-closed', () => {
  // eslint-disable-next-line no-undef
  if (process.platform !== 'darwin') app.quit()
})

// 设置自定义窗口事件
function setupMainProcessEvents() {
  ipcMain.on('window-minimize', () => {
    if (mainWindow) mainWindow.minimize();
  });

  ipcMain.on('window-toggle-maximize', () => {
    if (mainWindow) {
      if (mainWindow.isMaximized()) {
        mainWindow.unmaximize();
      } else {
        mainWindow.maximize();
      }
    }
  });

  ipcMain.on('window-close', () => {
    if (mainWindow) mainWindow.close();
  });
}


