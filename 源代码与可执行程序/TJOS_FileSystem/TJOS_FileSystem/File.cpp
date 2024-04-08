#include"File.h"
#include"User.h"

extern User g_user;


/*****************OPENFILES***********************/
OpenFiles::OpenFiles()
{
	memset(ProcessOpenFileTable,0,sizeof(ProcessOpenFileTable));//��ʼ�����̴��ļ���������
}
OpenFiles::~OpenFiles()
{
	//nothing to do here
}

void OpenFiles::Reset()
{
	memset(ProcessOpenFileTable, 0, sizeof(ProcessOpenFileTable));
};
//�ļ��򿪽ṹ������һ��������������ļ�ʱ���ڽ��̵Ĵ��ļ����������з���һ�����б���
int OpenFiles::AllocFreeSlot()
{
	for (int i = 0; i < OpenFiles::MAX_FILES; i++) {
		//���̴��ļ������������ҵ�������򷵻�֮
		if (!ProcessOpenFileTable[i]) {
			//���ú���ջ�ֳ��������е�EAX�Ĵ�����ֵ����ϵͳ���÷���ֵ
			g_user.ar0[User::EAX] = i;
			return i;
		}
	}
		g_user.ar0[User::EAX] = -1;
		g_user.errorCode = User::U_EMFILE;
		return -1;
}

//�����û�ϵͳ�����ṩ���ļ�����������fd���ҵ���Ӧ�Ĵ��ļ����ƿ�File�ṹ
File* OpenFiles::GetF(int fd)
{
	File* fpointer;
	//�ж�fd�Ƿ�Ϸ�
	if (fd < 0 || fd >= OpenFiles::MAX_FILES) {
		g_user.errorCode = User::U_EBADF;
		return NULL;
	}
	//�ӽ��̴��ļ�����Ѱ��ָ��ϵͳ���ļ����FILEָ��
	fpointer = ProcessOpenFileTable[fd];
	if (!fpointer) {
		g_user.errorCode = User::U_EBADF;
	}

	return fpointer;
}

//Ϊ�ѷ��䵽�Ŀ���������fd���ѷ���Ĵ��ļ����п���File������������ϵ
void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::MAX_FILES)
		return;

	ProcessOpenFileTable[fd] = pFile;
	return;
}