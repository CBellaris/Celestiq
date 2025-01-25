<script setup>
import {onMounted, onUnmounted, ref} from 'vue';

const props = defineProps({
  taskName: {
    type: String,
    required: true,
  },
  taskRes:{
    type: Object
  },
  allCompleted:{
    type: Boolean,
    default: false
  }
});

const emit = defineEmits(['update:taskRes', 'update:allCompleted']);

const error = ref("");
const taskList = ref([])
const isStartBtn = ref(false)

// 组件挂载时加载所有任务，为任务列表添加正在处理、已处理等状态
const loadTaskList = async(fileName) => {
  try {
    const result = await window.MyAPI.readJson(fileName);
    if (result.success) {
      taskList.value.push(...result.content)

      // 如果存在没有完成的任务，显示开始按钮
      let all_completed = true
      result.content.forEach(task =>{
        if(!task.completed){
          isStartBtn.value = true
          all_completed = false
        }
      })
      if(all_completed) emit('update:allCompleted', true)

      taskList.value.forEach(task => {
        if (typeof task === 'object' && task !== null) { // 检查 task 是否为对象
            task.processing = false;
            task.stdout = '';
            task.processed = false
        }
      });
      error.value = "";
    } else {
      throw new Error(result.error);
    }
  } catch (err) {
    console.error("Error loading taskFile:", err);
    error.value = "Failed to load task file.";
  }
}

const runTask = ()=>{
  window.taskProcessor.tasks.process(`${props.taskName}.json`)
  isStartBtn.value = false
}

const getPanelClass = (task) => {
  if (task.completed) {
    return "bg-success text-white"; // 成功：绿色背景，白色文字
  } else if (task.processing) {
    return "bg-primary text-white"; // 处理中：主色背景，白色文字
  } else if (task.processed){
    return "bg-error text-white"
  } else {
    return "bg-grey-lighten-2 text-grey-darken-3"; // 等待中：浅灰背景，深灰文字
  }
};


onMounted(() => {
    loadTaskList(`${props.taskName}.json`)
    window.taskProcessor.events.on('taskProgress', (data) => {
      // console.log(`from vue taskProgress: ${data}`)
      taskList.value.forEach(task => {
        if (typeof task === 'object' && task !== null && task.description === data) { // 检查 task 是否为对象
            task.processing = true;
        }
      });
    });
    window.taskProcessor.events.on('commandOutput', (data) => {
      // console.log(`from vue commandOutput: ${JSON.stringify(data)}`)
      // 定义换行符的正则表达式，以支持跨平台
      const newlineRegex = /\r\n|\r|\n/g;

      taskList.value.forEach(task => {
          if (typeof task === 'object' && task !== null && task.processing === true) { // 检查 task 是否为对象
              // 更新任务的标准输出
              task.stdout += data;

              // 使用正则表达式按行分割，并保留最新的5行
              const lines = task.stdout.split(newlineRegex);
              if (lines.length > 5) {
                  task.stdout = lines.slice(-5).join('\n'); // 只保留最新的5行
              }
          }
      });
    });
    window.taskProcessor.events.on('taskUpdate', (data) => {
      // console.log(`from vue taskUpdate: ${JSON.stringify(data)}`)
      emit('update:taskRes', data)
      taskList.value.forEach(task => {
        if (typeof task === 'object' && task !== null && task.processing === true && task.description === data.description) { // 检查 task 是否为对象
            if (data.succeed){
              task.completed = true;
            }
            task.processing = false;
            task.processed = true
        }
      });
    });
    window.taskProcessor.events.on('allTasksCompleted', (taskName, ...args) => {
      if(taskName.split('.')[0] === props.taskName){
        emit('update:allCompleted', true)
      }
    });

    window.taskProcessor.events.on('error', (data) => {
      console.log(`from vue error: ${JSON.stringify(data)}`)
      isStartBtn.value = true
    });
});

onUnmounted(() => {
    window.taskProcessor.events.off('taskUpdate', () => {});
    window.taskProcessor.events.off('commandOutput', () => {});
    window.taskProcessor.events.off('taskProgress', () => {});
    window.taskProcessor.events.off('allTasksCompleted', () => {});
    window.taskProcessor.events.off('error', () => {});
});
</script>

<template>
  <div>
    <!-- 使用 Flexbox 包裹 v-expansion-panels 和按钮 -->
    <div class="d-flex align-center" style="width: 100%">
      <!-- v-expansion-panels -->
      <v-expansion-panels
        class="my-4"
        variant="popout"
        style="flex:1; padding-inline: 5%"
      >
        <v-expansion-panel
          v-for="task in taskList"
          :title="task.descriptionCN"
          :text="task.stdout"
          :key="task.description"
          :class="getPanelClass(task)"
        >
          <v-expansion-panel-text>
            <v-tooltip text="标记为未完成(注意：有些任务重复执行可能出现意料外的问题)" location="bottom">
              <template v-slot:activator="{ props }">
                <v-btn
                  v-bind="props"
                  class="mr-2"
                  icon="mdi-arrow-u-right-top"
                  size="small"
                  @click="task.completed=false; isStartBtn=true"
                ></v-btn>
              </template>
            </v-tooltip>
            <v-progress-circular
              color="primary"
              indeterminate
              v-if="task.processing"
            ></v-progress-circular>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>

      <!-- 按钮 -->
      <div class="d-flex align-center justify-center" style="padding-right: 5%" v-if="isStartBtn">
        <v-btn
          icon="mdi-cog-play-outline"
          size="x-large"
          @click="runTask"
        ></v-btn>
      </div>
    </div>
  </div>

</template>

<style scoped lang="scss">
.v-progress-circular {
  margin: 1rem;
}
</style>
