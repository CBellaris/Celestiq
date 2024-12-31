import {spawn} from "child_process";

import fs from "fs";
import path from "path";
import EventEmitter from "events";
import {BrowserWindow} from "electron";

const PATH_TO_TASKFILE = './outSrc'

// 读取任务文件
function loadTasks(taskFile) {
    const taskFilePath = path.resolve(process.cwd(), PATH_TO_TASKFILE ,taskFile);
    if (!fs.existsSync(taskFilePath)) {
        throw new Error(`Task file not found: ${taskFilePath}`);
    }
    return JSON.parse(fs.readFileSync(taskFilePath, 'utf-8'));
}

// 保存任务文件
function saveTasks(tasks, taskFile) {
    const taskFilePath = path.resolve(process.cwd(), taskFile);
    fs.writeFileSync(taskFilePath, JSON.stringify(tasks, null, 4), 'utf-8');
}

// 写入日志
function logMessage(message) {
    const logFile = path.resolve(process.cwd(), 'execution.log');
    const timestamp = new Date().toISOString();
    fs.appendFileSync(logFile, `[${timestamp}] ${message}\n`, 'utf-8');
}

class TaskProcessor extends EventEmitter {
    constructor() {
        super();
        this.resultEachTask = []; // 存储任务结果
    }

    // 记录任务结果
    recordTaskResult(task, succeed, output) {
        const result = { task, succeed, output };
        this.resultEachTask.push(result);
        this.emit('taskUpdate', result); // 实时通知
    }

    // 执行任务
    async executeTask(task) {
        if (task.completed) {
            const message = `Skipping completed task: ${task.description}`;
            console.log(message)
            this.recordTaskResult(task, true, message);
            return;
        }

        try {
            // 构建完整的命令
            let fullCommand = '';
            if (task.env_commands && task.env_commands.length > 0) {
                fullCommand += task.env_commands.join('&& ') + '&& ';
            }
            fullCommand += task.commands;

            console.log(`Executing command: ${fullCommand} in ${task.working_dir}`);
            // 检查并创建工作目录
            const workingDir = path.resolve(process.cwd(), task.working_dir);
            if (!fs.existsSync(workingDir)) {
                console.log(`Working directory does not exist. Creating: ${workingDir}`);
                fs.mkdirSync(workingDir, { recursive: true });
            }

            const output = await this.executeCommand(fullCommand, workingDir);

            task.completed = true;
            this.recordTaskResult(task, true, output);
        } catch (error) {
            console.error(`Error executing task: ${task.description}`);
            this.recordTaskResult(task, false, error.message);
            throw error; // 继续抛出错误，便于外部处理
        }
    }

    // 执行命令（实时输出）
    executeCommand(command, workingDir) {
        return new Promise((resolve, reject) => {
            const process = spawn('cmd.exe', ['/c', command], { cwd: workingDir, shell: true });

            let output = '';
            let errorOutput = '';

            process.stdout.on('data', (data) => {
                const text = data.toString();
                output += text;
                this.emit('commandOutput', text); // 实时输出
            });

            process.stderr.on('data', (data) => {
                const text = data.toString();
                output += text;
                this.emit('commandOutput', text); // 实时输出
            });

            process.on('error', (err) => {
                console.error(`Process error: ${err.message}`);
                reject(new Error(`Process error: ${err.message}\n${errorOutput}`));
            });

            process.on('exit', (code) => {
                if ([0, 128].includes(code)) {
                    resolve(output);
                } else {
                    console.error(`Process exited with code ${code}`);
                    reject(new Error(`Command failed with exit code ${code}:\n${output}\n${errorOutput}`));
                }
            });

            // 如果 `close` 事件触发，确保一定清理资源
            process.on('close', (code) => {
                console.log(`Process closed with code ${code}`);
                if (code !== 0) {
                    reject(new Error(`Process closed unexpectedly with code ${code}:\n${output}\n${errorOutput}`));
                }
            });
        });
    }

