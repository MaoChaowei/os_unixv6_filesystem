#pragma once
#include<iostream>
#include"Buf.h"
#include"DiskDriver.h"
#include<map>

using namespace std;

/* BufManager向上层文件系统提供统一的访问catch的接口，间接调用diskdriver实现 */
class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 100;		// 缓存控制块、缓冲区的数量 
	static const int BUFFER_SIZE = 512;	// 缓冲区大小。 以字节为单位

private:
	Buf bFreeList;						// 缓存自由队列 
	Buf mBuf[NBUF];						// 缓存控制块数组 
	unsigned char Buffer[NBUF][BUFFER_SIZE];// 缓冲区数组 
	DiskDriver* diskDriver;				// 指向磁盘管理器的全局对象
	map<int, Buf*> bufMap;				// 登记buf和其关联的blkno的map，用于快速查找是否可重用 

public:
	BufferManager();
	~BufferManager();
	Buf* GetBlk(int blkno);				// 申请一块缓存，用于读写设备上的字符块blkno。
	void Brelse(Buf* bp);				// 释放缓存控制块buf 
	Buf* Bread(int blkno);				// 读一个磁盘块。blkno为目标磁盘块逻辑块号。 
	void Bwrite(Buf* bp);				// 写一个磁盘块 
	void Bdwrite(Buf* bp);				// 延迟写磁盘块 
	void ClrBuf(Buf* bp);				// 清空缓冲区内容 
	void Bflush();						// 将队列中延迟写的缓存全部输出到磁盘 
	void FormatBuffer();				// 格式化所有Buffer 

private:
	void InitBufList();					// 缓存队列和缓存块初始化 
	void NotAvail(Buf* bp);				// 从缓存队列中摘下指定的缓存控制块buf 
	void ListAppend(Buf* bp);			// 在缓存队列尾加入新buf 
};