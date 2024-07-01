# 挑战性任务challenge-shell

使用`make test lab=6_2 && make run`启动我们的shell

### 实现不带.b后缀指令

修改了`runcmd`函数。利用`spawn`去找到如ls扩展成ls.b的文件，执行文件里的内容。`(elf_from,elf_load_seg,(可执行文件))`。如果扩展后返回的子shell进程的id值还是小于0，就说明不存在这个.b文件，出现`debugf`的报错即可

```c
//user/sh.c

void runcmd(char *s){
    /*...*/
    int child = spawn(argv[0], argv);
    /*以下为修改处*/
    if(child < 0) { //找不到这个文件ls
		char prog[1024] = {0};
		strcpy(prog, argv[0]);
		int len = strlen(argv[0]);
		prog[len] = '.';
		prog[len + 1] = 'b';
		prog[len + 2] = '\0'; //将ls扩展成ls.b，去找.b文件即可
		child = spawn(prog, argv);
	}
}
```

### 实现引号支持

修改了`_gettoken`函数，将引号后面的内容作为一个整体返回。

```c
//user/sh.c

int _gettoken(char *s, char **p1, char **p2){
    /*...*/
    if(*s == '\"'){ //第一个字符为"，返回引号中的内容
		s++; //指向引号后面的内容
		*p1 = s; //因为p1对应的t，所以p1装的token的第一个字符
		while(*s && *s!='\"'){
			s++; //移动s的位置
		}
		*s = '\0';
		*p2 = s + 1;
		return 'w'; //引号里的内容当作一个w返回
	}
}
```

### 实现一行多命令

`;`的实现模拟`|`的处理，只是不用打开控制台。因此流程基本照抄`|`的处理思路即可。

- 函数作用

`return argc`，起到执行左侧代码的作用

`return parsecmd(argv, rightpipe)`起到执行这一行剩下的parse过程。

```c
//user/sh.c
	int parsecmd(char **argv, int *rightpipe){
        /*...*/
        case ';':
        	child = fork();
			if(child == 0){
				return argc;
			}else if(child > 0){
				wait(child);
				close(0);close(1);
				dup(opencons(), 1);dup(1,0);
				return parsecmd(argv, rightpipe);
			}
			break;
    }
}
```



### 实现注释功能

抛弃后面的注释功能，即识别到`#`就代表这一行的line结束了，与`case'0'`的执行效果是一样的。

所以将`#`添加进`SYMBOLS`的宏定义，在`parsecmd`中检测到`case'#'`直接`return argc`即可。

```c
//user/sh.c

#define SYMBOLS "<|>&;()#"

int parsecmd(char **argv, int *rightpipe){
    /*...*/
    
}
```



### 实现追加重定向

首先实现了追加模式的打开，这样可以在打开文件的时候指向文件的原有内容的最后一个字符的位置，而不是从文件开头。修改了`user/include/lib.h`和`fs/serv.c`。

```c
//user/include/lib.h
#define O_APPEND 0x1000 /*追加模式*/

//fs/serv.c
void serve_open(u_int envid, struct Fsreq_open *rq){
    /*...*/
    if(o->o_mode & O_APPEND){ //可追加
        struct Fd *fff = (struct Fd *)ff;
        fff->fd_offset = f->f_size; //偏移量设置为文件原来的大小
    }
}
```

然后修改了`_gettoken`，使其能够识别`>>`符号，并`return 'p'`作为自己的识别符号。

```c
//user/sh.c
int _gettoken(char *s, char **p1, char **p2){
    if (strchr(SYMBOLS, *s)) {
		int t = *s;
		if(t == '>' && *(s+1) == '>'){ //识别到追加重定向
			*p1 = s;
			*s = 0;
			s = s + 2;
			*p2 = s;
			return 'p'; //repeat,追加重定向的标志
		}
	}
}
```

