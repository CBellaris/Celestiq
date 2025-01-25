import { createRouter, createWebHashHistory } from 'vue-router'
import HomeView from '../views/HomeView.vue'
import initialization from '../views/initialization.vue'
import dev from '../views/dev.vue'

const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    {
      path: '/',
      name: 'home',
      component: HomeView,
    },
    {
      path: '/initialization',
      name: 'initialization',
      component: initialization,
    },
    {
      path: '/dev',
      name: 'dev',
      component: dev,
    },
  ],
})

export default router
