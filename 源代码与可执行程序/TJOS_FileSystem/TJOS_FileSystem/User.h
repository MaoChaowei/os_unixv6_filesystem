#pragma once
#include"SysCall.h"
#include<string>
using namespace std;

class User {
public:
	static const int EAX = 0;		//u.ar0[EAX]；访问现场保护区中EAX寄存器的偏移量

	// 错误码
	enum ErrorCode {
		U_NOERROR = 0,				//没有错误
		U_ENOENT = 1,				//找不到文件或者文件夹
		U_EBADF = 2,				//找不到文件句柄
		U_EACCES = 3,				//权限不足
		U_ENOTDIR = 4,				//文件夹不存在
		U_ENFILE = 5,				//文件表溢出
		U_EMFILE = 6,				//打开文件过多
		U_EFBIG = 7,				//文件过大
		U_ENOSPC = 8				//磁盘空间不足
	};

public:
	Inode* curDirIP;					//指向当前目录的Inode指针
	Inode* parDirIP;					//指向父目录的Inode指针
	DirectoryEntry curDirEnt;           //当前目录的目录项
	char dbuf[DirectoryEntry::DIRSIZ];	//当前路径分量
	string curDirPath;					//当前工作目录完整路径
	string dirp;						//系统调用参数(一般用于Pathname)的指针

/* 系统调用相关成员 */
	int arg[5];							//存放当前系统调用参数
	size_t ar0[5];                      /*指向核心栈现场保护区中EAX寄存器
									    存放的栈单元，本字段存放该栈单元的地址。
									    在V6中r0存放系统调用的返回值给用户程序*/
	ErrorCode errorCode;				//存放错误码

	OpenFiles ofiles;					//当前用户的进程打开文件描述符表对象
	IOParameter IOParam;				//记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数
	SysCall* sysCall;
	string ls;							//存放目录项

public:

	User();
	~User();

	void u_Ls();						//列目录
	void u_Cd(string dirName);			//移动到
	void u_MkDir(string dirName);		//创建目录
	void u_Create(string fileName);		//创建文件
	void u_Delete(string fileName);		//删除文件
	void u_Open(string fileName);		//打开文件
	void u_Close(string fd);			//关闭文件
	void u_Seek(string fd, string offset, string origin);//定位文件读写执政
	void u_Write(string fd, string inFile, string size);//写文件
	void u_Read(string fd, string outFile, string size);//读文件

	void u_Rename(string ori, string cur);  //重命名文件、文件夹
	void u_Tree(string path);               //打印树状目录

private:
	bool checkError();
	bool checkPathName(string path);
	void __userCd__(string dirName);
	void __userTree__(string path, int depth);//内部打印树状目录
};