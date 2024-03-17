# lab0-exam

> exam题目回忆：
>
> 1. 创建一个名为`test`的目录
> 2.  将`code`目录拷贝到test目录下
> 3.  在`test/code`目录下打印`14.c`的内容到标准输出
> 4.  将`test/code`目录下的`0.c 1.c ... 15.c`全部编译成对应名称的未链接的文件`.o`文件
> 5.  将`0.o 1.o ... 15.o`编译成可执行文件`hello`文件，将`hello`文件移动到`test`目录下
> 6.  将`hello`文件标准错误输出重定向到`err.txt`文件
> 7.  将`test`目录下的`err.txt`文件移动到上一级目录
> 8.  将当前目录下的`err.txt`文件的权限修改为`rw-r-xr-x`
> 9.  在执行`bash exam.sh`的时候有可能会有参数，参数的个数为[0,2]，格式为`bash exam.sh [n1] [n2]`，缺省值默认为1，(即直接执行`bash exam.sh`的时候默认n1=n2=1)，n=n1+n2，要求将err.txt的第n行标准错误输出重定向到控制台。提示给出了(>&2的方法) 

个人参考代码：

```
#1
mkdir test
#2
cp -r code test
#3
cd test/code
cat 14.c
#4
a=0
n=16
while [ $a -ne $n ]
do
	gcc -c $a.c -o $a.o
	let a=a+1
done
#5
gcc *.o -o hello
mv hello ..
#6
cd ..
./hello 2> err.txt
#7
mv err.txt ..
#8
cd ..
chmod 655 err.txt
#9
if [ $# -eq 0 ]
then
	sed -n '2p' err.txt >&2
elif [ $# -eq 1 ]
then
	let d=$1+1
	sed -n "${d}p" err.txt >&2
else
	let e=$1+$2
	sed -n "${e}p" err.txt >&2
fi	
```

**一些可能需要注意的地方：**

- **gcc的使用**

`-o`选项是**指定输出文件**的名称，**不是指生成了可执行文件！(上完机复盘发现不少同学都理解错了)**

- **chmod对权限的添加**

其实就是二进制编码，具有rwx的格式，可以认为r=4,w=2,x=1得到对应的数字即可，具体的形式可以去看下chmod权限添加的用法

- `$#`，`$?`等带`$`的使用要知道