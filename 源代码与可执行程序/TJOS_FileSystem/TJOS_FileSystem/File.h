#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include"INode.h"
/* ���ļ��������ļ����ƿ�File�ࡢ���̴��ļ���������OpenFiles���Լ�IO����������� */

/* ���ļ����ƿ�File�ࡣ
 * �ýṹ��¼�˽��̴��ļ��Ķ���д�������ͣ��ļ���дλ�õȵȡ� */
class File
{
public:
	enum FileFlags
	{
		FREAD = 0x1,			/* ���������� */
		FWRITE = 0x2,			/* д�������� */
	};

	unsigned int f_flag;		/* �Դ��ļ��Ķ���д����Ҫ�� */
	int		f_count;			/* ��ǰ���ø��ļ����ƿ�Ľ���������=0��ʾ��File������У��ɷ��� */
	Inode* f_inode;				/* ָ����ļ����ڴ�Inodeָ�� */
	int		f_offset;			/* �ļ���дλ��ָ�� */
	
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


/* ���̴��ļ���������(OpenFiles)�Ķ���
 * ���̵�u�ṹ�а���OpenFiles���һ������
 * ά���˵�ǰ���̵����д��ļ���
 * ps:��Ȼ��ʵ��ֻ���ǵ����̣������ǲο�v6++�ķ�ʽ��
 * �����˽��̵Ĵ��ļ��� */

class OpenFiles
{
public:
	static const int MAX_FILES = 100;			/* ��������򿪵�����ļ��� */

private:
	File* ProcessOpenFileTable[MAX_FILES];		/* File�����ָ�����飬ָ��ϵͳ���ļ����е�File���� */

public:
	OpenFiles();
	~OpenFiles();
	void Reset();

	//����������ļ�ʱ���ڴ��ļ����������з���һ�����б���
	int AllocFreeSlot();
	//�����û�ϵͳ�����ṩ���ļ�����������fd���ҵ���Ӧ�Ĵ��ļ����ƿ�File�ṹ
	File* GetF(int fd);
	//Ϊ�ѷ��䵽�Ŀ���������fd���ѷ���Ĵ��ļ����п���File������������ϵ
	void SetF(int fd, File* pFile);

};

/* �ļ�I/O�Ĳ�����
 * ���ļ�����дʱ���õ��Ķ���дƫ������
 * �ֽ����Լ�Ŀ�������׵�ַ������ */
class IOParameter
{
	/* Members */
public:
	unsigned char* m_Base;	/* ��ǰ����д�û�Ŀ��������׵�ַ */
	int m_Offset;	/* ��ǰ����д�ļ����ֽ�ƫ���� */
	int m_Count;	/* ��ǰ��ʣ��Ķ���д�ֽ����� */

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
