<script setup>
import workflowIntroduce from '../components/workflowIntroduce.vue'
import setupWorkflow from '../components/setupWorkflow.vue'
import {ref, onMounted, watch} from "vue"

const isWorkflow = async ()=>{
  const workFlow = await window.userConfig.get('existWorkflow')
  return (workFlow.content.length === 0)
}
const loadSelectacleWorkflow = async()=>{
  const selectableWorkflow = await window.userConfig.get('selectableWorkflow')
}

const isWorkflowEmpty = ref(true)
onMounted(() => {
  isWorkflowEmpty.value = isWorkflow()
})

const showSelectableWorkflow = ref(false)
const selectedSetupWorkflow = ref([]) // 响应式变量存储用户选择
const showWorkflowIntroduce = ref(false)
const isStartSetup = ref(false)

// 监听 selectedWorkflow 的变化
watch(selectedSetupWorkflow, (newValue) => {
  if(selectedSetupWorkflow.value.length>0){
    console.log('用户选择了:', newValue[0])
    showSelectableWorkflow.value = !showSelectableWorkflow.value
    showWorkflowIntroduce.value = true
  }
});
</script>

<template>
  <v-layout class="rounded rounded-md">
    <v-navigation-drawer permanent>
      <v-list>
        <v-list-item :title="isWorkflowEmpty? '点击配置工作流':'选择一个工作流'"></v-list-item>
      </v-list>

      <v-divider></v-divider>

      <v-list density="compact" nav selectable>
        <v-list-item prepend-icon="mdi-plus" title="新建" v-if="isWorkflowEmpty" @click="showSelectableWorkflow = !showSelectableWorkflow"></v-list-item>
        <v-list-item title="Workflow1" value="W1"></v-list-item>
        <v-list-item title="Workflow2" value="W2"></v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-navigation-drawer v-model="showSelectableWorkflow" temporary>
      <v-list density="compact" nav selectable v-model:selected="selectedSetupWorkflow" select-strategy="single-leaf">
        <v-list-item title="Workflow1" value="Task1"></v-list-item>
        <v-list-item title="Workflow2" value="W2"></v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-main min-width="512">
      <div v-if="showWorkflowIntroduce">
        <workflowIntroduce :taskName="selectedSetupWorkflow[0]" @startSetup="isStartSetup=!isStartSetup; showWorkflowIntroduce=!showWorkflowIntroduce"/>
      </div>
      <div v-else-if="isStartSetup">
        <setupWorkflow :taskName="selectedSetupWorkflow[0]"></setupWorkflow>
      </div>
      <div v-else>
        <a>你好！欢迎使用</a>
      </div>
    </v-main>
  </v-layout>
</template>

<style lang="scss">
</style>
