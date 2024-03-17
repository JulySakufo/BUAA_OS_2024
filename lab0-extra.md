# lab0-extra

基本是个信息题，现学现做的。大概如下：给了一系列概念，然后通过输入不同的参数执行不同的效果，命令的基本格式是`./handle-ps.sh -f <filename> [-s] [-q <CMD>] [-p <PID>]`，然后给了个文件让本地调试结果看对不对。文件名为ps.out

一共是三问，第一问就是根据给出来的sort用法现用即可，第二问就是个简单的grep查询，没什么太多说的。

第三问大概意思是递归查找一个PID的所有先辈进程的ID，参考ps.out，大概意思就是比如：

第二行是293，那么现在执行`./handle-ps.sh -f ps.out -p 293`后，先打印293的父进程ID228，然后找到本身进程为228的，打印它的父进程，得到227，然后找到本身进程为227的，打印它的父进程，如此下去直到得到父进程的ID为0截止。

给出的样例为

```shell
./handle-ps.sh -f ps.out -p 293
```

得到的结果为

```shell
228
227
217
1
0
```

本人卡在第三问了，主要是提示太抽象了并且指导书上对awk的介绍也几乎等于没有。上机的时候给的提示是`awk -v input=$1 'BEGIN {print input}'`，这个BEGIN太恶心了，然后参考往届代码也花了很多时间想代码是不是一行能写完的，**最后得出结论，这些代码没有规定一定是一行能写完的！**

想要完成第三问，你需要掌握：

- 反引号的用法，实现对变量的赋值
- awk的判定条件该怎么写，$在print里面的含义等等

现在上完机回过头来看，觉得extra其实也并不是很难，所以如果你能掌握以上两点，那么我觉得ak问题也不大。

本人修正后的最后代码如下：

```shell
#!/bin/bash

FILE=""
CMD=""
PID=""
SORT=false

function usage() {
    echo "USAGE:"
    echo "  ./handle-ps.sh -f <filename> [-s] [-q <CMD>] [-p <PID>]"
    echo "OPTIONS:"
    echo "  -f: input file"
    echo "  -s: sort by priority and pid"
    echo "  -q: query processes with queried command"
    echo "  -p: get process's ancestors"
}

while getopts "f:q:p:sh" opt; do
    case $opt in
        f) FILE=${OPTARG} ;;
        q) CMD=${OPTARG} ;;
        p) PID=${OPTARG} ;;
        s) SORT=true ;;
        h) usage; exit 0 ;;
        ?) usage; exit 1 ;;
    esac
done

if [ -z $FILE ]; then
    usage
    exit 1
fi

# You can remove ":" after finishing.
if $SORT; then
    # Your code here. (1/3)
    sort -k4nr -k2n $2
elif [ ! -z "$CMD" ]; then
    # Your code here. (2/3)
    grep $CMD $2
elif [ ! -z $PID ]; then
    # Your code here. (3/3)
    while [ $PID != 0 ]
    do
    	awk -v id=$PID 'id==$2 {print $3}' $FILE
		PID=`awk -v id=$PID 'id==$2 {print $3}' $FILE`
	done
else
    usage
    exit 1
fi
```

