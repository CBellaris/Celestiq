<template>
  <v-container>
    <v-card class="pa-3 mb-2" outlined>
      <v-card-title>Nvidia依赖</v-card-title>
      <v-card-text>这个工作流需要安装CUDA，请前往设置页面点击“Nvidia依赖检测”</v-card-text>
    </v-card>

<!--    <v-card class="pa-3 mb-2" outlined>-->
<!--      <v-card-title>python环境依赖</v-card-title>-->
<!--      <v-card-text>这个工作流需要python环境，请前往设置页面点击“python安装”</v-card-text>-->
<!--    </v-card>-->

    <v-card class="pa-3 mb-2" outlined>
      <v-card-title>安装llama</v-card-title>
      <div v-for="(task, index) in task_list" :key="index">
        <run_cmd_unit v-model:task="task_list[index]"></run_cmd_unit>
      </div>
    </v-card>

    <v-card class="pa-3 mb-2" outlined>
      <v-card-title>下载模型</v-card-title>
      <v-select
        v-model="selected_description"
        label="选择配置"
        :items="descriptions"
        style="max-width: 300px"
        class="ml-4"
      ></v-select>
      <div v-if="selected_model_list">
        <div v-for="(task, index) in selected_model_list" :key="index">
          <download_ms v-model:task="selected_model_list[index]"></download_ms>
        </div>
      </div>
    </v-card>
    <v-card class="pa-3 mb-2" outlined v-if="all_set">
      <v-card-title>配置成功！</v-card-title>
      <v-card-subtitle>从左侧列表选择工作流并继续</v-card-subtitle>
    </v-card>
  </v-container>
  <v-snackbar
      v-model="snackbar"
      :color="snackbarColor"
      :timeout="5000"
      top
    >
      {{ snackbarMessage }}
    </v-snackbar>
</template>

<script setup>
import download_ms from "../components/download_ms.vue"
import run_cmd_unit from "../components/run_cmd_unit.vue"
import {computed, onMounted, reactive, ref, watch} from "vue"

import {useEventStore} from '../stores/eventStore.js';
const eventStore = useEventStore();
// 定义发送配置完成事件
const sendEvent = () => {
  eventStore.emitEvent('setup_finish', {task: 'Qwen2VL'});
};

// 读入模型权重配置
const config_model = reactive([])
const descriptions = reactive([])
const load_config_model = async()=>{
  try {
    const get_config = await window.userConfig.get("Qwen2VL_gguf_model");
    if (!get_config.success){
      snackbar.value = true
      snackbarColor.value = 'error'
      snackbarMessage.value = '你可能没有下载配置文件！请更新软件'
      return
    }
    get_config.content.forEach(option=>{
      descriptions.push(option.description)
      config_model.push(reactive(option))
    })
    console.log("Loaded:", config_model);
  } catch (error) {
    console.error("Failed to load config:", error);
  }
}

// 选择模型
const selected_description = ref('')
const selectedIndex = computed(() => {
  return descriptions.indexOf(selected_description.value);
});
let selected_model_list = null
watch(selectedIndex, (newVal)=>{
  selected_model_list = config_model[newVal].model_list
  console.log("selected_model_list", selected_model_list)
})

watch(()=>config_model,()=>{
  window.userConfig.set('Qwen2VL_gguf_model', JSON.parse(JSON.stringify(config_model)))
}, {deep:true})




// 读入设置配置
// 写入配置文件
const task_list = reactive([])
watch(()=>task_list,()=>{
  window.userConfig.set('Qwen2VL_gguf_setup', JSON.parse(JSON.stringify(task_list)))
}, {deep:true})

const all_set = ref(false)

const snackbar = ref(false);
const snackbarMessage = ref("");
const snackbarColor = ref("");

const load_config = async()=>{
  try {
    const get_task = await window.userConfig.get("Qwen2VL_gguf_setup");
    if (!get_task.success){
      snackbar.value = true
      snackbarColor.value = 'error'
      snackbarMessage.value = '你可能没有下载配置文件！请更新软件'
      return
    }
    const tasks = get_task.content
    tasks.forEach(task=>{
      task_list.push(reactive(task))
    })
  } catch (error) {
    console.error("Failed to load config:", error);
  }
}

onMounted(()=>{
  load_config()
  load_config_model()
})



</script>

<style scoped>

</style>
