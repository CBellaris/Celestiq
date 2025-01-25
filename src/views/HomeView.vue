<template>
  <!-- 导航抽屉 -->
  <v-navigation-drawer permanent width="80" :style="drawerGradientStyle">
    <!-- 图标和文字的菜单项 -->
    <v-list
      density="compact"
      nav
      selectable
      v-model:selected="selected_page"
      select-strategy="single-leaf"
    >
      <v-list-item
        value="start"
        class="d-flex justify-center mb-4"
        height="60"
      >
        <v-icon
          class="mb-2"
          size="large"
          :color="selected_page_item === 'start' ? 'myc_primary' : '' "
        >
          mdi-play-circle-outline
        </v-icon>
        <v-expand-transition>
          <v-list-item-title v-if="selected_page_item !== 'start'">开始</v-list-item-title>
        </v-expand-transition>
      </v-list-item>

      <v-list-item
        value="setup"
        class="d-flex justify-center mb-4"
        height="60"
      >
        <v-icon
          class="mb-2"
          size="large"
          :color="selected_page_item === 'setup' ? 'myc_primary' : ''"
        >
          mdi-cog-play-outline
        </v-icon>
        <v-expand-transition>
          <v-list-item-title v-if="selected_page_item !== 'setup'">配置</v-list-item-title>
        </v-expand-transition>
      </v-list-item>

      <v-list-item
        value="setting"
        class="d-flex justify-center mb-4"
        height="60"
      >
        <v-icon
          class="mb-2"
          size="large"
          :color="selected_page_item === 'setting' ? 'myc_primary' : ''"
        >
          mdi-cogs
        </v-icon>
        <v-expand-transition>
          <v-list-item-title v-if="selected_page_item !== 'setting'">设置</v-list-item-title>
        </v-expand-transition>
      </v-list-item>
    </v-list>
  </v-navigation-drawer>

  <!-- 主内容区域 -->
  <v-main min-width="512" :style="mainGradientStyle">
    <!-- 使用 keep-alive 缓存指定页面 -->
    <keep-alive :include="cachedPages">
      <component :is="pages[selected_page_item]" :key="selected_page_item" />
    </keep-alive>
  </v-main>
</template>

<script setup>
import run_cmd_unit from '../components/run_cmd_unit.vue'
import Qwen2VL from '../components/Qwen2VL.vue'
import Qwen2VL_setup from "../components/Qwen2VL_setup.vue";
import start from "../components/start.vue"
import setup from "../components/setup.vue"
import setting from "../components/setting.vue"
import ComfyUI from "../components/ComfyUI.vue"

import {computed, onMounted, onUnmounted, ref, watch} from "vue"

import { useTheme } from 'vuetify';
// 获取 Vuetify 主题
const theme = useTheme();
// 动态计算导航抽屉的背景渐变样式
const drawerGradientStyle = computed(() => {
  const colors = (theme.global.current.value.dark
    ? 'rgba(0, 0, 0, 0.1)' // 深色主题下的颜色
    : 'rgba(255, 255, 255, 0.1)'); // 浅色主题下的颜色
  return `background-color: ${colors};`
});
// 动态计算主内容区域的背景渐变样式
const mainGradientStyle = computed(() => {
  const colors = (theme.global.current.value.dark
    ? 'rgba(0, 0, 0, 0.9)' // 深色主题下的颜色
    : 'rgba(255, 255, 255, 0.9)'); // 浅色主题下的颜色
  return `backdrop-filter: blur(10px); background-color: ${colors}`
});

// 组件间事件通信
import {useEventStore} from '../stores/eventStore.js';
const eventStore = useEventStore();
const switch_page = (page)=>{
  selected_page_item.value = page
}
onMounted(() => {
  eventStore.onEvent('switch_page', switch_page);
})
onUnmounted(() => {
  eventStore.offEvent('switch_page', switch_page);
})

const pages = {
  start,
  setup,
  setting,
  run_cmd_unit,
  Qwen2VL,
  Qwen2VL_setup,
  ComfyUI
}
// 需要保持加载的页面列表（使用组件名称）
const cachedPages = ['ComfyUI']; // 添加需要缓存的组件名称

const selected_page = ref(["start"])
const selected_page_item = ref('start')
watch(selected_page, (newVal)=>{
  selected_page_item.value=newVal[0]
})
</script>

<style scoped>
</style>
