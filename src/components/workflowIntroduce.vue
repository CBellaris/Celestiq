<script setup>
import markdownViewer from "/src/components/markdownViewer.vue"
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
  <div class="absolutely-btn" style="position:fixed; right:30px; bottom:30px; z-index: 50">
    <v-col cols="auto">
      <v-btn icon="mdi-plus" size="x-large" @click="$emit('startSetup')"></v-btn>
    </v-col>
  </div>
  <div class="introduce">
    <markdownViewer :taskName="props.taskName" style="padding-left: 20px" />
  </div>
</template>

<style scoped>

</style>
