#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()#"
static int history_count; //记录history文件里有多少条指令
static char history_buf[25][1030];
static char copy_buf[1024]; //buf的备份，专供系统调用使用
int run = 1; //下一条命令是否运行
int flag = 0; //记录上一条的运行情况是true还是false

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) { //将输入字符串s开头的所有空白字符去除，并将指针s移动到第一个非空白字符上。这样，后续的解析操作就可以从第一个非空白的字符开始，而不必担心字符串开头的空白字符会影响解析结果。
		*s++ = 0;
	}
	if (*s == 0) { //上面都是空白字符，结束，否则开始正式解析内容
		return 0;
	}

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

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) { //调用gettoken(0, &t)实际上是返回上一个解析到的类型，指令内容保存在t中，然后解析下一个token
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2); //进行下一次的解析
	return c; //返回上一个解析到的类型
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) { //解析用户输入的命令。通过调用 gettoken，将命令*解析成单个词法单元，保存在 argv 中*，并在遇到重定向、管道等特殊符号时做相应的特殊处理。当解析完一条命令包含的所有词法单元后，结束 parsecmd 函
//数，进入 runcmd 函数的命令执行阶段。
	int argc = 0; //词法单元的个数
	while (1) {
		char *t;
		int fd, r;
		int child;
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
		switch (c) {
		case 0:
			return argc;
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
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit(1);
			}
			argv[argc++] = t; //存了一个指令内容
			break;
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
		case '`': //t已经装好内容了
			runcmd(t); //echo 反引号的内容等价于直接执行后面的语句
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit(1);
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			if((fd = open(t, O_RDONLY)) < 0){
				debugf("failed to open '%s'\n", t);
				exit(1);
			}
			dup(fd, 0); //将fd的数据复制到标准输入里面去
			close(fd); //关闭原文件描述符
			//user_panic("< redirection not implemented");

			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit(1);
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			if((fd = open(t, O_WRONLY)) < 0){ //文件不存在
				if((r = create(t, FTYPE_REG)) < 0){ //创建文件
					debugf("failed to create '%s'\n", t);
					exit(1);
				}else{
					if((fd = open(t, O_WRONLY)) < 0){
						debugf("failed to open '%s'\n", t);
						exit(1);
					}
				}
			}
			dup(fd, 1); //将fd的数据复制到标准输出里面去
			close(fd);
			//user_panic("> redirection not implemented");

			break;
		case ';': //仿照|的处理逻辑
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
		case '#': //即遇到#就说明这一line解析完毕了，与0的行为是一样的
			return argc;
			break;
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
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			pipe(p);
			*rightpipe = fork();
			if(*rightpipe==0){ //子进程递归解析|右边的内容，直到处理完所有的|
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe); //argc是局部变量，覆盖的方式刷新argv
			} else if(*rightpipe > 0){ //父进程执行|左边的指令内容
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
			user_panic("| not implemented");

			break;
		}
	}

	return argc;
}

void jobs(){ 
	syscall_list_job();
}

void kill(int jobid){
	syscall_kill_job(jobid);
}

void runcmd(char *s) {
	gettoken(s, 0); //类似于do-while循环，先进来读一次，进行static变量的初始化
	int exit_value = 0;
	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe); //得到了指令的个数，正常不为0
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;
	if(strcmp(argv[0], "history") == 0){ //实现内置shell命令，即shell built-in command
		history();
		return;
	}
	if(strcmp(argv[0], "jobs") == 0){ //shell built-in command
		jobs();
		return;
	}
	if(strcmp(argv[0], "kill") == 0){
		int jobid = 0;
		for(int i=0; i<strlen(argv[1]); i++){
			jobid = 10*jobid + argv[1][i]-'0';
		}
		kill(jobid);
		return;
	}
//执行解析出的每一条命令。通过 argv 判断命令的种类，调用 spawn 产生子进程并装载命令对应的 ELF 文件，子进程执行命令，父进程在此处等待子进程结束后，结束进程。
	int child = spawn(argv[0], argv); //剩下的argv[1]...argv[n]是传进spawn进程的参数
	if(child < 0) { //找不到这个文件ls
		char prog[1024] = {0};
		strcpy(prog, argv[0]);
		int len = strlen(argv[0]);
		prog[len] = '.';
		prog[len + 1] = 'b';
		prog[len + 2] = '\0'; //将ls扩展成ls.b，去找.b文件即可
		child = spawn(prog, argv);
	}
	close_all();
	if (child >= 0) {
		exit_value = wait(child);
	} else {
		debugf("spawn %s: %d\n", argv[0], child); //没做修改之前一开始执行ls的时候会出现spawn ls: -10
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	exit(exit_value);
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

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit(1);
}

void history(){
	int fd;
	fd = open("/.mosh_history", O_RDONLY); //写的行为在readline完成，这里只用来读出.mosh_history的内容
	if(fd < 0){ //应该是不可能发生的情况
		debugf(".mosh_history is not created!\n");
		exit(1);
	}
	for(int i = 0; i < history_count; i++){
		debugf("%s\n",history_buf[i]);
	}
	close(fd); //关闭文件*/
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);
		strcpy(copy_buf, buf);
		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit(0);
		} else {
			wait(r);
		}
	}
	return 0;
}
