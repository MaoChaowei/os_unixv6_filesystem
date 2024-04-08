#pragma once
#include"SysCall.h"
#include<string>
using namespace std;

class User {
public:
	static const int EAX = 0;		//u.ar0[EAX]�������ֳ���������EAX�Ĵ�����ƫ����

	// ������
	enum ErrorCode {
		U_NOERROR = 0,				//û�д���
		U_ENOENT = 1,				//�Ҳ����ļ������ļ���
		U_EBADF = 2,				//�Ҳ����ļ����
		U_EACCES = 3,				//Ȩ�޲���
		U_ENOTDIR = 4,				//�ļ��в�����
		U_ENFILE = 5,				//�ļ������
		U_EMFILE = 6,				//���ļ�����
		U_EFBIG = 7,				//�ļ�����
		U_ENOSPC = 8				//���̿ռ䲻��
	};

public:
	Inode* curDirIP;					//ָ��ǰĿ¼��Inodeָ��
	Inode* parDirIP;					//ָ��Ŀ¼��Inodeָ��
	DirectoryEntry curDirEnt;           //��ǰĿ¼��Ŀ¼��
	char dbuf[DirectoryEntry::DIRSIZ];	//��ǰ·������
	string curDirPath;					//��ǰ����Ŀ¼����·��
	string dirp;						//ϵͳ���ò���(һ������Pathname)��ָ��

/* ϵͳ������س�Ա */
	int arg[5];							//��ŵ�ǰϵͳ���ò���
	size_t ar0[5];                      /*ָ�����ջ�ֳ���������EAX�Ĵ���
									    ��ŵ�ջ��Ԫ�����ֶδ�Ÿ�ջ��Ԫ�ĵ�ַ��
									    ��V6��r0���ϵͳ���õķ���ֵ���û�����*/
	ErrorCode errorCode;				//��Ŵ�����

	OpenFiles ofiles;					//��ǰ�û��Ľ��̴��ļ������������
	IOParameter IOParam;				//��¼��ǰ����д�ļ���ƫ�������û�Ŀ�������ʣ���ֽ�������
	SysCall* sysCall;
	string ls;							//���Ŀ¼��

public:

	User();
	~User();

	void u_Ls();						//��Ŀ¼
	void u_Cd(string dirName);			//�ƶ���
	void u_MkDir(string dirName);		//����Ŀ¼
	void u_Create(string fileName);		//�����ļ�
	void u_Delete(string fileName);		//ɾ���ļ�
	void u_Open(string fileName);		//���ļ�
	void u_Close(string fd);			//�ر��ļ�
	void u_Seek(string fd, string offset, string origin);//��λ�ļ���дִ��
	void u_Write(string fd, string inFile, string size);//д�ļ�
	void u_Read(string fd, string outFile, string size);//���ļ�

	void u_Rename(string ori, string cur);  //�������ļ����ļ���
	void u_Tree(string path);               //��ӡ��״Ŀ¼

private:
	bool checkError();
	bool checkPathName(string path);
	void __userCd__(string dirName);
	void __userTree__(string path, int depth);//�ڲ���ӡ��״Ŀ¼
};