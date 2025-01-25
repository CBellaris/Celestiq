import {spawn} from "child_process";
import jschardet from "jschardet";
import iconv from "iconv-lite";
import { resolveAppPath } from '../pathManager.js';
import treeKill from "tree-kill"; // 用于递归终止进程树
const ROOT_DIR = resolveAppPath('')

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

let activeProcesses = {};

function run_cmd_handler(ipcMain){
  ipcMain.on('start-command', (event, commandId, command, args = []) => {
    if (activeProcesses[commandId]) {
      event.reply('command-output', commandId, 'Error: Command ID already in use.\n');
      return;
    }

    const process = spawn(command, args, {cwd: ROOT_DIR, shell: true });
    activeProcesses[commandId] = process;

    // Handle stdout
    process.stdout.on('data', (data) => {
      const text = decodeOutput(data)+'\n';  // 解码
      event.reply('command-output', commandId, text);
    });

    // Handle stderr
    process.stderr.on('data', (data) => {
      const text = decodeOutput(data)+'\n';  // 解码
      event.reply('command-output', commandId, `Std wrong: ${text}`);
    });

    // Handle process close
    process.on('close', (code) => {
      event.reply('command-output', commandId, `Command finished with exit code ${code}`);
      delete activeProcesses[commandId];
    });

    // Handle process error
    process.on('error', (error) => {
      event.reply('command-output', commandId, `Process Error: ${error.message}`);
      delete activeProcesses[commandId];
    });

    event.reply('command-output', commandId, `Command "${command}" started.\n`);
  });

  ipcMain.on('stop-command', (event, commandId) => {
    console.log('stopcommand in electron', commandId)
    const process_to_kill = activeProcesses[commandId];
    if (process_to_kill) {
      treeKill(process_to_kill.pid, 'SIGKILL', (err) => {
        if (err) {
          event.reply('command-output', commandId, `Failed to terminate command "${commandId}".\n`);
        } else {
          event.reply('command-output', commandId, `Error: Command "${commandId}" has been terminated.\n`);
          delete activeProcesses[commandId];
        }
      });
    } else {
      event.reply('command-output', commandId, `Error: No running process found with ID ${commandId}.\n`);
    }
  });
}

export {run_cmd_handler}