然后在`parsecmd`中仿照`>`对`>>`做相应的处理，唯一的修改就是文件的打开模式改为`O_APPEND | O_WRONLY `即可。

```c
//user/sh.c
int parsecmd(char **argv, int *rightpipe){
    case 'p':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: >> not followed by word\n");
				exit(1);
			}
			if((fd = open(t, O_APPEND | O_WRONLY)) < 0){
				if((r = create(t, FTYPE_REG)) < 0){
					debugf("failed to create '%s'\n", t);
					exit(1);
				}else{
					if((fd = open(t, O_APPEND | O_WRONLY)) < 0){
						debugf("failed to open '%s'\n", t);
						exit(1);
					}
				}
			}
			dup(fd, 1); //将fd的数据复制到标准输出里面去
			close(fd);
			//user_panic(">> redirection not implemented");
			break;
}
```



### 实现反引号

在`_gettoken`里面识别\`，然后返回一个对应的parsecmd解析类型就行，比如return \`。

然后在`parsecmd`里对case \`做相应的`runcmd`即可。因为`echo`一个反引号装着的指令，与直接执行反引号的指令是一样的。所以说直接`run`就行了。

```c
//user/sh.c

int _gettoken(char *s, char **p1, char **p2){
    /*...*/
    if(*s == '`'){ //第一个字符为反引号`
		s++;
		*p1 = s; //指向`后面的内容
		while(*s && *s!='`'){
			s++;
		}
		*s = '\0'; //找到完结点
		*p2 = s + 1; //p2指向下一个token
		return '`';
	}
}

int parsecmd(char **argv, int *rightpipe){
    case '`': //t已经装好内容了
			runcmd(t); //echo 反引号的内容等价于直接执行后面的语句
			break;
}
```



### 实现更多指令

- `touch`

<img src="C:\Users\28670\AppData\Roaming\Typora\typora-user-images\image-20240622104841187.png" alt="image-20240622104841187" style="zoom:50%;" />

首先理清文件系统逻辑。

$$fd.c \rightarrow file.c \rightarrow fsipc.c \rightarrow serv.c \rightarrow fs.c$$

`fsipc.c`与`serv.c`即`user`进程和`fileServe`进程通过`fsipc`联系。`fsipc`使用了`ipc_send`和`ipc_recv`进行双向接受，即一个进程即发送给另一个进程消息，又从那个进程接受发回的消息。

`fd.c`的功能：根据`Fd`号获取`Fd`数据，判断是`file`、`pipe`、`console`中的哪一种类型，进入到设备种类运行该函数的函数。在我们本次任务中，是文件系统类型，于是进入了`file.c`进行接下来的运行。想一想平时重定向`>`的行为，本来是要找到这个文件，把数据输入进去的，但是很多情况下，本地并没有这个文件，所以打开不了，要创建。

所以采取了增加`create`函数，使得文件无法打开的时候创建即可。因此新增了`file.c`的`create`。然后顺其自然新增了`fsipc.c`的`fsipc_create`函数。`serv.c`的`serve_create`分发函数。注意的是事实上`fs.c`已经有了`file_create`函数的处理了，提供了接口我们直接使用就行，因此创建文件的一点已经有轮子了，无需重造。直接在`serve_create`函数里调用`file_create`函数，即可实现我们的想法。最后把文件创建成功与否的值发回给`fsipc.c`让它通知`user`进程即完成。最终又返回到一开始申请创建的文件`file.c`中，函数的返回值就说明了一切。

然后需要修改`sh.c`的重定向。只需要修改`>`和`>>`就行，即打开不了文件，就创建一个文件。然后再执行后续的行为。即把原来打开不了就报错的行为修改了。变成打开不了先创建，创建不了或者打开不了再报错。

```c
//fs/serv.c

void serve_create(u_int envid, struct Fsreq_create *rq){
	int r;
	struct File *f;
	if((r = file_create(rq->req_path, &f))<0){ //只填充了f的name，f的type还是未知
		ipc_send(envid, r, 0, 0);
		return;
	}
	f->f_type = rq->f_type;
	ipc_send(envid, 0, 0, 0);
}

