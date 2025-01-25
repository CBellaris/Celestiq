import {spawn} from "child_process";
import {BrowserWindow} from "electron";
import path from "path";
import fs from "fs";
import { resolveAppPath } from '../pathManager.js';
import iconv from "iconv-lite";
import jschardet from "jschardet";

const pathToQwen2VL = resolveAppPath('./outSrc/Qwen2VL')

let focusedWindow = null

function changeFileExtension(fileName, newExtension = 'txt') {
    // 确保新扩展名以点号开头
    const extensionWithDot = `.${newExtension.replace(/^\./, '')}`;

    // 使用正则表达式移除旧的文件扩展名，并添加新的扩展名
    return fileName.replace(/\.[^/.]*$/, extensionWithDot);
}

function saveStringToFile(text, filePath) {
    fs.writeFile(filePath, text, (err) => {
        if (err) throw err;
        console.log('The file has been saved!');
    });
}

function extractUsefulOutput(input) {
    const lines = input.split('\n');
    let result = [];
    let isCollecting = false;

    for (const line of lines) {
        if (line.startsWith('encode_image_with_clip')) {
            isCollecting = true;
        } else if (line.startsWith('llama_perf_context_print')) {
            isCollecting = false;
        }

        if (isCollecting && !line.startsWith('encode_image_with_clip')) {
            result.push(line);
        }
    }

    const output = result.join('\n').trim();
    // 删除llama_perf_context_print
    const index = output.indexOf('llama_perf_context_print');
    if (index !== -1) {
      return output.slice(0, index)
    }
    return output
}

// 解码输出
function decodeOutput(data) {
    let detected = jschardet.detect(data); // 检测编码
    let encoding = detected.encoding || 'utf8'; // 如果无法检测到编码，则默认使用 utf8

    // 根据检测到的编码进行解码
    let text;
    try {
        text = iconv.decode(data, encoding.toLowerCase());
    } catch (err) {
        console.error(`Failed to decode data with detected encoding ${encoding}:`, err);
        return;
    }
    return text
}

function inference(prompt, imagePath){
  return new Promise((resolve, reject) => {
    const fileName = path.basename(imagePath)
    const fullCommand = `llama-qwen2vl-cli -m Qwen2-VL-7B-Instruct-Q2_K.gguf --mmproj mmproj-Qwen2-VL-7B-Instruct-f16.gguf -p "${prompt}" --image "${imagePath}" --temp 0.1`;
    console.log('fullCommand: ',fullCommand)
    const process = spawn('cmd.exe', ['/c', fullCommand], { cwd: pathToQwen2VL, shell: true, stdio: ['pipe', 'pipe', 'pipe'], encoding: 'buffer'});
    let output = '';

    process.stdout.on('data', (data) => {
      const text = decodeOutput(data);
      output += text;
      const modelOutput = extractUsefulOutput(output)
      if (modelOutput.length>0){
        const payload = {
          name: fileName,
          output: modelOutput
        }
        focusedWindow.webContents.send('Qwen2VL-output', payload); // 实时输出
      }
    });
    process.stderr.on('data', (data) => {
      const text = decodeOutput(data);
      output += text;
      const modelOutput = extractUsefulOutput(output)
      if (modelOutput.length>0){
        const payload = {
          name: fileName,
          output: modelOutput
        }
        focusedWindow.webContents.send('Qwen2VL-output', payload); // 实时输出
      }
    });
    process.on('error', (err) => {
      reject(new Error(`Process error: ${err.message}`));
    });
    process.on('exit', (code) => {
      if ([0, 128].includes(code)) {
        // 保存txt
        const modelOutput = extractUsefulOutput(output)
        if (modelOutput.length>0){
          saveStringToFile(modelOutput, changeFileExtension(imagePath))
        }
        // 返回成功
        resolve(output);
      } else {
        reject(new Error(`Command failed with exit code ${code}:\n${output}`));
      }
    });
    // 如果 `close` 事件触发，确保一定清理资源
    process.on('close', (code) => {
      if (code !== 0) {
        reject(new Error(`Process closed unexpectedly with code ${code}:\n${output}`));
      }
    });
  });
}

function Qwen2VLHandler(ipcMain){
  focusedWindow = BrowserWindow.getFocusedWindow()
  ipcMain.handle('Qwen2VL-inference', async (event, prompt, imagePath)=>{
    try{
      const output = await inference(prompt, imagePath)
      return {
        succeed: true,
        output: output
      }
    } catch (err){
      return {
        succeed: false,
        output: err
      }
    }
  })
}

export {Qwen2VLHandler}
