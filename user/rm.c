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