#include"DiskDriver.h"
/* 磁盘镜像文件名 */
const char fileName[] = "mcwDisk.img";
DiskDriver::DiskDriver()
{
	diskFilePointer = fopen(fileName,"rb+");//读写方式打开文件，不存在不会新建
	if (!diskFilePointer) {
		cout << "ERR:" << fileName << "不存在，打开失败。（DiskDriver）" << endl;
	}
}

DiskDriver::~DiskDriver()
{
	if (!diskFilePointer) {
		fclose(diskFilePointer);
	}
}

//创建镜像文件，若之前已有，则会覆盖之。
void DiskDriver::CreateDiskFile()
{
	diskFilePointer = fopen(fileName,"wb+");
	if (!diskFilePointer) {
		cout << "磁盘文件创建失败！（CreateDiskFile）" << endl;
	}
}


//检查磁盘镜像文件是否存在
bool DiskDriver::isMounted()
{
	return diskFilePointer!=NULL;
}


//实现写磁盘文件
void DiskDriver::DiskWrite(const void* ptr, size_t size, int offset, size_t whence)
{
	if (offset >= 0) {
		fseek(diskFilePointer,offset,whence);
	}
	fwrite(ptr,size,1,diskFilePointer);
}

//实现读磁盘文件
void DiskDriver::DiskRead(void* ptr, size_t size, int offset, size_t whence)
{
	if (offset >= 0) {
		fseek(diskFilePointer, offset, whence);
	}
	fread(ptr, size, 1, diskFilePointer);
}