void *serve_table[MAX_FSREQNO] = {
    [FSREQ_OPEN] = serve_open,	 [FSREQ_MAP] = serve_map,     [FSREQ_SET_SIZE] = serve_set_size,
    [FSREQ_CLOSE] = serve_close, [FSREQ_DIRTY] = serve_dirty, [FSREQ_REMOVE] = serve_remove,
    [FSREQ_SYNC] = serve_sync,
    [FSREQ_SYNC] = serve_sync,   [FSREQ_CREATE] = serve_create,
};
```



```c
//user/include/fsreq.h

enum {
	FSREQ_OPEN,
	FSREQ_MAP,
	FSREQ_SET_SIZE,
	FSREQ_CLOSE,
	FSREQ_DIRTY,
	FSREQ_REMOVE,
	FSREQ_SYNC,
	FSREQ_CREATE,
	MAX_FSREQNO,
};

struct Fsreq_create{
	char req_path[MAXPATHLEN];
	u_int f_type;
};

```



```c
//user/include/lib.h
int fsipc_create(const char *, u_int, struct Fd *);

int create(const char *path, int f_type);
```



```c
//user/lib/file.c
int create(const char *path, int f_type){ //touch.c调用
	int r;
	struct Fd *fd;
	try(fd_alloc(&fd));
	if((r = fsipc_open(path, O_RDONLY, fd)) < 0){ //如果文件不存在，就创建
		return fsipc_create(path, f_type, fd); //fsipc的返回值，<0不成功，=0成功
	}else{ //文件存在
		return 1;
	}
}
```



```c
//user/lib/fsipc.c
int fsipc_create(const char *path, u_int f_type, struct Fd *fd){ //仿照open的行为,这里的f_type是文件的类型
	u_int perm;
	struct Fsreq_create *req;
	req = (struct Fsreq_create *)fsipcbuf;
	if(strlen(path) >= MAXPATHLEN){
		return -E_BAD_PATH;
	}
	strcpy((char *)req->req_path, path);
	req->f_type = f_type;
	return fsipc(FSREQ_CREATE, req, fd, &perm);
}

```

```c
//user/touch.c

#include <lib.h>

int main(int argc, char **argv){
	if(argc == 1){
        debugf("please touch a file with absolute path!\n");
        exit(1);
    }else{
        int r = create(argv[1], FTYPE_REG);
        if(r < 0){ //创建不成功
            debugf("touch: cannot touch '%s': No such file or directory\n", argv[1]);
            exit(1);
        }
        return 0;
    }
}
```



- `mkdir`

与`touch`没太大区别，因为底层逻辑写好了，只需要在用户端调用`mkdir`，即`mkdir.c`实现逻辑即可。反复调用`create`的魅力。

```c
#include <lib.h>

void normal_create(int argc, char **argv){
	int r = create(argv[1], FTYPE_DIR);
	if(r < 0){ //创建目录的父目录不存在
		debugf("mkdir: cannot create directory '%s': No such file or directory\n", argv[1]);
		exit(1);
	}else if(r == 1){ //创建的已经存在
		debugf("mkdir: cannot create directory '%s': File exists\n", argv[1]);
		exit(1);
	}
}

