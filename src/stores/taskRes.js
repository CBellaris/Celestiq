import { defineStore } from "pinia";
import { ref, computed } from 'vue'


export const useTaskStore = defineStore("taskRes", ()=>{
  const taskRes = ref([])
  function setTaskRes(newtask){
    taskRes.value = newtask
  }
  return{taskRes, setTaskRes}
});
