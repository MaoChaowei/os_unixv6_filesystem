#pragma once

#include "INode.h"
#include "File.h"
#include "FileSystem.h"

/* (系统的)打开文件管理类(OpenFileManager)负责内核中对打开文件机构的管理，为进程
 * 打开文件建立内核数据结构之间的勾连关系。
 * 勾连关系指进程打开文件描述符指向打开文件表中的File打开文件控制结构，
 * 以及从File结构指向文件对应的内存Inode。*/

class OpenFileTable
{
public:
	static const int MAX_FILES = 100;	/* 打开文件控制块File结构的数量 */

	File m_File[MAX_FILES];				/* 系统打开文件表，为所有进程共享，进程打开文件描述符表
									中包含指向打开文件表中对应File结构的指针。*/
public:
	OpenFileTable();
	~OpenFileTable();

	void Reset();
	File* FAlloc();					//在系统打开文件表中分配一个空闲的File结构
	void CloseF(File* pFile);		//对打开文件控制块File结构的引用计数f_count减1，若引用计数f_count为0，则释放File结构。
	
};

/*
 * 内存Inode表(class InodeTable)负责内存Inode的分配和释放。
 */
class InodeTable
{
public:
	static const int MAX_INODE = 100;	/* 内存Inode的数量 */

public:
	Inode m_Inode[MAX_INODE];		/* 内存Inode数组，每个打开文件都会占用一个内存Inode */
	FileSystem* m_FileSystem;	/* 对全局对象g_FileSystem的引用 */

public:
	InodeTable();
	~InodeTable();
	void Reset();
	
	Inode* IGet(int inumber);			/* 根据外存Inode编号获取对应Inode。
										* 如果该Inode已经在内存中，对其上锁并返回该内存Inode，
										* 如果不在内存中，则将其读入内存后返回该内存Inode*/

	void IPut(Inode* pNode);			/* 减少该内存Inode的引用计数，如果此Inode已经没有目录项指向它，
										* 且无进程引用该Inode，则释放此文件占用的磁盘块。*/

	void UpdateInodeTable();			/*将所有被修改过的内存Inode更新到对应外存Inode中*/

	int IsLoaded(int inumber);			/*检查编号为inumber的外存inode是否有内存拷贝，如果有则返回该内存Inode在内存Inode表中的索引 */
	
	Inode* GetFreeInode();				/*在内存Inode表中寻找一个空闲的内存Inode*/

};

