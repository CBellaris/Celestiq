<script setup>
import markdownViewer from "/src/components/markdown_viewer.vue"
import {ref} from 'vue'

const props = defineProps({
  taskName: {
    type: String,
    required: true,
  },
});

defineEmits(['startSetup'])
</script>

<template>
  <div style="position:fixed; right:30px; bottom:30px; z-index:50">

    <v-tooltip text="开始配置" location="start">
      <template v-slot:activator="{ props }">
        <v-btn v-bind="props" icon="mdi-chevron-right" size="x-large" @click="$emit('startSetup')" color="secondary"></v-btn>
      </template>
    </v-tooltip>
  </div>
  <div class="introduce">
    <markdownViewer :fileName="props.taskName" />
  </div>
</template>

<style scoped>

</style>
