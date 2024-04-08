#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
using namespace std;

/* DiskDriver类负责管理磁盘，因本文件系统只管理一个设备磁盘，故将UnxiV6++中磁盘管理
中需要的部分整理成该类 */


class DiskDriver {
public:				
	
	FILE* diskFilePointer;	//指向磁盘的文件指针

public:
	DiskDriver();

	~DiskDriver();

	//创建镜像文件，若之前已有，则会覆盖之。
	void CreateDiskFile();

	//重新打开磁盘文件
	//void getClean();

	//检查磁盘镜像文件是否存在
	bool isMounted();
	

	//实现写磁盘文件
	void DiskWrite(const void* ptr, size_t size, int offset = -1, size_t whence = SEEK_SET);

	//实现读磁盘文件
	void DiskRead(void* ptr, size_t size, int offset = -1, size_t whence = SEEK_SET);

};