void special_create(int argc, char **argv){
	char buf[1024] = {0};
	int i = 0;
	int r;
	while(1){
		if(argv[2][i] =='/'){ //因为识别到/会break并进行一轮mkdir的创建，后面可能还有目录，要继续1/2/3下去
			buf[i] = argv[2][i];
			i++; //跳过'/'
		}
		for(;;i++){
			if(argv[2][i] == '\0'){ //结束了
				break;
			}
			if(argv[2][i] == '/'){ //识别到分割线了
				break;
			}
			buf[i] = argv[2][i]; //buf存储一路上遇到的名字
		}
		r = create(buf, FTYPE_DIR);
		if(r == 1){
			if(argv[2][i] == '\0'){ //识别完了
				return;
			}else{ //还没搞完
				continue;
			}
		}
	}
}
int main(int argc, char **argv){
	if(argc == 1){
		debugf("please mkdir with absolute path!\n");
		exit(1);
	}
	if(strcmp(argv[1],"-p") != 0){
		normal_create(argc, argv);
	}else{
		special_create(argc, argv);
	}
	return 0;
}
```



- `rm`

因为底层函数已经有`remove`的一系列行为了，我们需要做的只是给`rm.c`添加上内容，让可执行文件能够执行`rm`的相关指令。阅读`ls.c`学习获取文件信息的行为`stat`函数。返回值判断文件是否存在，用`st.st_isdir`判断文件类型。然后删除操作统一调用`remove`函数即可。

```c
#include <lib.h>

int main(int argc, char **argv){
	if(argc == 1){
		debugf("please use rm with at least one parameter!\n");
		exit(1);
	}else if(strcmp(argv[1], "-r") == 0){ //rm -r
		int r;
		struct Stat st;
		if((r = stat(argv[2], &st)) < 0){ //文件不存在
			debugf("rm: cannot remove '%s': No such file or directory\n", argv[2]);
			exit(1);
		}
		if(st.st_isdir){ //TODO:暂时不采用递归删除
			r = remove(argv[2]);
		}else{ //是文件
			r = remove(argv[2]);
		}
	}else if(strcmp(argv[1], "-rf") == 0){ //rm -rf
		int r;
		struct Stat st;
		if((r = stat(argv[2], &st)) < 0){ //文件不存在直接退出
			exit(1);
		}
		if(st.st_isdir){ //TODO:暂时不采用递归删除
			r = remove(argv[2]);
		}else{ //是文件
			r = remove(argv[2]);
		}
	}else{ //rm
		int r;
		struct Stat st;
		if((r = stat(argv[1], &st)) < 0){ //文件不存在
			debugf("rm: cannot remove '%s': No such file or directory\n", argv[1]);
			exit(1);
		}
		if(st.st_isdir){ //想删除的是目录
			debugf("rm: cannot remove '%s': Is a directory\n", argv[1]);
			exit(1);
		}else{ //想删除的是文件
			r = remove(argv[1]); //删除必定成功
		}
	}
	return 0;
}
```

### 实现历史指令

首先，这是个shell built-in command，即是写在sh.c内部的。而不是通过烧录一个.b文件实现运行。所以要修改`runcmd`的逻辑。在`spawn`上新增一个判断，判断这条指令是否是`history`指令，不是的话才走`spawn`那条路线运行`.b`文件的路。

`.mosh_history`这个文件应该在第一次`readline`的时候创建，这样实现了即使第一条指令运行，`.mosh_history`也已经在根目录下了，因为运行逻辑是$readline\rightarrow runcmd \rightarrow parsecmd$的，即第一次`runcmd`前已经建立这个文件了。

此外，可以实现一个缓冲数组`history_buf`以及计数器`history_count`，用来记录当前有多少条指令，以及指令分别是什么，在每次`readline`的时候就完成数组的更新，所以实际上可以通过`debugf`数组内容完成history命令的查询的，而无需通过文件。

```c
//user/sh.c

static int history_count; //记录history文件里有多少条指令
static char history_buf[25][1030];

void runcmd(char *s){
    /*...*/
    if(strcmp(argv[0], "history") == 0){ //实现内置shell命令，即shell built-in command
		history();
		return;
	}
}

