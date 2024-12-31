const {contextBridge, ipcRenderer} = require("electron");

const MyAPI = {
  readMarkdown: (fileName) => ipcRenderer.invoke('readMarkdown', fileName),
  readJson: (fileName) => ipcRenderer.invoke('readJson', fileName)
}
// 安全暴露 ipcRenderer 接口
contextBridge.exposeInMainWorld('MyAPI', MyAPI);

const userConfig = {
  get: (key) => ipcRenderer.invoke('userConfig-get', key),
  set: (key, val) => ipcRenderer.invoke('userConfig-get', key, val),
}
contextBridge.exposeInMainWorld('userConfig', userConfig);

const taskProcessorAPI = {
    tasks: {
        process: (taskFile, continueOnError = false) => ipcRenderer.invoke('processTasks', taskFile, continueOnError),
    },
    events: {
        on: (eventName, callback) => ipcRenderer.on(eventName, (_, data) => callback(data)),
        off: (eventName, callback) => ipcRenderer.removeListener(eventName, callback),
    },
};
contextBridge.exposeInMainWorld('taskProcessor', taskProcessorAPI);
