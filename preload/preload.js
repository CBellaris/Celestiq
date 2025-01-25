const { contextBridge, ipcRenderer , shell } = require('electron');

const electron = {
  ipcRenderer: {
    send: (channel, data) => {
      ipcRenderer.send(channel, data);
    },
    on: (channel, callback) => {
      ipcRenderer.on(channel, (_, data) =>callback(data));
    },
  }
}
contextBridge.exposeInMainWorld('electron', electron);

const electronAPI = {
  startCommand: (commandId, command, args) =>
    ipcRenderer.send('start-command', commandId, command, args),
  stopCommand: (commandId) =>
    ipcRenderer.send('stop-command', commandId),
  onCommandOutput: (callback) =>
    ipcRenderer.on('command-output', (event, commandId, output) => callback(commandId, output)),
  removeCommandOutputListener: (callback) =>
    ipcRenderer.removeListener('command-output', callback),
  openExternal: (url) => ipcRenderer.send('open-external', url)
}
contextBridge.exposeInMainWorld('electronAPI', electronAPI);

const MyAPI = {
  readMarkdown: (fileName) => ipcRenderer.invoke('readMarkdown', fileName),
  readJson: (fileName) => ipcRenderer.invoke('readJson', fileName),
  Qwen2VL: (prompt, imagePath)=>ipcRenderer.invoke('Qwen2VL-inference', prompt, imagePath),
  onQwen2VL: (callback)=>ipcRenderer.on('Qwen2VL-output', (_, output)=>callback(output)),
  stopOnQwen2VL:()=>ipcRenderer.removeAllListeners('Qwen2VL-output')
}
// 安全暴露 ipcRenderer 接口
contextBridge.exposeInMainWorld('MyAPI', MyAPI);

const userConfig = {
  get: (key) => ipcRenderer.invoke('userConfig-get', key),
  set: (key, val) => ipcRenderer.invoke('userConfig-set', key, val),
}
contextBridge.exposeInMainWorld('userConfig', userConfig);

// const taskProcessorAPI = {
//     tasks: {
//         process: (taskFile, continueOnError = false) => ipcRenderer.invoke('processTasks', taskFile, continueOnError),
//     },
//     events: {
//         on: (eventName, callback) => ipcRenderer.on(eventName, (_, data, ...args) => callback(data, ...args)),
//         off: (eventName, callback) => ipcRenderer.removeListener(eventName, callback),
//     },
// };
// contextBridge.exposeInMainWorld('taskProcessor', taskProcessorAPI);

const imageHandler = {
  saveImage: (fileName, fileBuffer) => ipcRenderer.invoke('save-image', fileName, fileBuffer),
  picBrowser: ()=>ipcRenderer.invoke('select-folder'),
  deleteImage: (filePath) =>ipcRenderer.invoke('delete-image', filePath),
  startWatch: ()=>ipcRenderer.send('start-watch'),
  onChange:(callback)=>ipcRenderer.on('file-change', (_, data)=>callback(data)),
  stopOnChange: ()=>ipcRenderer.removeAllListeners('file-change')
}
// 安全暴露 ipcRenderer 接口
contextBridge.exposeInMainWorld('imageHandler', imageHandler);

const requests ={
  sendTask: (taskInfo)=> ipcRenderer.invoke('send-task-py', taskInfo)
}
contextBridge.exposeInMainWorld('taskRequest', requests);
