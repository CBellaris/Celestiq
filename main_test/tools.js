import fs from "fs"
import {marked} from "marked";
import { resolveAppPath } from '../pathManager.js';

const preprocessMediaTags = (markdown, baseDir) => {
  // 正则表达式匹配 <source> 标签中的 src 属性
  const sourcePattern = /<source\s+.*?src=["'](.*?)["'].*?>/g;
  // 正则表达式匹配 <video> 标签中的 src 属性
  const videoPattern = /<video\s+.*?src=["'](.*?)["'].*?>/g;
  // 正则表达式匹配 <img> 标签中的 src 属性
  const imgPattern = /<img\s+.*?src=["'](.*?)["'].*?>/g;

  // 辅助函数来处理每个匹配项
  const processSrcAttribute = (match, src) => {
    if (src && !src.startsWith('http')) {
      const resolvedPath = `local://${baseDir}/${src}`;
      return match.replace(src, resolvedPath);
    }
    return match;
  };

  // 处理 <source> 标签
  let processedMarkdown = markdown.replace(sourcePattern, processSrcAttribute);
  // 处理 <video> 标签中的 src 属性
  processedMarkdown = processedMarkdown.replace(videoPattern, processSrcAttribute);
  // 处理 <img> 标签中的 src 属性
  processedMarkdown = processedMarkdown.replace(imgPattern, processSrcAttribute);

  return marked(processedMarkdown);
};

function readMarkdown(ipcMain) {
  // 处理 Markdown 文件读取请求
  ipcMain.handle("readMarkdown", async (event, fileName) => {
    try {
      const filePath = resolveAppPath('./outSrc/mdFile', fileName)
      const data = fs.readFileSync(filePath, "utf-8")

      const parsedMarkdown = preprocessMediaTags(data, './outSrc/mdFile/asset')
      return { success: true, content: parsedMarkdown }
    } catch (err) {
      console.error("Error reading file:", err)
      return { success: false, error: err.message }
    }
  })
}

function readJson(ipcMain){
  ipcMain.handle("readJson", async (event, fileName) => {
    try {
      const filePath = resolveAppPath('./outSrc/taskFile', fileName)
      const data = JSON.parse(fs.readFileSync(filePath, "utf-8"))
      return { success: true, content: data }
    } catch (err) {
      console.error("Error reading jsonFile:", err)
      return { success: false, error: err.message }
    }
  })
}

export {readMarkdown, readJson}
