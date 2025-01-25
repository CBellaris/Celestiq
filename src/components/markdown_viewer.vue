<template>
    <v-sheet
      class="d-flex justify-start"
      :elevation="0"
      rounded>
      <div class="markdown-viewer">
        <div v-if="error" class="error" style="text-align: center">{{ error }}</div>
        <div
          v-else
          v-html="markdownContent"
          :key="markdownContent"
          class="markdown-content"
          @click.prevent="openLink"
        ></div>
      </div>
    </v-sheet>
</template>

<script setup>
import { ref , onMounted, watch } from "vue";

// 跳转外部链接
function openLink(event) {
  const url = event.target.href;
  window.electronAPI.openExternal(url);
}

const props = defineProps({
  fileName: {
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
watch(() => props.fileName, (newVal, oldVal) => {
  if (newVal !== oldVal && newVal) {
    loadMarkdown(`${newVal}.md`)
  }
});

onMounted(() => {
  loadMarkdown(`${props.fileName}.md`)
})

</script>

<style scoped lang="scss">
.markdown-viewer {
  padding-left: 3%;
  padding-top: 10px;
  padding-bottom: 10px;

  // 使用 ::v-deep 确保样式能够作用到 v-html 内部的内容上
  ::v-deep( .markdown-content) {
    font-family: Arial, sans-serif;
    line-height: 1.6;

    h1 {
      border-bottom: 1px solid #ddd;
      padding-bottom: 0.3em;
    }

    a {
      text-decoration: none;

      &:hover {
        text-decoration: underline;
      }
    }

    img,
    video {
      max-width: 80%;
      margin: 1rem 0;
      filter: brightness(0.95);
    }
  }
}
</style>