void readline(char *buf, u_int n) {
	int r;
	int fd; //.mosh_history的文件描述符
	if((fd = open("/.mosh_history", O_TRUNC | O_WRONLY)) < 0){ //第一次打开不了创建一个
		r = create("/.mosh_history", FTYPE_REG);
		if((fd = open("/.mosh_history", O_TRUNC | O_WRONLY)) < 0){ //用覆盖模式
			debugf("cannot create .mosh_history\n");
			exit(1);
		}
	}
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit(1);
		}
		if (buf[i] == '\b' || buf[i] == 0x7f) {
			if (i > 0) {
				i -= 2;
			} else {
				i = -1;
			}
			if (buf[i] != '\b') {
				printf("\b");
			}
		}
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = '\0';
			if(history_count < 20){ //还没存到20条指令
				strcpy(history_buf[history_count], buf); //把这条指令拷贝进数组
				history_count++; //存了多少条更新
			}else{ //存了20条了，要实现覆盖
				for(int j=0; j<20; j++){
					strcpy(history_buf[j], history_buf[j+1]); //往前覆盖就行了
				}
				strcpy(history_buf[19], buf); //把最后这条拿进去
			}
			for(int j=0; j<history_count; j++){ //读进去history_count条指令就行
				int len = strlen(history_buf[j]);
				if((r = write(fd, history_buf[j], len)) != len){
					debugf("write error\n");
					exit(1);
				}
			}
			buf[i] = 0;
			return;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

void history(){
	int fd;
	fd = open("/.mosh_history", O_RDONLY); //写的行为在readline完成，这里只用来读出.mosh_history的内容
	if(fd < 0){ //应该是不可能发生的情况
		debugf(".mosh_history is not created!\n");
		exit();
	}
	for(int i = 0; i < history_count; i++){
		debugf("%s\n",history_buf[i]);
	}
	close(fd); //关闭文件*/
}

```



### 实现前后台任务管理

想法是内核管理任务数量，通过系统调用实现任务管理。于是构建一系列通信函数。

- jobs

```c
//include/syscall.h

enum {
	SYS_putchar,
	SYS_print_cons,
	SYS_getenvid,
	SYS_yield,
	SYS_env_destroy,
	SYS_set_tlb_mod_entry,
	SYS_mem_alloc,
	SYS_mem_map,
	SYS_mem_unmap,
	SYS_exofork,
	SYS_set_env_status,
	SYS_set_trapframe,
	SYS_panic,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_cgetc,
	SYS_write_dev,
	SYS_read_dev,
	SYS_add_job,
	SYS_list_job,
	SYS_finish_job,
	SYS_kill_job,
	SYS_wait,
	SYS_exit,
	MAX_SYSNO,
};
```

```c
//kern/syscall_all.c
int jobs_count; //任务数量
struct Job{
	int job_id;
	char job_status[20];
	int env_id;
	char cmd[1024];
}; //任务的各种属性
struct Job jobs[20]; //任务表

int sys_add_job(u_int envid, char *cmd){
	jobs[jobs_count].job_id = jobs_count + 1;
	strcpy(jobs[jobs_count].job_status, "Running");
	jobs[jobs_count].env_id = envid;
	strcpy(jobs[jobs_count].cmd, cmd);
	jobs_count++;
	return 0;
}

int sys_list_job(){
	for(int i=0; i<jobs_count; i++){
		printk("[%d] %-10s 0x%08x %s\n", jobs[i].job_id, jobs[i].job_status, jobs[i].env_id, jobs[i].cmd);
	}
	return 0;
}

int sys_finish_job(u_int envid){
	//printk("my envid is %x\n", envid);
	for(int i=0; i<jobs_count; i++){
		if(jobs[i].env_id == envid){
			strcpy(jobs[i].job_status, "Done");
			break;
		}
	}
	return 0;
}

