#pragma once
#include<iostream>
using namespace std;
/*
 * 缓存控制块buf定义
 * 记录了相应缓存的使用情况等信息；
 * 同时兼任I/O请求块，记录该缓存
 * 相关的I/O请求和执行结果。
 * 
 本文件系统只实现了单用户的单进程，故缓存队列只设置了一个缓存队列，也无需判断是否需要重用了
 */

class Buf
{
public:
	/* b_flags中标志位 
	只用到了两个标志，B_DONE和B_DELWRI，分别表示已经完成IO和延迟写的标志。
	*/
	enum BufFlag	
	{
		B_DONE = 0x4,			/* I/O操作结束 */
		B_DELWRI = 0x80			/* 延迟写，在相应缓存要移做他用时，再将其内容写到相应块设备上 */
	};

	unsigned int b_flags;	/* 缓存控制块标志位 */

	int		padding;		/* 4字节填充，使得b_forw和b_back在Buf类中与Devtab类
							 * 中的字段顺序能够一致，否则强制转换会出错。 */
	/* 缓存控制块队列勾连指针 */
	Buf* b_forw;
	Buf* b_back;
		
	int		b_wcount;		/* 需传送的字节数 */
	unsigned char* b_addr;	/* 指向该缓存控制块所管理的缓冲区的首地址 */
	int		b_blkno;		/* 磁盘的逻辑块号 0开始 */
	int		no;				/* 加入一个number记录buf对象的标号 */


	Buf()
	{
		b_flags = 0;
		b_forw = NULL;
		b_back = NULL;
		b_wcount = 0;
		b_addr = NULL;
		b_blkno = -1;
		no = 0;
	}

	//清空缓存块
	void ClearBuf() {
		b_flags = 0;
		b_forw = NULL;
		b_back = NULL;
		b_wcount = 0;
		b_addr = NULL;
		b_blkno = -1;
		no = 0;
	}

};