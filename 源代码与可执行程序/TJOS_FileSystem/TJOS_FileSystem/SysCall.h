#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include"INode.h"
#include"OpenFileManager.h"
using namespace std;

/* ���ļ�������(FileManager)��ϵͳ���ý��з�װ
 * ��װ���ļ�ϵͳ�ĸ���ϵͳ�����ں���̬�´�����̣�
 * ����ļ���Open()��Close()��Read()��Write()�ȵ�
 * ��װ�˶��ļ�ϵͳ���ʵľ���ϸ�ڡ� */
class SysCall
{
public:
	enum DirSearchMode	//Ŀ¼����ģʽ,NameI()�к����õ�
	{
		OPEN = 0,		//�Դ��ļ���ʽ����
		CREATE = 1,		//���½��ļ���ʽ����
		DELETE = 2		//��ɾ���ļ���ʽ����
	};
	//���ݳ�Ա

	Inode* rootDirInode;	//��Ŀ¼�ڴ�INODE���ָ��
	FileSystem* fileSystem;	//����ȫ��g_fileSystem
	InodeTable* inodeTable;	//����ȫ��g_inodeTable
	OpenFileTable* openFileTable;//����ȫ��g_openFileTable

	//��Ա����

	SysCall();
	~SysCall();

	void Open();                          //Open()ϵͳ���ô������
	void Creat();                         //Creat()ϵͳ���ô������
	void Open1(Inode* pINode, int trf);   //Open()��Creat()ϵͳ���õĹ�������
	void Close();                         //Close()ϵͳ���ô������                  
	void Seek();                          //Seek()ϵͳ���ô������
	void Read();                          //Read()ϵͳ���ô������
	void Write();                         //Write()ϵͳ���ô������
	void Rdwr(enum File::FileFlags mode); //��дϵͳ���ù������ִ���
	Inode* NameI(enum DirSearchMode mode);//Ŀ¼��������·��ת��Ϊ��Ӧ��INode�����������INode
	Inode* MakNode(int mode);             //��Creat()ϵͳ����ʹ�ã�����Ϊ�������ļ������ں���Դ
	void UnLink();                        //ȡ���ļ�
	void WriteDir(Inode* pINode);         //��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼��
	void ChDir();                         //�ı䵱ǰ����Ŀ¼
	void Ls();                            //�г���ǰINode�ڵ���ļ���
	void Rename(string ori, string cur);  //�������ļ����ļ���
};