#include <print.h>

/* forward declaration */
static void print_char(fmt_callback_t, void *, char, int, int);
static void print_str(fmt_callback_t, void *, const char *, int, int);
static void print_num(fmt_callback_t, void *, unsigned long, int, int, int, int, char, int);

void vprintfmt(fmt_callback_t out, void *data, const char *fmt, va_list ap) {
	char c;
	const char *s;
	long num;

	int width;
	int long_flag; // output is long (rather than int)
	int neg_flag;  // output is negative
	int ladjust;   // output is left-aligned
	char padc;     // padding char
	int neg_flag1;
	int neg_flag2;
	long x;
	long y;
	long z;
	for (;;) {
		/* scan for the next '%' */
		/* Exercise 1.4: Your code here. (1/8) */
		while(*fmt!='%' && *fmt!='\0'){
			out(data, fmt, 1);
			fmt++;
		}
		/* flush the string found so far */
		/* Exercise 1.4: Your code here. (2/8) */

		/* check "are we hitting the end?" */
		/* Exercise 1.4: Your code here. (3/8) */
		if(*fmt=='\0'){ //hitting the end
			break;
		}
		/* we found a '%' */
		/* Exercise 1.4: Your code here. (4/8) */
		if(*fmt=='%'){
			fmt++;//移动到下一个位置
		}
		/* check format flag */
		/* Exercise 1.4: Your code here. (5/8) */
		ladjust = 0;
		padc = ' ';//默认空格
		if(*fmt=='-'){ //右对齐，否则默认左对齐
			fmt++;
			ladjust=1;
		}
		if(*fmt=='0'){
			fmt++;
			padc = '0';//用0填充
		}
		/* get width */
		/* Exercise 1.4: Your code here. (6/8) */
		width = 0;//长度的初始化 
		while(*fmt>='0' && *fmt<='9'){ //计算长度
			width = 10*width + *fmt - '0';
			fmt++;
		}
		/* check for long */
		/* Exercise 1.4: Your code here. (7/8) */
		long_flag = 0;//标记的初始化
		if(*fmt=='l'){ //为long型
			long_flag = 1;
			fmt++;
		}

		neg_flag = 0;
		switch (*fmt) {
		case 'b':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 2, 0, width, ladjust, padc, 0);
			break;

		case 'd':
		case 'D':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			if(num < 0){
				neg_flag = 1;//是负数
				num = -num;//num是unsigned，先取绝对值再说
			}
			print_num(out, data, num, 10, neg_flag, width, ladjust, padc, 0);
			/*
			 * Refer to other parts (case 'b', case 'o', etc.) and func 'print_num' to
			 * complete this part. Think the differences between case 'd' and the
			 * others. (hint: 'neg_flag').
			 */
			/* Exercise 1.4: Your code here. (8/8) */

			break;

		case 'o':
		case 'O':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 8, 0, width, ladjust, padc, 0);
			break;

		case 'u':
		case 'U':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 10, 0, width, ladjust, padc, 0);
			break;

		case 'x':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 16, 0, width, ladjust, padc, 0);
			break;

		case 'X':
			if (long_flag) {
				num = va_arg(ap, long int);
			} else {
				num = va_arg(ap, int);
			}
			print_num(out, data, num, 16, 0, width, ladjust, padc, 1);
			break;

		case 'c':
			c = (char)va_arg(ap, int);
			print_char(out, data, c, width, ladjust);
			break;

		case 's':
			s = (char *)va_arg(ap, char *);
			print_str(out, data, s, width, ladjust);
			break;

		case 'P':
			neg_flag1 = 0;
			neg_flag2 = 0;
			if(long_flag){
				x = va_arg(ap, long int);
				y = va_arg(ap, long int);
				z = (x+y) * (x-y);
				z = z<0? -z : z;
			}else{
				x = va_arg(ap,int);
				y = va_arg(ap,int);
				z = (x+y) * (x-y);
				z = z<0? -z:z;
			}
				out(data,"(",1);//print '('
				if(x<0){
				        neg_flag1 = 1;
					x = -x;
				}
				print_num(out,data,x,10,neg_flag1,width,ladjust,padc,0);
				out(data,",",1);//print','
				if(y<0){
					neg_flag2 = 1;
					y = -y;
				}
				print_num(out,data,y,10,neg_flag2,width,ladjust,padc,0);
				out(data,",",1);//print ','
				print_num(out,data,z,10,0,width,ladjust,padc,0);
				out(data,")",1);//print ')'
				break;
		case '\0':
			fmt--;
			break;

		default:
			/* output this char as it is */
			out(data, fmt, 1);
		}
		fmt++;
	}
}

/* --------------- local help functions --------------------- */
void print_char(fmt_callback_t out, void *data, char c, int length, int ladjust) {
	int i;

	if (length < 1) {
		length = 1;
	}
	const char space = ' ';
	if (ladjust) {
		out(data, &c, 1);
		for (i = 1; i < length; i++) {
			out(data, &space, 1);
		}
	} else {
		for (i = 0; i < length - 1; i++) {
			out(data, &space, 1);
		}
		out(data, &c, 1);
	}
}

void print_str(fmt_callback_t out, void *data, const char *s, int length, int ladjust) {
	int i;
	int len = 0;
	const char *s1 = s;
	while (*s1++) {
		len++;
	}
	if (length < len) {
		length = len;
	}

	if (ladjust) {
		out(data, s, len);
		for (i = len; i < length; i++) {
			out(data, " ", 1);
		}
	} else {
		for (i = 0; i < length - len; i++) {
			out(data, " ", 1);
		}
		out(data, s, len);
	}
}

void print_num(fmt_callback_t out, void *data, unsigned long u, int base, int neg_flag, int length,
	       int ladjust, char padc, int upcase) {
	/* algorithm :
	 *  1. prints the number from left to right in reverse form.
	 *  2. fill the remaining spaces with padc if length is longer than
	 *     the actual length
	 *     TRICKY : if left adjusted, no "0" padding.
	 *		    if negtive, insert  "0" padding between "0" and number.
	 *  3. if (!ladjust) we reverse the whole string including paddings
	 *  4. otherwise we only reverse the actual string representing the num.
	 */

	int actualLength = 0;
	char buf[length + 70];
	char *p = buf;
	int i;

	do {
		int tmp = u % base;
		if (tmp <= 9) {
			*p++ = '0' + tmp;
		} else if (upcase) {
			*p++ = 'A' + tmp - 10;
		} else {
			*p++ = 'a' + tmp - 10;
		}
		u /= base;
	} while (u != 0);

	if (neg_flag) {
		*p++ = '-';
	}

	/* figure out actual length and adjust the maximum length */
	actualLength = p - buf;
	if (length < actualLength) {
		length = actualLength;
	}

	/* add padding */
	if (ladjust) {
		padc = ' ';
	}
	if (neg_flag && !ladjust && (padc == '0')) {
		for (i = actualLength - 1; i < length - 1; i++) {
			buf[i] = padc;
		}
		buf[length - 1] = '-';
	} else {
		for (i = actualLength; i < length; i++) {
			buf[i] = padc;
		}
	}

	/* prepare to reverse the string */
	int begin = 0;
	int end;
	if (ladjust) {
		end = actualLength - 1;
	} else {
		end = length - 1;
	}

	/* adjust the string pointer */
	while (end > begin) {
		char tmp = buf[begin];
		buf[begin] = buf[end];
		buf[end] = tmp;
		begin++;
		end--;
	}

	out(data, buf, length);
}