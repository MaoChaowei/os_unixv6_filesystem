#pragma once

#include "INode.h"
#include "Buf.h"
#include "BufferManager.h"

/* �ļ�ϵͳ�洢��Դ�����(Super Block)�Ķ��塣*/
class SuperBlock
{
public:
	const static int MAX_FREE = 100;
	const static int MAX_INODE = 100;

public:
	int		s_isize;		/* ���Inode��ռ�õ��̿���2~1023��1022�� */
	int		s_fsize;		/* �������̿����� 16384 */

	int		s_nfree;		/* ֱ�ӹ���Ŀ����̿����� */
	int		s_free[MAX_FREE];	/* ֱ�ӹ���Ŀ����̿����������100 */
	int		s_ninode;		/* ֱ�ӹ���Ŀ������Inode���� */
	int		s_inode[MAX_INODE];	/* ֱ�ӹ���Ŀ������Inode���������100 */

	int		s_fmod;			/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	int		s_time;			/* ���һ�θ���ʱ�� */
	int		padding[50];	/* ���ʹSuperBlock���С����1024�ֽڣ�ռ��2������ */

public:
	SuperBlock() 
	{
		//nothing much
	}
	~SuperBlock()
	{
		//nothing much
	}
};

/* �ļ�ϵͳ��(FileSystem)�����ļ��洢�豸��
 * �ĸ���洢��Դ�����̿顢���INode�ķ��䡢
 * �ͷš�*/
class FileSystem
{
public:
	/*���̵Ŀռ����*/
	static const int BLOCK_SIZE = 512;				//Block���С����λ�ֽ�
	static const int DISK_SECTOR_NUMBER = 16384;	//�������������� 8MB / 512B = 16384  
	static const int SUPERBLOCK_START_SECTOR = 0;	//����SuperBlockλ�ڴ����ϵ������ţ�ռ����������

	static const int INODE_ZONE_START_SECTOR = 2;	//���Inode��λ�ڴ����ϵ���ʼ������
	static const int INODE_ZONE_SIZE = 1022;		//���������Inode��ռ�ݵ������� 
	static const int INODE_SIZE = sizeof(DiskInode);//64�ֽ�
	static const int INODE_NUMBER_PER_SECTOR = BLOCK_SIZE / INODE_SIZE;//���INode���󳤶�Ϊ64�ֽڣ�ÿ�����̿���Դ��512/64 = 8�����INode

	static const int ROOT_INODE_NO = 0;									//�ļ�ϵͳ��Ŀ¼���INode���
	static const int INODE_NUMBER_ALL = INODE_ZONE_SIZE * INODE_NUMBER_PER_SECTOR;	//���INode������

	static const int DATA_ZONE_START_SECTOR = INODE_ZONE_START_SECTOR + INODE_ZONE_SIZE;	//����������ʼ������
	static const int DATA_ZONE_END_SECTOR = DISK_SECTOR_NUMBER - 1;		//�������Ľ���������
	static const int DATA_ZONE_SIZE = DISK_SECTOR_NUMBER - DATA_ZONE_START_SECTOR;	//������ռ�ݵ���������

	DiskDriver* diskDriver;
	SuperBlock* superBlock;
	BufferManager* bufManager;

	FileSystem();
	~FileSystem();

	void FormatSuperBlock();//��ʽ��SuperBlock
	void FormatDevice();    //��ʽ�������ļ�ϵͳ

	/* �ڲ���ϵͳ��ʼ��ʱ���Ὣ���̵�SuperBlock����һ���ڴ��SuperBlock�������Ա����ں��Ը�����ٶ���ʱ�����ڴ渱����
	һ���ڴ��еĸ��������仯����ͨ������s_fmod��־�����ں˽��ڴ渱��д����� */
	void LoadSuperBlock();  //ϵͳ��ʼ��ʱ����SuperBlock
	void Update();          //��SuperBlock������ڴ渱�����µ��洢�豸��SuperBlock��ȥ

	/* ����Inode�ڵ�ķ���������㷨�����ʵ�� */
	Inode* IAlloc();        //�ڴ洢�豸�Ϸ���һ���������INode��һ�����ڴ����µ��ļ�
	void IFree(int number); //�ͷű��Ϊnumber�����INode��һ������ɾ���ļ�

	Buf* Alloc();			//�ڴ洢�豸�Ϸ�����д��̿�   
	void Free(int blkno);   //�ͷŴ洢�豸�ϱ��Ϊblkno�Ĵ��̿�
};

/* DiskINode�ڵ�������ṹ 32B*/
class DirectoryEntry
{
public:
	static const int DIRSIZ = 28;

public:
	int m_ino;                   //Ŀ¼����INode��Ų��֣�����Ӧ�ļ��ڿ��豸�ϵ���������ڵ��
	char name[DIRSIZ];           //Ŀ¼����·��������

public:
	DirectoryEntry() 
	{
		//nothing much
	}
	~DirectoryEntry()
	{
		//nothing much
	}
};