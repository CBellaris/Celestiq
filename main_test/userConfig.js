import Store from "electron-store";
import { resolveAppPath } from '../pathManager.js';


const userStore = new Store({
  cwd: resolveAppPath('./MyData'),
});

// 获取值
function get(key) {
  return userStore.get(key);
}

// 设置值
function set(key, value) {
  userStore.set(key, value);
}

// // 删除值
// function remove(key) {
//   userStore.delete(key);
// }
//
// // 检查是否存在
// function has(key) {
//   return userStore.has(key);
// }

function storeOperations(ipcMain) {
  ipcMain.handle("userConfig-get", async (event, key) => {
    try {
      const data = get(key)
      return { success: true, content: data }
    } catch (err) {
      console.error("Error reading userConfig:", err)
      return { success: false, error: err.message }
    }
  })

  ipcMain.handle("userConfig-set", async (event, key, val) => {
    try {
      set(key, val)
      return { success: true }
    } catch (err) {
      console.error("Error reading userConfig:", err)
      return { success: false, error: err.message }
    }
  })
}

export {storeOperations};
