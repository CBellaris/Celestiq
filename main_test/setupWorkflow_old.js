import fs from "fs";
import path from "path";
import {exec} from "child_process";

// 读取任务文件
function loadTasks(taskFile) {
    const taskFilePath = path.resolve(process.cwd(), taskFile);
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

// 执行任务
async function executeTask(task) {
    if (task.completed) {
        console.log(`Skipping completed task: ${task.description}`);
        return {succeed: true, stdout: `Skipping completed task: ${task.description}`}
    }

    console.log(`Starting task: ${task.description}`);
    logMessage(`Starting task: ${task.description}`);


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
        const stdout = await executeCommand(fullCommand, workingDir);

        task.completed = true;
        console.log(`Task completed: ${task.description}`);
        logMessage(`Task completed: ${task.description}`);
        return {succeed: true, stdout: stdout}
    } catch (error) {
        console.error(`Error executing task: ${task.description}`);
        logMessage(`Error executing task: ${task.description} - ${error.message}`);
        return {succeed: false, stdout: error}
    }
}

// 执行命令
function executeCommand(command, workingDir) {
    return new Promise((resolve, reject) => {
        exec(`cmd.exe /k "${command}"`, { cwd: workingDir }, (error, stdout, stderr) => {
            if (error) {
                logMessage(`Command failed: ${command} - ${error.message}`);
                reject(error);
            } else {
                logMessage(`Command succeeded: ${command}\n${stdout}`);
                resolve(stdout);
            }
        });
    });
}

// 主任务处理函数
async function processTasks(taskFile, callback) {
    const tasks = loadTasks(taskFile);

    try {
      const resultEachTask = []
        for (const task of tasks) {
          const res = await executeTask(task); // 顺序执行任务
          resultEachTask.push(res)

          if (typeof callback === 'function') {
            // 调用回调函数，传递当前的任务结果和其他相关信息
            callback(resultEachTask);
          }
        }
        saveTasks(tasks, taskFile);
        console.log('All tasks processed.');
        logMessage('All tasks processed.');
        return { success: true, message: 'All tasks processed.' };
    } catch (error) {
        console.error('Execution halted due to error:', error.message);
        logMessage(`Execution halted due to error: ${error.message}`);
        return { success: false, message: error.message };
    }
}

// 注册 IPC 通信
function runTasks(ipcMain) {
  ipcMain.handle('run-tasks', async (_, taskFile, callback) => {
    try {
      return await processTasks(taskFile, callback);
    } catch (error) {
      console.error('An unexpected error occurred:', error);
      logMessage(`Unexpected error: ${error.message}`);
      return {success: false, message: error.message};
    }
  });
}

export {runTasks}

function mycallback(res) {
  console.log(res[res.length - 1].succeed, res[res.length - 1].stdout)
}
try {
  await processTasks('tasks.json', mycallback)
} catch (error){
  console.error('An unexpected error occurred:', error);
}
