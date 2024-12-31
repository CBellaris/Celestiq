<template>
  <div class="markdown-viewer">
    <div v-if="error" class="error">{{ error }}</div>
    <div v-else v-html="markdownContent" :key="markdownContent" class="markdown-content"></div>
  </div>
</template>

<script setup>
import { ref , onMounted, watch } from "vue";

const props = defineProps({
  taskName: {
    type: String,
    required: true,
  },
});

const markdownContent = ref("markdown");
const error = ref("");


async function loadMarkdown(path) {
  if (!path) {
    markdownContent.value = "<p>No file path provided.</p>";
    return;
  }
  try {
    const result = await window.MyAPI.readMarkdown(path);
    if (result.success) {
      markdownContent.value = result.content
      error.value = "";
    } else {
      throw new Error(result.error);
    }
  } catch (err) {
    console.error("Error loading markdown:", err);
    markdownContent.value = "";
    error.value = "Failed to load markdown file.";
  }
}

// 监听 props.filePath 的变化
watch(() => props.taskName, (newVal, oldVal) => {
  console.log(newVal)
  if (newVal !== oldVal) {
    loadMarkdown(`${newVal}.md`)
  }
});

onMounted(() => {
  loadMarkdown(`${props.taskName}.md`)
})

</script>

<style scoped>
.markdown-content {
  font-family: Arial, sans-serif;
  line-height: 1.6;
  color: #333;
}
.markdown-content h1 {
  border-bottom: 1px solid #ddd;
  padding-bottom: 0.3em;
}
.markdown-content a {
  color: #007acc;
  text-decoration: none;
}
.markdown-content a:hover {
  text-decoration: underline;
}
.markdown-viewer img {
  max-width: 80%;
  height: auto;
}
.markdown-viewer video {
  max-width: 80%;
  margin: 1rem 0;
}
</style>

