<template>
  <div>
    <!-- 输出显示 -->
    <div class="outputs">
      <v-card outlined style="overflow-y: auto; overflow-x:auto" :style="cardStyle">
        <v-card-subtitle>
          <div class="d-flex align-content-center">
            <v-progress-circular
              v-if="running"
              color="myc_pending"
              indeterminate
              size="x-small"
              class="mr-2"
            ></v-progress-circular>
            {{ `${props.description} (${commandId})` }}
            <v-spacer/>
            <!-- 复制按钮 -->
            <v-tooltip text="复制输出" location="start">
              <template v-slot:activator="{ props }">
                <v-btn
                  v-bind="props"
                  icon="mdi-content-copy"
                  @click="copyToClipboard"
                  size="x-small"
                />
              </template>
            </v-tooltip>
          </div>
        </v-card-subtitle>
        <v-card-text>
          <pre>{{ output_cmd }}</pre>
        </v-card-text>
      </v-card>
    </div>
  </div>
</template>

<script setup>
import {ref, onMounted, onBeforeUnmount, watch, onUnmounted} from 'vue';

const props = defineProps({
  command: {
    type: String,
    required: true,
  },
  description:{
    type:String,
    default:'Running command'
  },
  autoRun: {
    type: Boolean,
    default: false,
  },
  cardStyle: {
    type: String,
    default: "", // 默认样式为空
  },
  output:{
    type: String,
    default: "",
  },
  output_stream:{
    type:String,
    default:""
  }
});

const emit = defineEmits(["update:output","update:output_stream", "finish", "failure"]);

const commandId = `cmd-${Math.random().toString(36).slice(2, 9)}`;
const output_cmd = ref("");
const running = ref(false);

const copyToClipboard = async ()=>{
  try {
        const text = output_cmd.value;
        await navigator.clipboard.writeText(text);
      } catch (err) {
        console.error('复制失败: ', err);
      }
}

const startCommand = () => {
  if (running.value) return;
  output_cmd.value = "";
  running.value = true;
  window.electronAPI.startCommand(commandId, props.command);
};

const stopCommand = () => {
  if (!running.value) return;
  console.log('stopcommand in vue', commandId)
  window.electronAPI.stopCommand(commandId);
  running.value = false;
};

// 使用 defineExpose 暴露函数
defineExpose({
  startCommand,
  stopCommand
});

// 定义监听器函数
const handleCommandOutput = (id, data) => {
  if (id === commandId) {
    output_cmd.value += data;
    emit("update:output_stream", data);

    if (data.includes("Command finished")) {
      const code = parseInt(data.replace('Command finished with exit code ', ''));
      running.value = false;
      emit("update:output", output_cmd.value);
      emit("finish", props.command, code);
    } else if (data.includes("Error")) {
      running.value = false;
      emit("update:output", output_cmd.value);
      emit("failure", props.command);
    }
  }
};

onMounted(() => {
  if (props.autoRun) {
    startCommand();
  }
  // 注册监听器
  window.electronAPI.onCommandOutput(handleCommandOutput);
});
// 在组件销毁时移除监听器
onUnmounted(() => {
  window.electronAPI.removeCommandOutputListener(handleCommandOutput);
});

onBeforeUnmount(() => {
  stopCommand();
});

watch(
  () => props.command,
  (newCommand, oldCommand) => {
    if (newCommand !== oldCommand && running.value) {
      stopCommand();
    }
  }
);
</script>