void *syscall_table[MAX_SYSNO] = {
    [SYS_putchar] = sys_putchar,
    [SYS_print_cons] = sys_print_cons,
    [SYS_getenvid] = sys_getenvid,
    [SYS_yield] = sys_yield,
    [SYS_env_destroy] = sys_env_destroy,
    [SYS_set_tlb_mod_entry] = sys_set_tlb_mod_entry,
    [SYS_mem_alloc] = sys_mem_alloc,
    [SYS_mem_map] = sys_mem_map,
    [SYS_mem_unmap] = sys_mem_unmap,
    [SYS_exofork] = sys_exofork,
    [SYS_set_env_status] = sys_set_env_status,
    [SYS_set_trapframe] = sys_set_trapframe,
    [SYS_panic] = sys_panic,
    [SYS_ipc_try_send] = sys_ipc_try_send,
    [SYS_ipc_recv] = sys_ipc_recv,
    [SYS_cgetc] = sys_cgetc,
    [SYS_write_dev] = sys_write_dev,
    [SYS_read_dev] = sys_read_dev,
	[SYS_add_job] = sys_add_job,
	[SYS_list_job] = sys_list_job,
	[SYS_finish_job] = sys_finish_job,
	[SYS_kill_job] = sys_kill_job,
};
```

```c
//user/include/lib.h
int syscall_add_job(u_int envid, char *cmd);
int syscall_list_job();
int syscall_finish_job(u_int envid);
```

```c
//user/lib/syscall_lib.c
int syscall_add_job(u_int envid, char *cmd){
	return msyscall(SYS_add_job, envid, cmd);
}

int syscall_list_job(){
	return msyscall(SYS_list_job);
}

int syscall_finish_job(u_int envid){
	return msyscall(SYS_finish_job, envid);
}

```

接下来是任务的具体实现。

- 修改了`parsecmd`函数，新增了`case '&'`的判断，与`;`的处理逻辑差不多，取消`wait`即可实现并发执行了。注意，我们每一次遇到后台任务的时候，都在执行任务前将这个任务通过系统调用`syscall_add_job`将这个任务加入到内核管理的任务表里面。这里需要传任务的具体名称，因为buf的定义在很下面，无法使用，所以使用了一个备份buf，名为`copy_buf`，专用于系统调用的传参。这样就在`runcmd`之前先知道了任务的名称。

```c
//user/sh.c
static char copy_buf[1024]; //buf的备份，专供系统调用使用
int parsecmd(char **argv, int *rightpipe){
    case '&':
			child = fork();
			if(child == 0){ //不用wait即并行，实现前后台，此处是后台任务
				syscall_add_job(syscall_getenvid(), copy_buf);
				return argc;
			}else if(child > 0){
				dup(opencons(), 1);dup(1,0);
				return parsecmd(argv,rightpipe);
			}
			break;
}

int main(int argc, char **argv){
    /*...*/
    readline(buf, sizeof buf);
	strcpy(copy_buf, buf);
}
```

- 任务什么时候结束？在exit()里结束，因此我们设置任务的状态为`Done`是在exit前进行。据此找到`user/lib/libos.c`函数，在`syscall_env_destroy(0)`销毁进程前先系统调用`syscall_finish_job`修改内核管理的任务表的任务状态。

```c
//user/lib/libos.c
void exit(int value) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif
	syscall_exit(value);
	syscall_finish_job(syscall_getenvid());
	syscall_env_destroy(0);
	user_panic("unreachable code");
}
```

- 接下来实现内置命令，使通过输入`jobs`就能查询当前后台的任务状态。同`history`的实现差不多。在`runcmd`里进行字符串比较，看是不是`jobs`这个命令，如果不是就走`spawn`那条路径，否则就执行`jobs`函数。

```c
//user/sh.c
void jobs(){ 
	syscall_list_job();
}

void runcmd(char *s){
    if(strcmp(argv[0], "jobs") == 0){ //shell built-in command
		jobs();
		return;
	}
}
```



- kill

kill是指定任务结束，也需要实现进程与进程之间的通信。内核态的kill行为也很简单，通过传入的jobid看有不有想kill的job，没有输出错误信息，有的话，修改job的状态为`Done`并且杀死进程即可。

```c
//include/syscall.h

