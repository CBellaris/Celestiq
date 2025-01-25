import axios from "axios"

const axiosInstance = axios.create({
  proxy: false, // 明确禁用代理
});

function handleFlask(ipcMain) {
  // 接收渲染进程的请求并向 Flask 服务发送数据
  ipcMain.handle('send-task-py', async (event, taskData) => {
      try {
        console.log('taskData: ', taskData)
        const response = await axiosInstance.post('http://127.0.0.1:5000/process', taskData);
        return response.data;
      } catch (error) {
        console.error(error);
        return { status: 'error', message: 'Failed to process task' };
      }
  });
}

export {handleFlask}
