#include <env.h>
#include <pmap.h>
#include <printk.h>

/* Overview:
 *   Implement a round-robin scheduling to select a runnable env and schedule it using 'env_run'.
 *
 * Post-Condition:
 *   If 'yield' is set (non-zero), 'curenv' should not be scheduled again unless it is the only
 *   runnable env.
 *
 * Hints:
 *   1. The variable 'count' used for counting slices should be defined as 'static'.
 *   2. Use variable 'env_sched_list', which contains and only contains all runnable envs.
 *   3. You shouldn't use any 'return' statement because this function is 'noreturn'.
 */
void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv;
	static int flag = 0;
	/* We always decrease the 'count' by 1.
	 *
	 * If 'yield' is set, or 'count' has been decreased to 0, or 'e' (previous 'curenv') is
	 * 'NULL', or 'e' is not runnable, then we pick up a new env from 'env_sched_list' (list of
	 * all runnable envs), set 'count' to its priority, and schedule it with 'env_run'. **Panic
	 * if that list is empty**.
	 *
	 * (Note that if 'e' is still a runnable env, we should move it to the tail of
	 * 'env_sched_list' before picking up another env from its head, or we will schedule the
	 * head env repeatedly.)
	 *
	 * Otherwise, we simply schedule 'e' again.
	 *
	 * You may want to use macros below:
	 *   'TAILQ_FIRST', 'TAILQ_REMOVE', 'TAILQ_INSERT_TAIL'
	 */
	/* Exercise 3.12: Your code here. */
	if(e!=NULL){
		e->env_count = e->env_count + ((struct Trapframe *)KSTACKTOP - 1)->cp0_count;
	}
	if(yield!=0 || count==0 || e==NULL || e->env_status!=ENV_RUNNABLE){ //需要切换进程
		if(!TAILQ_EMPTY(&env_sched_list)){
			if(e!=NULL){ //切换进程的时候才需要移动位置
				if(e->env_status == ENV_RUNNABLE){
					TAILQ_REMOVE(&env_sched_list, e, env_sched_link); //将e从调度队列中取出来，把头部让给下一个
					TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link); //插回队列尾部
					(e->env_sched_count)++; //sched count +1
					//e->env_tf.cp0_count = e->env_tf.cp0_count + ((struct Trapframe *)KSTACKTOP - 1)->cp0_count;
				}else{
					TAILQ_REMOVE(&env_sched_list, e, env_sched_link); //不是可runnable的，直接移除调度队列
				}
			}else{
				flag=1;
			}
			e = TAILQ_FIRST(&env_sched_list); //拿出进程的时候无需remove和insert，用完了再移动
			count = e->env_pri;
		}else{
			panic("schedule: no runnable envs");
		}
	}
	count--;
	env_run(e);
}