enum {
	/*...*/
	SYS_kill_job,
	MAX_SYSNO,
};
```

```c
//kern/syscall_all.c
int sys_kill_job(u_int jobid){
	for(int i=0; i<jobs_count; i++){
		if(jobs[i].job_id == jobid){
			if(strcmp(jobs[i].job_status, "Done") == 0){
				printk("fg: (0x%08x) not running\n", jobs[i].env_id);
			}else{
				strcpy(jobs[i].job_status, "Done");
				sys_env_destroy(jobs[i].env_id); //关掉该进程
			}
			return 0;
		}
	}
	printk("fg: job (%d) do not exist\n", jobid);
	return 1;
}

void *syscall_table[MAX_SYSNO] = {
    /*...*/
	[SYS_kill_job] = sys_kill_job,
};
```

```c
//user/include/lib.h
int syscall_kill_job(u_int jobid);
```

```c
//user/lib/syscall_lib.c

int syscall_kill_job(u_int jobid){
	return msyscall(SYS_kill_job, jobid);
}

```

kill同样是个内置命令，处理方式同`jobs`操作一样。

```c
//user/sh.c

void kill(int jobid){
	syscall_kill_job(jobid);
}

void runcmd(char *s) {
    if(strcmp(argv[0], "kill") == 0){
		int jobid = 0;
		for(int i=0; i<strlen(argv[1]); i++){
			jobid = 10*jobid + argv[1][i]-'0';
		}
		kill(jobid);
		return;
	}
}
```

写到这里，会发现后台任务跑的逻辑很奇怪，比如`sleep 1&;ls`，等待一会儿后输入`jobs`命令查看运行状态，会发现`sleep`居然还没有结束，事实上是因为在`kern/syscall.c`中有一个函数`int sys_cgetc(void)`中处理逻辑是没有输入的时候就忙等，等待输入，有输入了才开始执行进程。因此没有输入的时候，后台任务是运行部了的，所以要修改这里的忙等逻辑，使得后台能运行任务。修改逻辑也很简单，直接`return scancharc()`就行了。



### 实现指令条件执行

题目要求命令的执行有返回值。而在之前的实现中，执行就是执行了，没执行就直接退出了，而没有明确的返回值。这是我写shell最难的部分。参考了不少同学的做法。最终是采用了将进程的返回值保存在进程控制块的`env_exit_value`上，供父进程使用。

```c
//include/env.h
struct Env {
	/*...*/
	u_int env_exit_value; //exit的值
};

```

题目后续指令的执行都依据于前面指令的执行结果，因此是串行的，处理逻辑与`;`类似。即需要使用`wait`等待前面的进程执行完。因此，有个很自然的想法就是修改`wait`函数，使其能够返回子进程的执行结果，拿着这个结果决定本身指令是否该进行。所以`wait`是要去取子进程的`exit_value`的。所以又要实现进程与进程之间得通信，又要构建系统调用。

```c
//user/lib/wait.c

int wait(u_int envid) {
	const volatile struct Env *e;

	e = &envs[ENVX(envid)];
	while (e->env_id == envid && e->env_status != ENV_FREE) {
		syscall_yield();
	}
	return syscall_wait(envid); //取envid对应的进程控制块的exit_value
}

```

上面提到退出的时候要保存返回值，那么什么时候退出？又涉及到`user/lib/libos.c`文件了。其实所有用户进程（包括指令）都在`libos.c`的`libmain`里面的`main`进行调用的。因此这个函数也要有返回值。所以总结下来，就是exit的时候拿着本身的执行情况退出。因此也要修改`exit`的逻辑。

```c
//user/lib/libos.c
#include <env.h>
#include <lib.h>
#include <mmu.h>

void exit(int value) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif
	syscall_exit(value);
	syscall_finish_job(syscall_getenvid());
	syscall_env_destroy(0);
	user_panic("unreachable code");
}

