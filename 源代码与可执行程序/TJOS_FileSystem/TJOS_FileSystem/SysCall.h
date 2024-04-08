#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include"INode.h"
#include"OpenFileManager.h"
using namespace std;

/* 对文件管理类(FileManager)的系统调用进行封装
 * 封装了文件系统的各种系统调用在核心态下处理过程，
 * 如对文件的Open()、Close()、Read()、Write()等等
 * 封装了对文件系统访问的具体细节。 */
class SysCall
{
public:
	enum DirSearchMode	//目录搜索模式,NameI()中函数用到
	{
		OPEN = 0,		//以打开文件方式搜索
		CREATE = 1,		//以新建文件方式搜索
		DELETE = 2		//以删除文件方式搜索
	};
	//数据成员

	Inode* rootDirInode;	//根目录内存INODE结点指针
	FileSystem* fileSystem;	//引用全局g_fileSystem
	InodeTable* inodeTable;	//引用全局g_inodeTable
	OpenFileTable* openFileTable;//引用全局g_openFileTable

	//成员函数

	SysCall();
	~SysCall();

	void Open();                          //Open()系统调用处理过程
	void Creat();                         //Creat()系统调用处理过程
	void Open1(Inode* pINode, int trf);   //Open()、Creat()系统调用的公共部分
	void Close();                         //Close()系统调用处理过程                  
	void Seek();                          //Seek()系统调用处理过程
	void Read();                          //Read()系统调用处理过程
	void Write();                         //Write()系统调用处理过程
	void Rdwr(enum File::FileFlags mode); //读写系统调用公共部分代码
	Inode* NameI(enum DirSearchMode mode);//目录搜索，将路径转化为相应的INode返回上锁后的INode
	Inode* MakNode(int mode);             //被Creat()系统调用使用，用于为创建新文件分配内核资源
	void UnLink();                        //取消文件
	void WriteDir(Inode* pINode);         //向父目录的目录文件写入一个目录项
	void ChDir();                         //改变当前工作目录
	void Ls();                            //列出当前INode节点的文件项
	void Rename(string ori, string cur);  //重命名文件、文件夹
};