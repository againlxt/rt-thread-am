/*
 * @Author: 23060306-Lei Xiao Tian leixiaotian434@gmail.com
 * @Date: 2024-09-30 09:47:07
 * @LastEditors: 23060306-Lei Xiao Tian leixiaotian434@gmail.com
 * @LastEditTime: 2024-10-03 15:22:25
 * @FilePath: /rt-thread-am/bsp/abstract-machine/src/context.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <am.h>
#include <klib.h>
#include <rtthread.h>
static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
      rt_thread_t current   = rt_thread_self();
      rt_ubase_t *contexts  = (rt_ubase_t *) current->user_data;
      rt_ubase_t to         = contexts[0];
      rt_ubase_t from       = contexts[1];
      if(from) {
        *((Context **) from) = c;
        c = *((Context **) to);
      } 
      else 
        c = *((Context **) to);
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }

  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_thread_t current = rt_thread_self();
  rt_ubase_t temp;
  rt_ubase_t data[2];

  temp = current->user_data;
  data[0] = to;
  data[1] = 0;
  current->user_data = (rt_ubase_t)data;

  yield();

  current->user_data = temp;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_thread_t current = rt_thread_self();
  rt_ubase_t temp;
  rt_ubase_t data[2];

  temp = current->user_data;
  data[0] = to;
  data[1] = from;
  current->user_data = (rt_ubase_t)data;

  yield();

  current->user_data = temp;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

void (*rt_entry) (void *args) = NULL;
void (*rt_exit) () = NULL;

static void rt_hw_entry(void *arg) {
  uintptr_t *args = NULL;
  args = arg;
  void *tentry = (void *) args[0];
  void *parameter = (void *) args[1];
  void *texit = (void *) args[2];

  rt_entry = tentry;
  rt_exit = texit;
  rt_entry(parameter);
  rt_exit();
}

/**
 * @description: Init context.
 * This function is used tois to create a context with tentry 
 * as the entry and parameter as parameter with stack_addr as 
 * the bottom of the stack, and return the pointer of this 
 * context structure. In addition, if the kernel thread 
 * corresponding to the context returns from tentry, texit 
 * is called, and RT-Thread will ensure that the code will 
 * not return from texit.
 * @param {void} *tentry, entry of context.
 * @param {void} *parameter, parameter of context.
 * @param {rt_uint8_t} *stack_addr, stack end.
 * @param {void} *texit, when kernel thread is return from
 * tentry, texit while be call.
 * @return {rt_uint8_t *} pointer of this context.  
 */
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  // Align uintptr_t
  rt_uint32_t stack_addr_size = (rt_uint32_t) stack_addr;
  rt_uint32_t stack_addr_offset = stack_addr_size % (sizeof(uintptr_t) / sizeof(rt_uint8_t));
  stack_addr = stack_addr + ((sizeof(uintptr_t) / sizeof(rt_uint8_t)) - stack_addr_offset);

  uintptr_t *args = (uintptr_t *)((Context *)stack_addr - 2);
  *args = (uintptr_t) tentry; *(args+1) = (uintptr_t) parameter; *(args+2) = (uintptr_t) texit;
  Area kstack = {.end = stack_addr};

  void *context = (void *) kcontext(kstack, rt_hw_entry, (void *) args);

  return (rt_uint8_t *) context;
}
