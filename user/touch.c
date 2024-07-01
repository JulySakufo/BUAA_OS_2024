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