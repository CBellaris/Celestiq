import fs from "fs";
import path from "path";
import {BrowserWindow, dialog} from "electron"
import sharp from "sharp";
import chokidar from "chokidar"
import { resolveAppPath } from '../pathManager.js';

const PATH_TO_IMAGE = './outSrc/images'

let watcher = null
let mainwindow = null

// 监控文件夹变化
function watchFolder(folderPath) {
  if (watcher) {
    watcher.close();
  }

  watcher = chokidar.watch(folderPath, {
    ignored: /^\./,
    persistent: true,
  });

  // 处理文件变化
  watcher
    .on('add', async(filePath) => {
      await handleFileChange('add', filePath);
    })
    .on('unlink', async(filePath) => {
      await handleFileChange('delete', filePath);
    });
}

// 处理文件变化逻辑
async function handleFileChange(type, filePath_) {
  const filePath = filePath_.replace(/\\/g, '/')
  const isImage = /\.(png|jpg|jpeg|gif)$/i.test(filePath);
  const isTextFile = /\.txt$/i.test(filePath);

  if (isImage) {
    const dimensions = await getImageDimensions(filePath); // 获取图像尺寸
    const fileName = path.basename(filePath);
    // 构造 JSON 可序列化的数据
    const payload = {
      type,
      fileType: 'image',
      fileName,
      filePath,
      dimensions,
    };
    console.log('Sending payload:', payload);
    mainwindow.webContents.send('file-change', payload);
  }

  if (isTextFile) {
    const fileName = path.basename(filePath);
    let content = ''
    if(type === 'add') {
      content = fs.readFileSync(filePath, 'utf8'); // 读取文件内容
    }
    const payload = {
        type,
        fileType: 'text',
        fileName,
        filePath,
        content,
      };
    console.log('Sending payload:', payload);
    mainwindow.webContents.send('file-change', payload);
  }
}

// 获取图像尺寸
async function getImageDimensions(filePath) {
  try{
    const metadata = await sharp(filePath).metadata(); // 使用 image-size 包
    return { width:metadata.width, height:metadata.height};
  }catch (err){
    return { width:'-1', height:'-1'};
  }
}



function imageHandler(ipcMain){
  mainwindow = BrowserWindow.getFocusedWindow() // 或获取特定窗口实例
  const imageSaveDirectory = resolveAppPath(PATH_TO_IMAGE);

  // 确保保存目录存在
  if (!fs.existsSync(imageSaveDirectory)) {
    fs.mkdirSync(imageSaveDirectory);
  }


  ipcMain.on('start-watch', () => {
    // 开始监控
    watchFolder(imageSaveDirectory);

    // 发送初始文件扫描结果
    // const files = fs.readdirSync(imageSaveDirectory);
    // files.forEach((file) => {
    //   console.log('init file: ',file)
    //   const filePath = path.join(imageSaveDirectory, file);
    //   handleFileChange('add', filePath);
    // });
  });

  ipcMain.handle('select-folder', async () => {
    const result = await dialog.showOpenDialog({
      properties: ['openDirectory'],
    });

    if (result.canceled) return null;

    const folderPath = result.filePaths[0];
    const files = fs.readdirSync(folderPath).filter((file) => {
      const ext = path.extname(file).toLowerCase();
      return ['.png', '.jpg', '.jpeg', '.gif', '.bmp'].includes(ext);
    });

    return files.map((file) => path.join(folderPath, file));
  });

  ipcMain.handle('save-image', async (event, fileName, fileBuffer) => {
    let savePath = path.resolve(imageSaveDirectory, fileName); // 保存到指定目录
    savePath = savePath.replace(/\\/g, '/')
    try {
      // 保存文件
      fs.writeFileSync(savePath, Buffer.from(fileBuffer));
      return '成功保存'
    } catch (err) {
      return `保存失败：${err}`
    }
  });

  ipcMain.handle('delete-image', async (event, filePath) => {
    try {
      if (fs.existsSync(filePath)) {
        fs.unlinkSync(filePath); // 删除文件
      }
      return true;
    } catch (err) {
      console.error('Error deleting image:', err);
      return false;
    }
  });
}

export {imageHandler}
