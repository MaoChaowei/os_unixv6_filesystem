#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include"INode.h"
/* 该文件包括打开文件控制块File类、进程打开文件描述符表OpenFiles，以及IO参数类的声明 */

/* 打开文件控制块File类。
 * 该结构记录了进程打开文件的读、写请求类型，文件读写位置等等。 */
class File
{
public:
	enum FileFlags
	{
		FREAD = 0x1,			/* 读请求类型 */
		FWRITE = 0x2,			/* 写请求类型 */
	};

	unsigned int f_flag;		/* 对打开文件的读、写操作要求 */
	int		f_count;			/* 当前引用该文件控制块的进程数量，=0表示该File对象空闲，可分配 */
	Inode* f_inode;				/* 指向打开文件的内存Inode指针 */
	int		f_offset;			/* 文件读写位置指针 */
	
public:
	File() {
		f_count = 0;
		f_inode = NULL;
		f_offset = 0;
	}
	~File() {
		//nothing to do here
	}
	void Reset() {
		f_count = 0;
		f_inode = NULL;
		f_offset = 0;
	}

};


/* 进程打开文件描述符表(OpenFiles)的定义
 * 进程的u结构中包含OpenFiles类的一个对象，
 * 维护了当前进程的所有打开文件。
 * ps:虽然本实验只考虑单进程，但还是参考v6++的方式，
 * 保留了进程的打开文件表 */

class OpenFiles
{
public:
	static const int MAX_FILES = 100;			/* 进程允许打开的最大文件数 */

private:
	File* ProcessOpenFileTable[MAX_FILES];		/* File对象的指针数组，指向系统打开文件表中的File对象 */

public:
	OpenFiles();
	~OpenFiles();
	void Reset();

	//进程请求打开文件时，在打开文件描述符表中分配一个空闲表项
	int AllocFreeSlot();
	//根据用户系统调用提供的文件描述符参数fd，找到对应的打开文件控制块File结构
	File* GetF(int fd);
	//为已分配到的空闲描述符fd和已分配的打开文件表中空闲File对象建立勾连关系
	void SetF(int fd, File* pFile);

};

/* 文件I/O的参数类
 * 对文件读、写时需用到的读、写偏移量、
 * 字节数以及目标区域首地址参数。 */
class IOParameter
{
	/* Members */
public:
	unsigned char* m_Base;	/* 当前读、写用户目标区域的首地址 */
	int m_Offset;	/* 当前读、写文件的字节偏移量 */
	int m_Count;	/* 当前还剩余的读、写字节数量 */

	/* Functions */
public:
	IOParameter() {
		m_Base = 0;
		m_Offset = 0;
		m_Count = 0;
	}
	~IOParameter() {
		//nothing to do here
	}
};
