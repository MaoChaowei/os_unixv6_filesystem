#include"File.h"
#include"User.h"

extern User g_user;


/*****************OPENFILES***********************/
OpenFiles::OpenFiles()
{
	memset(ProcessOpenFileTable,0,sizeof(ProcessOpenFileTable));//初始化进程打开文件描述符表
}
OpenFiles::~OpenFiles()
{
	//nothing to do here
}

void OpenFiles::Reset()
{
	memset(ProcessOpenFileTable, 0, sizeof(ProcessOpenFileTable));
};
//文件打开结构创建的一步：进程请求打开文件时，在进程的打开文件描述符表中分配一个空闲表项
int OpenFiles::AllocFreeSlot()
{
	for (int i = 0; i < OpenFiles::MAX_FILES; i++) {
		//进程打开文件描述符表中找到空闲项，则返回之
		if (!ProcessOpenFileTable[i]) {
			//设置核心栈现场保护区中的EAX寄存器的值，即系统调用返回值
			g_user.ar0[User::EAX] = i;
			return i;
		}
	}
		g_user.ar0[User::EAX] = -1;
		g_user.errorCode = User::U_EMFILE;
		return -1;
}

//根据用户系统调用提供的文件描述符参数fd，找到对应的打开文件控制块File结构
File* OpenFiles::GetF(int fd)
{
	File* fpointer;
	//判断fd是否合法
	if (fd < 0 || fd >= OpenFiles::MAX_FILES) {
		g_user.errorCode = User::U_EBADF;
		return NULL;
	}
	//从进程打开文件表中寻找指向系统打开文件表的FILE指针
	fpointer = ProcessOpenFileTable[fd];
	if (!fpointer) {
		g_user.errorCode = User::U_EBADF;
	}

	return fpointer;
}

//为已分配到的空闲描述符fd和已分配的打开文件表中空闲File对象建立勾连关系
void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::MAX_FILES)
		return;

	ProcessOpenFileTable[fd] = pFile;
	return;
}