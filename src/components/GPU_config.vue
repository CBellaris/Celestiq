<template>
  <div>
    <div class="pb-4">
      <run_cmd_unit v-model:task="GPU_Driver"></run_cmd_unit>
      <run_cmd_unit v-model:task="GPU_CUDA"></run_cmd_unit>
    </div>
    <div style="padding-inline: 3%">
      <v-col v-if="GPU_Driver.state==='success'">
        <v-card variant="tonal" class="mb-2">
          <v-card-item>
            <v-card-title>Nvidia Driver</v-card-title>
            <v-card-subtitle>GPU驱动已安装</v-card-subtitle>
          </v-card-item>
          <v-card-text>
            你的显卡驱动版本为：{{ GPU_Info.driver }}，支持的最高CUDA版本为：{{ GPU_Info.CUDA_Max }}
          </v-card-text>
        </v-card>
      </v-col>

      <v-col>
        <v-card variant="tonal" class="mb-2" v-if="GPU_CUDA.state==='success'">
          <v-card-item>
            <v-card-title>CUDA toolkit</v-card-title>
            <v-card-subtitle>CUDA已安装</v-card-subtitle>
          </v-card-item>
          <v-card-text>
            当前安装的CUDA版本为：{{ GPU_Info.CUDA }}
          </v-card-text>
        </v-card>
        <v-card v-else-if="['error', 'pending'].includes(GPU_CUDA.state)" variant="tonal" class="mb-2">
          <v-card-item>
            <v-card-title>CUDA toolkit</v-card-title>
            <v-card-subtitle>CUDA未安装</v-card-subtitle>
          </v-card-item>
          <v-card-text>
            请安装你的驱动所支持的最高版本CUDA
          </v-card-text>
          <v-card-actions>
            <v-btn text="查看教程" @click="showTutorial = !showTutorial;" />
          </v-card-actions>
        </v-card>
      </v-col>
    </div>
    <div v-if="showTutorial" class="pl-3">
      <markdown_viewer file-name="CUDATutorial" />
    </div>
  </div>
</template>

<script setup>
import run_cmd_unit from "./run_cmd_unit.vue"
import markdown_viewer from "./markdown_viewer.vue"
import {ref, watch, onMounted, reactive} from 'vue'

const GPU_Driver = reactive({
  command: '',
  description: '',
  state: '',
  output: ''
})
const GPU_CUDA = reactive({
  command: '',
  description: '',
  state: '',
  output: ''
})
const showTutorial = ref(false)
const GPU_Info = reactive({
  driver: "",
  CUDA_Max: "",
  CUDA:""
})

const load_config = async()=>{
  try {
    const getGPU_task = (await window.userConfig.get("GPU_config")) || {};
    const GPU_task = getGPU_task.content
    Object.assign(GPU_Driver,GPU_task.GPU_Driver || {});
    Object.assign(GPU_CUDA, GPU_task.GPU_CUDA || {});
  } catch (error) {
    console.error("Failed to load GPU info:", error);
  }
}

const getGPUDriverInfo = task =>{
  // 正则表达式匹配CUDA和Driver版本号
    const cudaMaxRegex = /CUDA Version: (\d+(\.\d+)?)/;
    const driverRegex = /Driver Version: (\d+(\.\d+)?)/;
    const cudaInstalledregex = /release\s+\d+\.\d+,\s*V(\d+\.\d+\.\d+)/;

    // 匹配结果
    const cudaMaxMatch = task.match(cudaMaxRegex);
    const driverMatch = task.match(driverRegex);
    const cudaInstalledMatch = task.match(cudaInstalledregex);
    // 构造返回对象
    const versionInfo = {};

    if (cudaMaxMatch && cudaMaxMatch[1]) {
        versionInfo.cudaMaxVersion = cudaMaxMatch[1];
    }
    if (driverMatch && driverMatch[1]) {
        versionInfo.driverVersion = driverMatch[1];
    }
    if (cudaInstalledMatch && cudaInstalledMatch[1]) {
        versionInfo.cudaInstalledVersion = cudaInstalledMatch[1];
    }

    return versionInfo;
}

watch(GPU_Driver, ()=>{
  GPU_Info.driver = getGPUDriverInfo(GPU_Driver.output).driverVersion
  GPU_Info.CUDA_Max = getGPUDriverInfo(GPU_Driver.output).cudaMaxVersion
  const payload = {
    GPU_Driver: {...GPU_Driver},
    GPU_CUDA: {...GPU_CUDA}
  }
  window.userConfig.set("GPU_config", payload)
})
watch(GPU_CUDA, ()=>{
  GPU_Info.CUDA = getGPUDriverInfo(GPU_CUDA.output).cudaInstalledVersion
  const payload = {
    GPU_Driver: {...GPU_Driver},
    GPU_CUDA: {...GPU_CUDA}
  }
  window.userConfig.set("GPU_config", payload)
})

onMounted(()=>{
  load_config()
})
</script>

<style scoped>

</style>
