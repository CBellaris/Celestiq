<template>
  <!-- 主内容区域 -->
  <v-main min-width="512" :style="mainGradientStyle" class="d-flex justify-center">
    <v-card elevation="0" class="ma-4" style="background-color: rgba(0,0,0,0)">
      <v-card-title>安装(1/1)</v-card-title>
      <v-card-subtitle>python依赖</v-card-subtitle>
      <v-card-text opacity="0.7">
        <p class="pb-2">本软件大部分功能依赖python环境运行，如果你不了解如何管理python环境，强烈建议在此页面安装提前打包好的python环境。你可以暂时跳过，并随时在设置页面管理python依赖</p>
        <v-select
        v-model="selected_option"
        label="选择配置"
        :items="options"
        style="max-width: 300px"
        class="ml-4"
      ></v-select>
      <v-divider/>
      <template v-if="is_installing">
        <div v-for="(cmd, index) in running_cmd_list" :key="index">
          <run_cmd :command="cmd.command" :description="cmd.description" :auto-run="true" @finish="handleFinish" @failure="handleFailure"></run_cmd>
        </div>
      </template>
      </v-card-text>

      <v-card-actions v-if="!is_installing">
        <v-spacer/>
        <v-btn color="myc_pending" @click="finishSetup">跳过</v-btn>
        <v-btn color="myc_primary" @click="install">安装</v-btn>
      </v-card-actions>
    </v-card>
    <div style="position:fixed; right:30px; bottom:30px; z-index:50">
      <v-tooltip text="开始使用" location="start">
        <template v-slot:activator="{ props }">
          <v-btn v-bind="props" icon="mdi-chevron-right" size="x-large" @click="finishSetup" color="myc_primary" v-if="all_finished"></v-btn>
        </template>
      </v-tooltip>
    </div>
    <v-snackbar
      v-model="snackbar"
      :color="snackbarColor"
      :timeout="5000"
      top
    >
      {{ snackbarMessage }}
    </v-snackbar>
  </v-main>
</template>

<script setup>
import { useRouter } from 'vue-router';
import {computed, reactive, ref, watch} from "vue";
import run_cmd from '../components/run_cmd.vue'

// 选择配置
const options = ['基础环境，仅常见下载工具(huggingface, modelscope)']
const selected_option = ref('')
const selected_index = computed(() => {
  return options.indexOf(selected_option.value);
});
// 监听选中的值变化
watch(selected_index, (newVal) => {
  console.log('selected_option', newVal)
});

const snackbar = ref(false); // 弹窗组件
const snackbarMessage = ref("");
const snackbarColor = ref("");
// 安装环境
const is_installing = ref(false)
const env_list = ['pyEnv']
const env_name = computed(()=>{
  return env_list[selected_index.value]
})
const full_cmd_list = computed(()=>{
  return [
  {
    command:`tar -xzf outSrc/${env_name.value}.tar.gz -C pyEnv`,
    description: "解压"
  },
  {
    command:`${env_name.value}\\Scripts\\conda-unpack`,
    description: "处理环境变量"
  }
]
})
// 开始安装
const running_cmd_list = ref([])
const running_index = ref(0)
const install = ()=>{
  if(is_installing.value) {
    return;
  }
  if(!selected_option.value){
    snackbar.value = true
    snackbarMessage.value = "请先选择一个配置"
    snackbarColor.value = 'myc_primary'
    return;
  }

  running_cmd_list.value.push(full_cmd_list.value[running_index.value])
  is_installing.value = true
}
// 完成一个任务后进行下一个
const all_finished = ref(false)
const handleFinish = (command, code) => {
  if ([0].includes(code)){
    console.log("full_cmd_list length", full_cmd_list.value.length)
    if(running_index.value === (full_cmd_list.value.length-1)){
      all_finished.value = true
      return
    }
    console.log("running_index", running_index.value)
    running_index.value += 1
    running_cmd_list.value.push(full_cmd_list.value[running_index.value])
    console.log("running_cmd_list", running_cmd_list.value)
  }else{
    snackbar.value = true
    snackbarMessage.value = `任务 ${command} 未完成，请复制命令输出并反馈`
    snackbarColor.value = 'myc_pending'
  }
};
const handleFailure = (command) => {
  snackbar.value = true
  snackbarMessage.value = `任务 ${command} 失败，请复制命令输出并反馈`
  snackbarColor.value = 'myc_error'
};

watch(all_finished, (value)=>{
  if(value){
    snackbar.value = true
  snackbarMessage.value = "配置完成！"
  snackbarColor.value = 'myc_success'
  }
})

import { useTheme } from 'vuetify';
// 获取 Vuetify 主题
const theme = useTheme();
// 动态计算主内容区域的背景渐变样式
const mainGradientStyle = computed(() => {
  const colors = (theme.global.current.value.dark
    ? 'rgba(0, 0, 0, 0.9)' // 深色主题下的颜色
    : 'rgba(255, 255, 255, 0.9)'); // 浅色主题下的颜色
  return `backdrop-filter: blur(10px); background-color: ${colors}`
});

const router = useRouter();
// 用户完成初始化设置后
const finishSetup = () => {
  window.userConfig.set('is_first_launch', false)
  // 跳转到首页
  router.push('/');
};
</script>

<style scoped>

</style>
