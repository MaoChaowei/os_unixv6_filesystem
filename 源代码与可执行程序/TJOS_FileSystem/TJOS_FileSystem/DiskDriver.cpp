#include"DiskDriver.h"
/* ���̾����ļ��� */
const char fileName[] = "mcwDisk.img";
DiskDriver::DiskDriver()
{
	diskFilePointer = fopen(fileName,"rb+");//��д��ʽ���ļ��������ڲ����½�
	if (!diskFilePointer) {
		cout << "ERR:" << fileName << "�����ڣ���ʧ�ܡ���DiskDriver��" << endl;
	}
}

DiskDriver::~DiskDriver()
{
	if (!diskFilePointer) {
		fclose(diskFilePointer);
	}
}

//���������ļ�����֮ǰ���У���Ḳ��֮��
void DiskDriver::CreateDiskFile()
{
	diskFilePointer = fopen(fileName,"wb+");
	if (!diskFilePointer) {
		cout << "�����ļ�����ʧ�ܣ���CreateDiskFile��" << endl;
	}
}


//�����̾����ļ��Ƿ����
bool DiskDriver::isMounted()
{
	return diskFilePointer!=NULL;
}


//ʵ��д�����ļ�
void DiskDriver::DiskWrite(const void* ptr, size_t size, int offset, size_t whence)
{
	if (offset >= 0) {
		fseek(diskFilePointer,offset,whence);
	}
	fwrite(ptr,size,1,diskFilePointer);
}

//ʵ�ֶ������ļ�
void DiskDriver::DiskRead(void* ptr, size_t size, int offset, size_t whence)
{
	if (offset >= 0) {
		fseek(diskFilePointer, offset, whence);
	}
	fread(ptr, size, 1, diskFilePointer);
}
