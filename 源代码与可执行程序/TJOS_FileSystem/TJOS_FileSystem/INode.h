#pragma once

#include "Buf.h"

/*
 * 内存索引节点(INode)的定义
 * 系统中每一个打开的文件、当前访问目录都对应唯一的内存inode。
 */
class Inode
{
public:
	/* i_flag中标志位 */
	enum INodeFlag
	{
		IUPD = 0x2,		// 内存inode被修改过，需要更新相应外存inode 
		IACC = 0x4,		// 内存inode被访问过，需要修改最近一次访问时间 
	};

	/* static const member */
	//类的static成员具有如下特征：有独立的存储区，属于整个类，是类内部的静态局部，再加上const=》类内部的静态常变量。
	static const unsigned int IALLOC = 0x8000;		// 文件被使用 
	static const unsigned int IFMT = 0x6000;		// 文件类型掩码 
	static const unsigned int IFDIR = 0x4000;		// 文件类型：目录文件 
	static const unsigned int ILARG = 0x1000;		// 文件长度类型：大型或巨型文件 
	static const unsigned int IREAD = 0x100;		// 对文件的读权限 
	static const unsigned int IWRITE = 0x80;		// 对文件的写权限 

	static const int BLOCK_SIZE = 512;										// 文件逻辑块大小: 512字节 
	static const int ADDRESS_PER_INDEX_BLOCK = BLOCK_SIZE / sizeof(int);	// 每个间接索引表(或索引块)包含的物理盘块号 
	static const int SMALL_FILE_BLOCK = 6;									// 小型文件：直接索引表最多可寻址的逻辑块号 
	static const int LARGE_FILE_BLOCK = 128 * 2 + 6;						// 大型文件：经一次间接索引表最多可寻址的逻辑块号 
	static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;			// 巨型文件：经二次间接索引最大可寻址文件逻辑块号 
	
	/* Member */
	unsigned int i_flag;	// 状态的标志位，定义见enum INodeFlag 
	unsigned int i_mode;	// 文件工作方式信息 
	int		i_count;		// 引用计数 ps若等于0表示空闲
	int		i_nlink;		// 文件联结计数，即该文件在目录树中不同路径名的数量 
	int		i_number;		// 外存inode区中的编号  ps:从0开始计数,若=-1表示空闲 
	int		i_size;			// 文件大小，字节为单位 
	int		i_addr[10];		// 用于文件逻辑块号和物理块号转换的基本索引表 

public:
	Inode();
	~Inode();
	void Reset();

	//根据Inode对象中的物理磁盘块索引表，读取相应的文件数据
	void ReadI();
	//根据Inode对象中的物理磁盘块索引表，将数据写入文件
	void WriteI();
	//将文件的逻辑块号转换成对应的物理盘块号
	int Bmap(int lbn);

	//更新外存Inode的最后的访问时间、修改时间
	void IUpdate(int time);
	//释放Inode对应文件占用的磁盘块
	void ITrunc();
	// 清空Inode对象中的数据，但是不清除与外存inode连接的标志：i_flag\i_count\i_number
	void Clean();
	//将包含外存Inode字符块中信息拷贝到内存Inode中
	void ICopy(Buf* bp, int inumber);

};


/*
 * 外存索引节点(DiskINode)的定义，是磁盘中的INode
 * 外存Inode位于文件存储设备上的
 * 外存Inode区中。每个文件有唯一对应
 * 的外存Inode，其作用是记录了该文件
 * 对应的控制信息。
 * 外存Inode中许多字段和内存Inode中字段
 * 相对应。外存INode对象长度为64字节，
 * 每个磁盘块可以存放512/64 = 8个外存Inode
 * 注意：顺序不要改，因为后续用指针读取，不是按名读取
 */
class DiskInode
{
public:
	unsigned int d_mode;	/* 状态的标志位，定义见enum InodeFlag */
	int		d_nlink;		/* 文件联结计数，即该文件在目录树中不同路径名的数量 */
	int		d_size;			/* 文件大小，字节为单位 */
	int		d_addr[10];		/* 用于文件逻辑块好和物理块好转换的基本索引表 */
	int		d_atime;		/* 最后访问时间 */
	int		d_mtime;		/* 最后修改时间 */
	int		padding;		/* diskinode为64字节 */
public:
	DiskInode();
	~DiskInode();
};
