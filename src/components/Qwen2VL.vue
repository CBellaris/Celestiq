<template>
  <v-container>
    <v-card class="pa-3" outlined>
      <v-card-title>拖入图片，点击推理</v-card-title>
      <v-card-subtitle>拖入的图片会另存一份在'./outSrc/images'文件夹下</v-card-subtitle>
      <v-card-text>
        <div
          class="drop-zone"
          @drop.prevent="handleDrop"
          @dragover.prevent
          @dragenter.prevent
        >
          拖拽图片到此区域
        </div>
      </v-card-text>
      <v-divider></v-divider>
      <v-card-text>
        <v-card
          v-for="(image, index) in images"
          :key="index"
          class="mb-3 mt-3"
        >
          <v-row>
            <!-- 图片部分 -->
            <v-col cols="2" class="d-flex align-center justify-center">
              <v-img
                :src="`local://${image.path}`"
                max-height="150"
                class="cursor-pointer mb-1 mt-1"
                @click="previewImage(image)"
                :class="{ 'image-loading': image.waiting }"
              />
            </v-col>

            <!-- 列表部分 -->
            <v-col cols="8">
              <v-list>
                <v-list-item>
                  <v-list-item-title>{{ image.name }}</v-list-item-title>
                  <v-list-item-subtitle>
                    {{ `${image.width} x ${image.height}` }}
                  </v-list-item-subtitle>
                </v-list-item>
                <v-list-item>
                  <div>
                    {{ image.content }}
                  </div>
                </v-list-item>
              </v-list>
            </v-col>
            <v-col cols="2" class="align-self-end mb-2">
              <!-- 操作按钮 -->
              <div>
                <v-btn text="推理" color="secondary" @click="QwenInference(image.path); image.waiting = true"></v-btn>
              </div>
              <div>
                <v-btn text="删除" color="error" @click="removeImage(index)"></v-btn>
              </div>
            </v-col>
          </v-row>
        </v-card>
      </v-card-text>
    </v-card>
    <!-- 图片预览对话框 -->
    <div v-if="previewedImage">
    <v-dialog
      v-model="isPreviewDialogOpen"
      :max-width="previewedImage.width > previewedImage.height ? '60vw':'40vw'"
      :max-height="previewedImage.height > previewedImage.width ? '80vh':'60vh'"
    >
      <v-card class="pa-0">
        <v-img
          :src="`local://${previewedImage.path}`"
          max-width="100%"
          max-height="100%"
        />
      </v-card>
    </v-dialog>
    </div>
  </v-container>
<!--  <div>-->
<!--    &lt;!&ndash; 固定在右下角的按钮 &ndash;&gt;-->
<!--    <div style="position:fixed; right:20px; bottom:30px; z-index:51">-->
<!--      <v-btn icon="mdi-cog" size="x-large" @click="showSetting = true"></v-btn>-->
<!--    </div>-->

<!--    &lt;!&ndash; 背景遮罩层 &ndash;&gt;-->
<!--    <div-->
<!--      v-if="showSetting"-->
<!--      class="overlay"-->
<!--      @click="showSetting = false"-->
<!--    ></div>-->

<!--    &lt;!&ndash; 设置区域 &ndash;&gt;-->
<!--    <v-expand-x-transition>-->
<!--      <v-card-->
<!--        v-show="showSetting"-->
<!--        class="settings-card"-->
<!--        elevation="5"-->
<!--      >-->
<!--        <v-card-title>设置</v-card-title>-->
<!--        <v-card-text>-->
<!--          <download></download>-->
<!--        </v-card-text>-->
<!--      </v-card>-->
<!--    </v-expand-x-transition>-->
<!--  </div>-->
</template>

<script setup>
import {computed, onMounted, onUnmounted, ref} from 'vue';

const images = ref([]);
const prompts = ref([])
const isPreviewDialogOpen =  ref(false)// 控制对话框显示
const previewedImage = ref(null) // 当前预览的图片
const processing = ref(false)
// const showSetting = ref(false)

// 拖拽上传图片
const handleDrop = async (event) => {
  const files = Array.from(event.dataTransfer.files).filter(file =>
    ['image/png', 'image/jpeg', 'image/gif', 'image/bmp'].includes(file.type)
  );
  if (files.length === 0) {
    console.warn('没有有效的图片文件');
    return;
  }
  for (const file of files) {

    try {
      // 读取文件为 ArrayBuffer
      const fileBuffer = await file.arrayBuffer();
      // 保存文件到主进程
      const result = await window.imageHandler.saveImage(file.name, fileBuffer);
      console.log(result)

    } catch (error) {
      console.error('保存文件失败:', error);
    }
  }
};

