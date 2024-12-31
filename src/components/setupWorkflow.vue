<script setup>
import {onMounted, onUnmounted, ref} from 'vue';

const props = defineProps({
  taskName: {
    type: String,
    required: true,
  },
});

const error = ref("");
const taskList = ref([])
const loadTaskList = async(fileName) => {
  try {
    const result = await window.MyAPI.readMarkdown(path);
    if (result.success) {
      taskList.value.push(result.content)
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
  window.taskProcessor.tasks.process('tasks.json')
}

onMounted(() => {
    loadTaskList(`${props.taskName}.json`)
    window.taskProcessor.events.on('taskProgress', (data) => console.log(`from vue: ${JSON.stringify(data)}`));
    window.taskProcessor.events.on('commandOutput', (data) => console.log(`from vue: ${JSON.stringify(data)}`));
    window.taskProcessor.events.on('taskUpdate', (data) => console.log(`from vue: ${JSON.stringify(data)}`));
    window.taskProcessor.events.on('allTasksCompleted', (data) => console.log(`from vue: ${JSON.stringify(data)}`));
});

onUnmounted(() => {
    window.taskProcessor.events.off('taskUpdate', () => {});
    window.taskProcessor.events.off('commandOutput', () => {});
    window.taskProcessor.events.off('taskProgress', () => {});
    window.taskProcessor.events.off('allTasksCompleted', () => {});
});
</script>

<template>
  <p>{{JSON.stringify(taskList)}}</p>
  <div>
    <v-col cols="auto">
      <v-btn icon="mdi-plus" size="x-large" @click="runTask"></v-btn>
    </v-col>
    <v-expansion-panels class="my-4 stepsOfSetup" variant="popout">
      <v-expansion-panel
        text="处理结果"
        title="执行任务：task1"
      ></v-expansion-panel>
    </v-expansion-panels>
  </div>
</template>

<style scoped lang="scss">
.stepsOfSetup{
  padding-inline: 30px;
}
</style>
