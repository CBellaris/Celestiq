// src/stores/eventStore.js
import {defineStore} from 'pinia';
import {ref} from 'vue';

export const useEventStore = defineStore('event', () => {
  // 存储所有监听器
  const listeners = ref({});

  // 发送事件
  const emitEvent = (event, payload) => {
    if (listeners.value[event]) {
      listeners.value[event].forEach((callback) => callback(payload));
    }
  };

  // 监听事件
  const onEvent = (event, callback) => {
    if (!listeners.value[event]) {
      listeners.value[event] = [];
    }
    listeners.value[event].push(callback);
  };

  // 移除事件监听
  const offEvent = (event, callback) => {
    if (!listeners.value[event]) return;
    listeners.value[event] = listeners.value[event].filter(
      (cb) => cb !== callback
    );
  };

  return {
    emitEvent,
    onEvent,
    offEvent,
  };
});


// 发送事件的组件
// <template>
//   <div>
//     <button
//     @click="sendEvent">发送事件
//   </button>
// </div>
// </template>
//
// <script setup>
//   import {useEventStore} from '@/stores/eventStore';
//
//   const eventStore = useEventStore();
//
//   // 定义发送事件的方法
//   const sendEvent = () => {
//   eventStore.emitEvent('custom-event', {message: 'Hello from EventEmitter!'});
// };
// </script>

// 监听事件的组件
// <template>
//   <div>
//     <p>接收到的事件数据: {{eventData}}</p>
//   </div>
// </template>
//
// <script setup>
//   import {onMounted, onUnmounted, ref} from 'vue';
//   import {useEventStore} from '@/stores/eventStore';
//
//   const eventStore = useEventStore();
//   const eventData = ref(null);
//
//   // 定义事件处理函数
//   const handleEvent = (payload) => {
//   eventData.value = payload.message;
// };
//
//   onMounted(() => {
//   eventStore.onEvent('custom-event', handleEvent);
// });
//
//   onUnmounted(() => {
//   eventStore.offEvent('custom-event', handleEvent);
// });
// </script>


