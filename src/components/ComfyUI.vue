<template>
  <!-- v-sheet 用于包含背景图片 -->
  <v-sheet
    height="220"
    class="d-flex align-center justify-start ma-6 rounded"
    style="position: relative; overflow: hidden;"
    v-if="!running"
  >
    <v-img
      src="local://public/ComfyUI.png"
      cover
      style="position: absolute; top: 0; left: 0; width: 100%; height: 100%; z-index: 1"
      class="opacity-80"
    >
    </v-img>
    <!-- 欢迎文本 -->
    <div style="z-index: 2">
      <h5 style="text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3); color: #e6e6e6" class="ml-10">ComfyUI</h5>
      <h2 style="text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3); color: #e6e6e6" class="ml-10">Celestiq-启动器</h2>
      <h3 style="text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3); color: #e6e6e6" class="ml-10">以工作流为单位部署，更简洁、更易用！</h3>
    </div>
  </v-sheet>

  <v-card
    height="220"
    class="ma-6 rounded"
    v-if="running"
  >
    <v-card-title>
      ComfyUI运行中
      <v-spacer/>
      <v-btn @click="stop_running">停止运行</v-btn>
    </v-card-title>
    <v-card-text>
      <run_cmd
        command="pyEnv\python.exe ComfyUI\main.py"
        :auto-run="true"
        ref="childRef"
      ></run_cmd>
    </v-card-text>
  </v-card>

  <v-row style="height: 50%">
    <v-col cols="8">
      <v-col
        v-for="(workflow, index) in workflows"
        :key="index"
        cols="8" md="4" lg="2"
      >
        <v-card class="mx-auto" max-width="400" style="background-color: rgba(0, 0, 0, 0.1);">
          <v-card-title>{{ workflow.name }}</v-card-title>
          <v-card-text style="opacity:0.7">{{ workflow.description }}</v-card-text>
          <v-card-actions>
            <v-spacer/>
            <v-btn color="myc_primary" @click="selectWorkflow(workflow.key)">
              使用
            </v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-col>
    <v-col cols="4">
      <v-card style="background-color: rgba(0,0,0,0);" class="mr-4" >
        <v-card-title>文件夹</v-card-title>
        <v-card-text style="max-height: 300px; overflow-y:auto">
          <v-list nav selectable style="background-color: rgba(0,0,0,0);" >
            <v-list-item title="根目录" subtitle="." prepend-icon="mdi-folder-home-outline" append-icon="mdi-chevron-right" class="mb-2"></v-list-item>
            <v-list-item title="工作流" subtitle="./user/default/workflows" prepend-icon="mdi-sitemap" append-icon="mdi-chevron-right" class="mb-2"></v-list-item>
            <v-list-item title="输入图片" subtitle="./input" prepend-icon="mdi-image-plus-outline" append-icon="mdi-chevron-right" class="mb-2"></v-list-item>
            <v-list-item title="输出图片" subtitle="./output" prepend-icon="mdi-image-multiple-outline" append-icon="mdi-chevron-right" class="mb-2"></v-list-item>
            <v-list-item title="模型" subtitle="./models" prepend-icon="mdi-matrix" append-icon="mdi-chevron-right" class="mb-2"></v-list-item>
            <v-list-item title="插件" subtitle="./custom_nodes" prepend-icon="mdi-power-plug-outline" append-icon="mdi-chevron-right" class="mb-2"></v-list-item>
          </v-list>
        </v-card-text>
      </v-card>
    </v-col>
  </v-row>

  <v-row class="d-flex align-baseline">
    <v-col cols="8" class="d-flex align-center opacity-50">
      <div style="display: inline-block;">
        <h6 class="ml-10">启动器版本:</h6>
        <h6 class="ml-10">ComfyUI版本:</h6>
      </div>
      <div style="display: inline-block;max-width: 30%">
        <h6 class="ml-10">v0.1.0</h6>
        <h6 class="ml-10" style="white-space: nowrap; overflow: hidden; text-overflow: ellipsis;">caa6476a, 3 weeks ago : Update web content to release v1.6.17 (#6337)</h6>
      </div>
    </v-col>
    <v-col cols="4" class="d-flex justify-center">
      <v-sheet elevation="0" rounded
               class="bg-myc_primary mr-4 d-flex align-center justify-center"
               height="60px" width="220px"
               style="cursor: pointer;"
               @click="start_ComfyUI"
      >
        <v-icon icon="mdi-play" class="mr-4"></v-icon>
        <h3>启动ComfyUI</h3>
      </v-sheet>
    </v-col>
  </v-row>
</template>

<script>
export default {
  name: 'ComfyUI'
}
</script>

<script setup>
import {reactive, ref} from "vue";
import run_cmd from "./run_cmd.vue"

// 工作流配置管理
const workflows = reactive([]);
const load_workflows_setup = async () => {
  try {
    const get_workflows = await window.userConfig.get("Workflows_setup");
    get_workflows.content
      .forEach((workflow) => {
        workflows.push(reactive(workflow));
      });
    console.log("Loaded workflows_setup:", workflows);
  } catch (error) {
    console.error("Failed to load config:", error);
  }
};

// 启动ComfyUI
const childRef = ref(null); // 获取子组件实例
const running = ref(false)
const start_ComfyUI = ()=>{
  running.value = true
}
const stop_running = ()=>{
  childRef.value?.stopCommand()
}

</script>



<style scoped>
</style>
