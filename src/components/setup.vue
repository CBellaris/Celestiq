<template>
  <div style="position: relative">
  <v-container fluid>
    <v-row>
      <v-col
        v-for="(workflow, index) in workflows"
        :key="index"
        cols="12" md="6" lg="4"
      >
        <v-card class="mx-auto react-card" max-width="400" style="background-color: rgba(0, 0, 0, 0.1);">
          <v-card-title>{{ workflow.name }}</v-card-title>
          <v-card-text style="opacity:0.7">{{ workflow.description }}</v-card-text>
          <v-card-actions>
            <v-spacer/>
            <v-btn color="myc_primary" @click="show_introduction(workflow.key)" v-if="workflow.ready">
              查看介绍
            </v-btn>
            <v-btn color="myc_primary" @click="add_to_startpage(workflow.key)" v-if="!workflow.ready">
              添加至主页
            </v-btn>
            <v-btn color="myc_primary" @click="selectWorkflow(workflow.key)" v-if="workflow.ready">
              配置
            </v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-row>
  </v-container>

  <v-scroll-y-transition>
    <v-card
      v-if="if_show_intro"
      style="position: absolute; top: 30%; left: 10%; right: 10%; overflow: hidden;"
    >
      <v-card-text style="overflow-y: auto; max-height: 600px;">
        <markdown_viewer :file-name="show_intro" />
      </v-card-text>
      <v-card-actions>
        <v-spacer/>
        <v-btn @click="if_show_intro = false" color="myc_primary">关闭</v-btn>
      </v-card-actions>
    </v-card>
  </v-scroll-y-transition>
  </div>
</template>

<script setup>
import { onMounted, reactive, ref} from "vue";

// 工作流介绍
import markdown_viewer from './markdown_viewer.vue'
const show_intro = ref('')
const if_show_intro = ref(false)
const show_introduction= (key)=>{
  show_intro.value = key
  if_show_intro.value = !if_show_intro.value
}

// 组件间事件通信
import {useEventStore} from '../stores/eventStore.js';
const eventStore = useEventStore();
const selectWorkflow = (key) => {
  eventStore.emitEvent('switch_page', key);
};

// 工作流配置管理
const workflows = reactive([]);
const load_workflows_setup = async () => {
  try {
    const get_workflows = await window.userConfig.get("Workflows_setup");
    get_workflows.content
      // .filter((workflow) => !workflow.ready) 这里暂时没有用到ready逻辑
      .forEach((workflow) => {
        workflows.push(reactive(workflow));
      });
    console.log("Loaded workflows_setup:", workflows);
  } catch (error) {
    console.error("Failed to load config:", error);
  }
};
const add_to_startpage = async (key)=>{
  // 将"Workflows_setup"中对应的工作流改为ready
  const target = workflows.find(workflow=>workflow.key===key)
  if(target){
    target.ready = true
  }
  window.userConfig.set("Workflows_setup", JSON.parse(JSON.stringify(workflows)))
  // 将"Workflows"中对应的工作流改为ready
  const key_ = key.replace("_setup", "")
  try {
    const get_workflows = await window.userConfig.get("Workflows");
    const workflows_ = get_workflows.content
    const target = workflows_.find(workflow=>workflow.key===key_)
    if(target){
      target.ready = true
    }
    window.userConfig.set("Workflows", workflows_)
  } catch (error) {
    console.error("Failed to load config:", error);
  }
}
onMounted(() => {
  load_workflows_setup();
});


</script>

<style scoped>
.react-card {
  transition: transform 0.2s;
}

.react-card:hover {
  transform: scale(1.02);
}
</style>
