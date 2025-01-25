<script setup>
import { RouterView , useRouter } from 'vue-router'

const router = useRouter();
// 检测是否第一次启动
const checkFirstLaunch = async () => {
  const get_isFirstLaunch = await window.userConfig.get('is_first_launch');
  const isFirstLaunch = get_isFirstLaunch.content || false
  if (isFirstLaunch) {
    await router.push('/initialization');
  }
};
// 检测是否进入调试页面
const check_dev = async () => {
  const get_isdev = await window.userConfig.get('is_dev');
  const isdev = get_isdev.content || false
  if (isdev) {
    await router.push('/dev');
  }
};
onMounted(()=>{
  checkFirstLaunch()
  check_dev()
})

// 界面控制
import { useTheme } from 'vuetify'
import {onMounted} from "vue";
const theme = useTheme()
const minimizeWindow = () => {
  console.log('click')
  window.electron.ipcRenderer.send('window-minimize');
};
const toggleMaximizeWindow = () => {
  window.electron.ipcRenderer.send('window-toggle-maximize');
};
const closeWindow = () => {
  window.electron.ipcRenderer.send('window-close');
};
</script>

<template>
  <v-app style="background-color: rgba(0, 0, 0, 0.0) !important;">
    <!-- 自定义标题栏 -->
    <v-app-bar flat height="48" class="custom-title-bar">
      <v-spacer />
      <div class="button-container">
        <v-btn
          class="mr-8"
          icon="mdi-theme-light-dark"
          size="small"
          @click="theme.global.name.value = theme.global.current.value.dark ? 'light' : 'dark'"
        ></v-btn>
        <v-btn icon="mdi-window-minimize" size="small" @click="minimizeWindow"></v-btn>
        <v-btn icon="mdi-dock-window" size="small" @click="toggleMaximizeWindow"></v-btn>
        <v-btn icon="mdi-close" size="small" @click="closeWindow"></v-btn>
      </div>
    </v-app-bar>

    <RouterView />
  </v-app>
</template>

<style>
.custom-title-bar {
  -webkit-app-region: drag; /* 启用窗口拖动 */
}
.button-container {
  -webkit-app-region: no-drag; /* 使按钮区域可点击 */
  display: flex;
  gap: 0; /* 按钮之间的间距，可根据需求调整 */
}



/* 使用类添加样式控制,直接添加可能与vuetify冲突，使用!important覆盖 */
.bg-myc-success {
  background-color: #00897b !important; /* 背景：鸭绿 */
}

.bg-myc-primary {
  background-color: #00acc1; /* 背景：青蓝 */
}

.bg-myc-error {
  background-color: #F4511E; /* 背景：深橘 */
}

.bg-myc-pending {
  background-color: #757575; /* 背景：浅灰 */
}

/* 可选：单独控制文字颜色 */
.myc-success {
  color: #00897b;
}

.myc-primary {
  color: #00acc1;
}

.myc-error {
  color: #F4511E;
}

.myc-pending {
  color: #757575;
}


</style>
