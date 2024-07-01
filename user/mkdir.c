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