<template>
  <!-- v-sheet 用于包含背景图片 -->
    <v-sheet
      height="200"
      class="d-flex align-center justify-start ma-6 rounded"
      style="position: relative; overflow: hidden;"
    >
      <v-img
        src="local://public/welcome_1.png"
        cover
        style="position: absolute; top: 0; left: 0; width: 100%; height: 100%; z-index: 1"
        class="opacity-80"
      >

      </v-img>
      <!-- 欢迎文本 -->
      <div style="z-index: 2">
        <h5 style="text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3); color: #e6e6e6" class="ml-10">Celestiq</h5>
        <h2 style="text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3); color: #e6e6e6" class="ml-10">AI工具启动器</h2>
        <h3 style="text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3); color: #e6e6e6" class="ml-10">欢迎使用</h3>
      </div>
    </v-sheet>

  <v-container fluid>
    <v-card v-if="workflows.length === 0" class="mx-auto ms-2 pa-2" style="background-color: rgba(0, 0, 0, 0.1);">
      <v-card-title>还没有工作流</v-card-title>
      <v-card-subtitle>前往配置页面添加并配置！</v-card-subtitle>
    </v-card>
    <v-row>
      <v-col
        v-for="(workflow, index) in workflows"
        :key="index"
        cols="12" md="6" lg="4"
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
    </v-row>
  </v-container>
</template>

<script setup>
import { onMounted, reactive, ref} from "vue";

const workflows = reactive([]);

// 组件间事件通信
import {useEventStore} from '../stores/eventStore.js';
const eventStore = useEventStore();

const load_workflows = async () => {
  try {
    const get_workflows = await window.userConfig.get("Workflows");
    get_workflows.content
      .filter((workflow) => workflow.ready)
      .forEach((workflow) => {
        workflows.push(reactive(workflow));
      });
    console.log("Loaded workflows:", workflows);
  } catch (error) {
    console.error("Failed to load config:", error);
  }
};

const selectWorkflow = (key) => {
  eventStore.emitEvent('switch_page', key);
};

onMounted(() => {
  load_workflows();
});
</script>

<style scoped>
.v-card {
  transition: transform 0.2s;
}

.v-card:hover {
  transform: scale(1.02);
}
</style>
