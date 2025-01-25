<script setup>
import GPU_config from "./GPU_config.vue";
</script>

<template>
  <v-expansion-panels
    class="mt-4 mb-2 opacity-80"
    style="padding-inline: 3%;"
  >
    <v-expansion-panel>
      <v-expansion-panel-title>
        Nvidia依赖检测
      </v-expansion-panel-title>
      <v-expansion-panel-text>
        <GPU_config></GPU_config>
      </v-expansion-panel-text>
    </v-expansion-panel>

    <v-expansion-panel>
      <v-expansion-panel-title>
        python安装
      </v-expansion-panel-title>
      <v-expansion-panel-text>
        TODO
      </v-expansion-panel-text>
    </v-expansion-panel>
  </v-expansion-panels>
</template>

<style scoped>

</style>
