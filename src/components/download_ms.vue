<template>
  <v-card class="pa-3">
    <v-card-title>{{ props.task.description }}</v-card-title>
    <v-card-text>
      <div>{{ props.task.state }}</div>
      <div>进度: {{ props.task.progress }}%</div>
      <v-progress-linear
        :model-value="props.task.progress"
        rounded
        bg-color="blue-grey"
        color="secondary"
      ></v-progress-linear>
      <div>
        <p>已下载: {{ props.task.downloaded }} / {{ props.task.totalSize }}</p>
        <p>速度: {{ props.task.speed }}</p>
        <p>剩余时间: {{ props.task.remainingTime }}</p>
      </div>
    </v-card-text>
    <v-card-actions>
      <v-btn text="开始下载" @click="start_cmd"></v-btn>
      <v-btn text="暂停下载" @click="stop_cmd"></v-btn>
      <v-btn text="查看完整输出" @click="show_cmd_output = !show_cmd_output"></v-btn>
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
  <run_cmd :command="props.task.command"
           v-model:output="output"
           v-model:output_stream="output_stream"
           ref="childRef"
           @finish="handleFinish"
           @failure="handleFailure"
           :card-style="'max-height: 300px;'"
           v-show="show_cmd_output"
  ></run_cmd>
</template>

<script setup>
/* eslint-disable vue/no-mutating-props */
import run_cmd from './run_cmd.vue';
import {reactive, ref, watch} from 'vue'

const show_cmd_output = ref(false)

// 获取子组件实例
const childRef = ref(null);

const props = defineProps({
  task: {
    type: Object,
    required: true
  }
})
// task should be: {
//   command: 'nvidia-smi',
//   description: '显卡驱动',
//   state: 'pending',
//   output: '',
//   progress: 0,
// 	 downloaded: "0B",
// 	 totalSize: "0B",
// 	 remainingTime: "00:00",
// 	 speed: "0B/s"
// }


const output = ref("")
const output_stream = ref("")
watch(output_stream, (newVal)=>{
  let download_info = {
      progress: 0,
      downloaded:'',
      totalSize:'',
      remainingTime:'',
      speed:''
    };
    try{
      download_info = parseDownload_ms(newVal)
    }catch(err) {
      console.log('Error at parsing output: ', err.message)
    }
    // 更新当前任务的进度信息
    props.task.progress = parseFloat(download_info.progress) || props.task.progress;
    props.task.downloaded = download_info.downloaded || props.task.downloaded;
    props.task.totalSize = download_info.totalSize || props.task.totalSize;
    props.task.remainingTime = download_info.remainingTime || props.task.remainingTime;
    props.task.speed = download_info.speed || props.task.speed;
})

const snackbar = ref(false);
const snackbarMessage = ref("");
const snackbarColor = ref("");

const start_cmd = ()=>{
  props.task.state = 'running'
  childRef.value?.startCommand()
}

const stop_cmd = ()=>{
  props.task.state = 'pending'
  childRef.value?.stopCommand()
}

const handleFinish = (command, code) => {
  if ([0, 3].includes(code)){
    snackbar.value = true
    snackbarColor.value = 'success'
    snackbarMessage.value = `下载结束: ${props.task.description}`
    props.task.state = 'success'
  }else{
    snackbar.value = true
    snackbarColor.value = 'primary'
    snackbarMessage.value = `下载未完成: ${props.task.description}`
    props.task.state = 'pending'
  }
};

const handleFailure = () => {
  snackbar.value = false
  snackbarColor.value = 'error'
  snackbarMessage.value = `下载失败: ${props.task.description}，请查看完整任务日志`
  props.task.state = 'error'
};

function parseDownload_ms(log) {
  const result = {
    progress:'',
    downloaded:'',
    totalSize:'',
    remainingTime:'',
    speed:''
  };

  const log_ = log.replace('Std wrong:', '')

  // 按照冒号分割，提取主要部分
  const parts = log_.split(':');
  if (parts.length > 2) {
    // 提取进度条部分和速度部分
    const progressPart = parts[1].trim();
    const lastPart = parts[3] || ''
    const speedPart = parts[2].trim()+':'+lastPart.trim();

    // 获取下载进度百分比
    const percentIndex = progressPart.indexOf('%');
    if (percentIndex !== -1) {
      result.progress = progressPart.slice(0, percentIndex+1);
    }

    // 获取文件大小和已下载数据
    const sizeInfo = progressPart.split('|')[2].trim();
    let [downloaded, totalSize] = sizeInfo.split('/').map(size => size.trim());
    totalSize = totalSize.split('[')[0].trim()
    result.downloaded = downloaded;
    result.totalSize = totalSize;

    // 获取剩余时间和速度
    const speedParts = speedPart.split(',');
    if (speedParts.length === 2) {
      const remainingTime = speedParts[0].split('<')[1].trim();
      let speed = speedParts[1].trim();
      speed = speed.replace(']', '')

      result.remainingTime = remainingTime;
      result.speed = speed;
    }
  }

  return result;
}
/* eslint-enable vue/no-mutating-props */
</script>



<style scoped>

</style>
