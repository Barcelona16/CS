#include "gdt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"
#include "sched.h"
#include "string.h"
#include "debug.h"

// ȫ�� pid ֵ
pid_t now_pid = 0;

// �ں��̴߳���
int32_t kernel_thread(int (*fn)(void *), void *arg)
{
	struct task_struct *new_task = (struct task_struct *)kmalloc(STACK_SIZE); // ���·���һƬ�ڴ�
	assert(new_task != NULL, "kern_thread: kmalloc error");

	// ��ջ�Ͷ˽ṹ��Ϣ��ʼ��Ϊ 0 
	bzero(new_task, sizeof(struct task_struct));

	new_task->state = TASK_RUNNABLE; // ��������
	// ��ǰ���е�����
	// extern struct task_struct *current;
	new_task->stack = new_task; // Ӧ����ָ��ջ�ĵײ���Ҳ���ǵ�ַ��͵ĵط�
	
	new_task->pid = now_pid++; // �̵߳�Ψһ��ʶ
	new_task->mm = NULL;

	uint32_t *stack_top = (uint32_t *)((uint32_t)new_task + STACK_SIZE); // �ðɣ�ջָ��ȷʵ�ڶ���

	*(--stack_top) = (uint32_t)arg; // ѹ�����
	*(--stack_top) = (uint32_t)kthread_exit;
	*(--stack_top) = (uint32_t)fn;

	new_task->context.esp = (uint32_t)new_task + STACK_SIZE - sizeof(uint32_t) * 3; // ����ջָ��

	// ����������ı�־�Ĵ���δ�����жϣ�����Ҫ
	new_task->context.eflags = 0x200;
	new_task->next = running_proc_head;
	
	// �ҵ���ǰ��������У����뵽ĩβ
	// ����һ��˫��ѭ��������ʵ���и��Ӽ򵥵Ĳ��뷽����
	struct task_struct *tail = running_proc_head;
	assert(tail != NULL, "Must init sched!");

	while (tail->next != running_proc_head) {
		tail = tail->next;
	}
	tail->next = new_task;

	return new_task->pid;
}

void kthread_exit()
{
	register uint32_t val asm ("eax");

	printk("Thread exited with value %d\n", val);

	while (1);
}

