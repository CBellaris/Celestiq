<template>
<v-expansion-panels
  variant="popout"
  class="mt-2 mb-2"
  style="padding-inline: 3%"
>
  <v-expansion-panel
    :color="getPanelColor(props.task)"
    eager
  >
    <v-expansion-panel-title>
      <div class="d-flex align-center justify-space-between" style="width: 95%">
        <div>
          {{props.task.description}}
        </div>
        <div>
          <div v-if="!(props.task.state==='running')">
            <v-tooltip text="执行" location="start">
              <template v-slot:activator="{ props }">
                <v-btn v-bind="props" @click="start_cmd" class="mr-3" icon="mdi-play"></v-btn>
              </template>
            </v-tooltip>
          </div>
          <div v-else-if="props.task.state==='running'">
            <v-tooltip text="终止" location="start">
              <template v-slot:activator="{ props }">
                <v-btn v-bind="props" @click="stop_cmd" class="mr-3" icon="mdi-stop-circle-outline"></v-btn>
              </template>
            </v-tooltip>
          </div>
        </div>
      </div>
    </v-expansion-panel-title>
    <v-expansion-panel-text>
      <run_cmd
        :command="props.task.command"
        :description="props.task.description"
        ref="childRef"
        @finish="handleFinish"
        @failure="handleFailure"
        :cardStyle="'max-height: 300px;'"
        v-model:output="output"
      ></run_cmd>
    </v-expansion-panel-text>
  </v-expansion-panel>
</v-expansion-panels>
</template>

<script setup>
/* eslint-disable vue/no-mutating-props */
import run_cmd from './run_cmd.vue';
import {ref, watch} from 'vue'

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
//   output: ''
// }

const output = ref("")
watch(output, (newValue) => {
  props.task.output = newValue;
});

const getPanelColor = (task) => {
  if (task.state === 'success') {
    return "myc_success"; // 成功：鸭绿
  } else if (task.state === 'running') {
    return "myc_primary"; // 主色: 青蓝
  } else if (task.state === 'error'){
    return "myc_error"  // 错误：深橘
  } else if (task.state === 'pending'){
    return "myc_pending"; // 等待中：浅灰背景，深灰文字
  }
};

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
    console.log(`任务 ${command} 成功完成`);
    props.task.state = 'success'
  }else{
    console.log(`任务 ${command} 未完成`);
    props.task.state = 'pending'
  }
};

const handleFailure = (command) => {
  console.log(`任务 ${command} 失败`);
  props.task.state = 'error'
};
/* eslint-enable vue/no-mutating-props */
</script>