const volatile struct Env *env;
extern int main(int, char **);

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	int value = main(argc, argv);
	// exit gracefully
	exit(value);
}

```

基本逻辑理清楚了，剩下的事情就是固定化的系统调用时间了。

```c
//include/syscall.h
enum {
	/*...*/
	SYS_wait,
	SYS_exit,
	MAX_SYSNO,
};
```

```c
//kern/syscall_all.c
extern struct Env envs[NENV];

int sys_wait(u_int envid){
	struct Env *e = &envs[ENVX(envid)];
	if(e->env_status == ENV_FREE){
		return e->env_exit_value;
	}
	return -1;
}

void sys_exit(int value){
	curenv->env_exit_value = value;
}

void *syscall_table[MAX_SYSNO] = {
    /*...*/
	[SYS_wait] = sys_wait,
	[SYS_exit] = sys_exit,
};
```

```c
//user/include/lib.h

void exit(int value) __attribute__((noreturn));

int syscall_wait(u_int envid);
void syscall_exit(int value);

int wait(u_int envid);
```

```c
//user/lib/syscall_lib.c
int syscall_wait(u_int envid){
	return msyscall(SYS_wait, envid);
}

void syscall_exit(int value){
	msyscall(SYS_exit, value);
}

```

值得注意的是，一旦修改了`exit`意味着之前的文件带有`exit`的全部都需要修改，表示拿回去的值到底是多少，以供条件指令执行。

最后一步就是修改`sh.c`的文件，使得条件指令执行能够按照我们所想的正常跑起来。有了之前任务的经验，套路化完成即可。

```c
//user/sh.c

int run = 1; //下一条命令是否运行
int flag = 0; //记录上一条的运行情况是true还是false

int _gettoken(char *s, char **p1, char **p2){
    if (strchr(SYMBOLS, *s)) {
		int t = *s;
		if(t == '>' && *(s+1) == '>'){ //识别到追加重定向
			*p1 = s;
			*s = 0;
			s = s + 2;
			*p2 = s;
			return 'p'; //repeat,追加重定向的标志
		}
		if(t == '|' && *(s+1) =='|'){
			*p1 = s;
			*s = 0;
			s = s + 2;
			*p2 = s;
			return 'o'; //or，表示||
		}
		if(t == '&' && *(s+1) == '&'){
			*p1 = s;
			*s = 0;
			s = s + 2;
			*p2 = s;
			return 'a'; //and，表示&&
		}
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

}

int parsecmd(char **argv, int *rightpipe) {
    /*...*/
    int c = gettoken(0, &t); //不再return 0，根据解析情况返回后续parse值
		if(c == 0){ //提前终止，否则会一直continue
			return argc;
		}
		if(!run){ //这条指令不能run直接跳过,相当于这条指令不存在，但要考虑后续的影响
			if(c == 'a' && flag == 0){
				run = 1;
			}else if(c == 'o' && flag == 1){
				run = 1;
			}
			continue; //跳过本次指令
		}
    /*...*/
    case 'a':
			child = fork();
			if(child == 0){ //&&左侧的命令运行
				return argc;
			}else if(child > 0){
				flag = wait(child); //&&左侧命令运行完了返回结果
				// debugf("it's flag:%d\n",flag);
				close(0);close(1);
				dup(opencons(), 1);dup(1,0);
				if(flag == 0){ //&&左侧返回0，下条指令可以执行
					run = 1;
				}else{
					run = 0;
				}
				return parsecmd(argv, rightpipe);
			}
			break;
		case 'o':
			child = fork();
			if(child == 0){ //||左侧的命令运行
				return argc;
			}else if(child > 0){
				flag = wait(child); //||左侧命令运行完了返回结果
				close(0);close(1);
				dup(opencons(), 1);dup(1,0);
				if(flag == 0){ //||左侧返回0，下条指令不可以执行
					run = 0;
				}else{
					run = 1;
				}
				return parsecmd(argv, rightpipe);
			}
			break;
}
```

