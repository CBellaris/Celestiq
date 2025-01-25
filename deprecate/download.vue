<script setup>
import {ref, reactive, onMounted, onUnmounted} from "vue";

let config = []
const descriptions = []
const model_list_options = reactive([])


const load_config = async()=>{
  try {
    const get_config = await window.userConfig.get("Qwen2VL_gguf_model");
    if (!get_config.success){
      snackbar.value = true
      snackbarColor.value = 'error'
      snackbarMessage.value = '你可能没有下载配置文件！请更新软件'
      return
    }
    config = get_config.content
    config.forEach(option=>{
      console.log("task:", option);
      descriptions.push(option.description)
    })
    console.log("Loaded:", descriptions);
  } catch (error) {
    console.error("Failed to load config:", error);
  }
}

const snackbar = ref(false);
const snackbarMessage = ref("");
const snackbarColor = ref("");

let currentTaskIndex = 0; // 当前下载任务索引

// 开始下载任务
const startDownload = async () => {
  if (currentTaskIndex >= modelList.length) {
    snackbarMessage.value = "所有任务已完成！";
    snackbarColor.value = "success";
    snackbar.value = true;
    return;
  }

  const currentTask = modelList[currentTaskIndex];
  currentTask.status = "进行中";

  // 将任务转换为普通对象
  const taskToSend = {
    taskName: currentTask.taskName,
    taskInfo: { ...currentTask.taskInfo },
  };

  const response = await window.MyAPI.download_ms(taskToSend);
  if (response.succeed) {
    currentTask.status = "已完成";
    currentTask.progress = 100; // 下载完成
    snackbarMessage.value = `${currentTask.taskName} 下载完成！`;
    snackbarColor.value = "success";
    snackbar.value = true;

    // 开始下一个任务
    currentTaskIndex++;
    startDownload();
  } else {
    currentTask.status = "失败";
    snackbarMessage.value = `${currentTask.taskName} 下载失败：${response.message}`;
    snackbarColor.value = "error";
    snackbar.value = true;
  }
  await window.userConfig.set('models_for_Qwen2VL_gguf', JSON.parse(JSON.stringify(modelLists)))
};

// 保存事件监听器的引用
let downloadProgressHandler;
onMounted(() => {
  loadModelLists();

  // 保存事件监听器的引用
  downloadProgressHandler = (data) => {
    const currentTask = modelList[currentTaskIndex];
    if (!currentTask || currentTask.status !== "进行中") return;

    // 更新当前任务的进度信息
    currentTask.progress = parseFloat(data.progress);
    currentTask.downloaded = data.downloaded || currentTask.downloaded;
    currentTask.totalSize = data.totalSize || currentTask.totalSize;
    currentTask.remainingTime = data.remainingTime || currentTask.remainingTime;
    currentTask.speed = data.speed || currentTask.speed;
  };

  // 监听 Electron 主线程的进度回调
  window.electron.ipcRenderer.on("download_ms_output", downloadProgressHandler);
});
// 在组件卸载时移除事件监听器
onUnmounted(() => {
  window.electron.ipcRenderer.off("download_ms_output", downloadProgressHandler);
});
</script>

<template>
  <v-card>
    <v-card-title>下载模型</v-card-title>
    <v-card-text>
      <div v-for="(task, index) in modelList" :key="index" class="mb-4">
        <h3>{{ task.taskName }}</h3>
        <div>状态: {{ task.status }}</div>
        <div>进度: {{ task.progress }}%</div>
        <v-progress-linear
          :model-value="task.progress"
          rounded
          bg-color="blue-grey"
          color="secondary"
        ></v-progress-linear>
        <div>
          <p>已下载: {{ task.downloaded }} / {{ task.totalSize }}</p>
          <p>速度: {{ task.speed }}</p>
          <p>剩余时间: {{ task.remainingTime }}</p>
        </div>
      </div>
    </v-card-text>
    <v-card-actions>
      <v-btn text="开始下载" @click="startDownload">开始下载</v-btn>
    </v-card-actions>
    <v-snackbar
      v-model="snackbar"
      :color="snackbarColor"
      :timeout="5000"
      top
    >
      {{ snackbarMessage }}
    </v-snackbar>
  </v-card>
</template>