function changeFileExtension(fileName, newExtension = 'txt') {
    // 确保新扩展名以点号开头
    const extensionWithDot = `.${newExtension.replace(/^\./, '')}`;

    // 使用正则表达式移除旧的文件扩展名，并添加新的扩展名
    return fileName.replace(/\.[^/.]*$/, extensionWithDot);
}

// 删除图片
const removeImage = async (index) => {
  const image = images.value[index];
  const txtPath = changeFileExtension(image.path)
  let success = await window.imageHandler.deleteImage(image.path);
  success = await window.imageHandler.deleteImage(txtPath) && success
  if (!success) {
    alert('删除失败，请重试');
  }
};

const previewImage = (image) => {
  previewedImage.value = image;
  isPreviewDialogOpen.value = true;
}

function updatePromptToImage(){
  prompts.value.forEach(prompt =>{
    const target = images.value.find((image) => compareWithoutExtension(image.name, prompt.name));
    if (target) {
        target.content = prompt.content;
      }
  })
}

function compareWithoutExtension(str1, str2) {
    // 使用正则表达式移除文件扩展名
    const removeExtension = (str) => str.replace(/\.[^/.]+$/, "");

    // 比较去掉扩展名后的字符串
    return removeExtension(str1).toLowerCase() === removeExtension(str2).toLowerCase();
}
// 处理文件变化
function handleFileChange(data) {
  const { type, fileType, fileName, filePath, dimensions, content } = data;
  console.log('vue0: ', { type, fileType, fileName, filePath, dimensions })
  if (type === 'add') {
    if (fileType === 'image') {
      images.value.push({
        fileType,
        name: fileName,
        path: filePath,
        width: dimensions.width,
        height: dimensions.height,
        content:'',
        waiting: false
      });
    } else if(fileType === 'text'){
      prompts.value.push({
        name: fileName,
        content: content
      })
    }

  } else if (type === 'delete') {
    if(fileType === 'image'){
      images.value = images.value.filter((file) => file.name !== fileName);
      prompts.value = prompts.value.filter((file) => compareWithoutExtension(file.name, fileName));
    } else if(fileType === 'text'){
      prompts.value = prompts.value.filter((file) => file.name !== fileName);
      }
    }
  updatePromptToImage()
  console.log('images', images.value)
}

async function QwenInference(imagePath){
  if (!processing.value){
    processing.value = true
    const prompt='Describe this image in detail in Chinese'
    const res = await window.MyAPI.Qwen2VL(prompt, imagePath)
    if (res.suceed){
      // 执行完毕返回的完成信息res.output
    }else{
     // 执行失败返回的完成信息res.output
    }
    processing.value = false
  }
}

// 处理qwen输出
function handleQwenOutput(data){
  const target = images.value.find(image=> image.name === data.name)
  if(target){
    target.waiting = false
    target.content = data.output
  }
}


onMounted(() => {
  // 开始监控
  window.imageHandler.startWatch()
  window.imageHandler.onChange((data)=>handleFileChange(data))
  window.MyAPI.onQwen2VL((data)=>handleQwenOutput(data))
});

onUnmounted(() => {
  // 停止监听
  window.imageHandler.stopOnChange()
  window.MyAPI.stopOnQwen2VL()
});
</script>

<style scoped>
.drop-zone {
  border: 2px dashed #ccc;
  padding: 20px;
  text-align: center;
  color: #aaa;
  border-radius: 8px;
  transition: border-color 0.3s;
}

.drop-zone:hover {
  border-color: #1976d2;
  color: #1976d2;
}

.cursor-pointer {
  cursor: pointer;
}

.image-loading {
  animation: focusBlurEffect 2s infinite ease-in-out;
}

@keyframes focusBlurEffect {
  0% {
    transform: scale(1);
    filter: blur(0);
  }
  50% {
    transform: scale(1.05);
    filter: blur(1px);
  }
  100% {
    transform: scale(1);
    filter: blur(0);
  }
}

/* 半透明遮罩层样式 */
.overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.3); /* 背景变暗 */
  z-index: 40; /* 低于设置区域的z-index */
}

/* 设置区域样式 */
.settings-card {
  position: fixed;
  bottom: 5px;
  right: 50px;
  width: 100%; /* 占据窗口宽度 */
  height: 50%; /* 占据窗口下半部分 */
  max-width: 700px; /* 可选：限制宽度 */
  z-index: 50; /* 确保在遮罩层之上 */
}
</style>
