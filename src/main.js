// import './assets/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'

// Vuetify
import 'vuetify/styles'
import { createVuetify } from 'vuetify'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'
import '@mdi/font/css/materialdesignicons.css'

const app = createApp(App)

// Pinia
app.use(createPinia())

app.use(router)

const vuetify = createVuetify({
  theme: {
    defaultTheme: 'dark',
    themes: {
      light: {
        colors: {
          myc_primary: '#00acc1', // 青蓝
          myc_error: '#F4511E',   // 深橘
          myc_success: '#00897b', // 鸭绿
          myc_pending: '#757575', // 浅灰
        },
      },
      dark: {
        colors: {
          myc_primary: '#00acc1', // 青蓝
          myc_error: '#F4511E',   // 深橘
          myc_success: '#00897b', // 鸭绿
          myc_pending: '#757575', // 浅灰
        },
      },
    },
  },
  components,
  directives,
})
app.use(vuetify)

app.mount('#app')
