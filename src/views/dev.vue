<template>
<v-main>
  <run_cmd_unit v-model:task="test"></run_cmd_unit>
</v-main>
</template>

<script setup>
import run_cmd_unit from '../components/run_cmd_unit.vue'
import {reactive} from "vue";

const test = reactive({
  command: 'pyEnv\\Scripts\\conda-unpack',
  description: '测试',
  state: 'pending',
  output: ''
})
</script>

<style scoped>

</style>