    // 主任务处理函数
    async processTasks(taskFile, continueOnError = false) {
        try {
          const tasks = loadTasks(taskFile);
          for (const task of tasks) {
            try {
              this.emit('taskProgress', task.description); // 实时通知进度
              await this.executeTask(task); // 顺序执行任务
            } catch (error) {
              if (!continueOnError) {
                console.error('Halting execution due to task failure.');
                throw error; // 如果配置为终止，则抛出错误
              }
            }
          }
          saveTasks(tasks, taskFile); // 保存所有任务结果
          logMessage(this.resultEachTask)
          this.emit('allTasksCompleted', this.resultEachTask); // 通知任务完成
        } catch (error) {
          console.error('Execution halted due to error:', error.message);
          this.emit('error', error); // 通知任务出错
        }
    }
}

// 设置 IPC 处理器，用于接收渲染进程的调用
function setupWorkflow(ipcMain){
  // 实例化任务处理器
  const processor = new TaskProcessor();

  // 注册事件监听，转发给渲染进程
  processor.on('taskUpdate', ({ task, succeed, output }) => {
    console.log(`Task updated: ${task.description}, Success: ${succeed}, Output: ${output}`);
    const focusedWindow = BrowserWindow.getFocusedWindow(); // 或获取特定窗口实例
    if (focusedWindow) {
        const task_description = task.description
        focusedWindow.webContents.send('taskUpdate', { task_description, succeed, output });
    }
  });

  processor.on('commandOutput', (output) => {
    console.log(`Command output: ${output}`);
    const focusedWindow = BrowserWindow.getFocusedWindow(); // 或获取特定窗口实例
    if (focusedWindow) {
        focusedWindow.webContents.send('commandOutput', output);
    }
  });

  processor.on('taskProgress', (results) => {
    console.log(`Current task progress: ${results.length} tasks completed.`);
    const focusedWindow = BrowserWindow.getFocusedWindow(); // 或获取特定窗口实例
    if (focusedWindow) {
        focusedWindow.webContents.send('taskProgress', results);
    }
  });

  processor.on('allTasksCompleted', (results) => {
    console.log('All tasks have been processed successfully:', results);
    const focusedWindow = BrowserWindow.getFocusedWindow(); // 或获取特定窗口实例
    if (focusedWindow) {
        focusedWindow.webContents.send('allTasksCompleted', results);
    }
  });

  processor.on('error', (error) => {
    console.error('An error occurred during task processing:', error.message);
    const focusedWindow = BrowserWindow.getFocusedWindow(); // 或获取特定窗口实例
    if (focusedWindow) {
        focusedWindow.webContents.send('error', error);
    }
  });

  ipcMain.handle('processTasks', async (event, taskFile, continueOnError = false) => {
    try {
      await processor.processTasks(taskFile, continueOnError);
      return { success: true, message: 'Tasks processed successfully' };
    } catch (error) {
      return { success: false, message: error.message };
    }
});
}

export {setupWorkflow}







// 注册事件监听
// processor.on('taskUpdate', ({ task, succeed, output }) => {
//     console.log(`Task updated: ${task.description}, Success: ${succeed}, Output: ${output}`);
// });
//
// processor.on('commandOutput', (output) => {
//     console.log(`Command output: ${output}`);
// });
//
// processor.on('taskProgress', (results) => {
//     console.log(`Current task progress: ${results.length} tasks completed.`);
// });
//
// processor.on('allTasksCompleted', (results) => {
//     console.log('All tasks have been processed successfully:', results);
// });
//
// processor.on('error', (error) => {
//     console.error('An error occurred during task processing:', error.message);
// });
// 启动任务处理
// try {
//     await processor.processTasks('tasks.json');
// } catch (error) {
//     console.error('An unexpected error occurred:', error.message);
// }
