#pragma once

#include "INode.h"
#include "File.h"
#include "FileSystem.h"

/* (ϵͳ��)���ļ�������(OpenFileManager)�����ں��жԴ��ļ������Ĺ���Ϊ����
 * ���ļ������ں����ݽṹ֮��Ĺ�����ϵ��
 * ������ϵָ���̴��ļ�������ָ����ļ����е�File���ļ����ƽṹ��
 * �Լ���File�ṹָ���ļ���Ӧ���ڴ�Inode��*/

class OpenFileTable
{
public:
	static const int MAX_FILES = 100;	/* ���ļ����ƿ�File�ṹ������ */

	File m_File[MAX_FILES];				/* ϵͳ���ļ���Ϊ���н��̹������̴��ļ���������
									�а���ָ����ļ����ж�ӦFile�ṹ��ָ�롣*/
public:
	OpenFileTable();
	~OpenFileTable();

	void Reset();
	File* FAlloc();					//��ϵͳ���ļ����з���һ�����е�File�ṹ
	void CloseF(File* pFile);		//�Դ��ļ����ƿ�File�ṹ�����ü���f_count��1�������ü���f_countΪ0�����ͷ�File�ṹ��
	
};

/*
 * �ڴ�Inode��(class InodeTable)�����ڴ�Inode�ķ�����ͷš�
 */
class InodeTable
{
public:
	static const int MAX_INODE = 100;	/* �ڴ�Inode������ */

public:
	Inode m_Inode[MAX_INODE];		/* �ڴ�Inode���飬ÿ�����ļ�����ռ��һ���ڴ�Inode */
	FileSystem* m_FileSystem;	/* ��ȫ�ֶ���g_FileSystem������ */

public:
	InodeTable();
	~InodeTable();
	void Reset();
	
	Inode* IGet(int inumber);			/* �������Inode��Ż�ȡ��ӦInode��
										* �����Inode�Ѿ����ڴ��У��������������ظ��ڴ�Inode��
										* ��������ڴ��У���������ڴ�󷵻ظ��ڴ�Inode*/

	void IPut(Inode* pNode);			/* ���ٸ��ڴ�Inode�����ü����������Inode�Ѿ�û��Ŀ¼��ָ������
										* ���޽������ø�Inode�����ͷŴ��ļ�ռ�õĴ��̿顣*/

	void UpdateInodeTable();			/*�����б��޸Ĺ����ڴ�Inode���µ���Ӧ���Inode��*/

	int IsLoaded(int inumber);			/*�����Ϊinumber�����inode�Ƿ����ڴ濽����������򷵻ظ��ڴ�Inode���ڴ�Inode���е����� */
	
	Inode* GetFreeInode();				/*���ڴ�Inode����Ѱ��һ�����е��ڴ�Inode*/

};

