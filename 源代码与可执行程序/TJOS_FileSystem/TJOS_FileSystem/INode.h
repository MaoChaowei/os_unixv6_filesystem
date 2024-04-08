#pragma once

#include "Buf.h"

/*
 * �ڴ������ڵ�(INode)�Ķ���
 * ϵͳ��ÿһ���򿪵��ļ�����ǰ����Ŀ¼����ӦΨһ���ڴ�inode��
 */
class Inode
{
public:
	/* i_flag�б�־λ */
	enum INodeFlag
	{
		IUPD = 0x2,		// �ڴ�inode���޸Ĺ�����Ҫ������Ӧ���inode 
		IACC = 0x4,		// �ڴ�inode�����ʹ�����Ҫ�޸����һ�η���ʱ�� 
	};

	/* static const member */
	//���static��Ա���������������ж����Ĵ洢�������������࣬�����ڲ��ľ�̬�ֲ����ټ���const=�����ڲ��ľ�̬��������
	static const unsigned int IALLOC = 0x8000;		// �ļ���ʹ�� 
	static const unsigned int IFMT = 0x6000;		// �ļ��������� 
	static const unsigned int IFDIR = 0x4000;		// �ļ����ͣ�Ŀ¼�ļ� 
	static const unsigned int ILARG = 0x1000;		// �ļ��������ͣ����ͻ�����ļ� 
	static const unsigned int IREAD = 0x100;		// ���ļ��Ķ�Ȩ�� 
	static const unsigned int IWRITE = 0x80;		// ���ļ���дȨ�� 

	static const int BLOCK_SIZE = 512;										// �ļ��߼����С: 512�ֽ� 
	static const int ADDRESS_PER_INDEX_BLOCK = BLOCK_SIZE / sizeof(int);	// ÿ�����������(��������)�����������̿�� 
	static const int SMALL_FILE_BLOCK = 6;									// С���ļ���ֱ������������Ѱַ���߼���� 
	static const int LARGE_FILE_BLOCK = 128 * 2 + 6;						// �����ļ�����һ�μ������������Ѱַ���߼���� 
	static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;			// �����ļ��������μ����������Ѱַ�ļ��߼���� 
	
	/* Member */
	unsigned int i_flag;	// ״̬�ı�־λ�������enum INodeFlag 
	unsigned int i_mode;	// �ļ�������ʽ��Ϣ 
	int		i_count;		// ���ü��� ps������0��ʾ����
	int		i_nlink;		// �ļ���������������ļ���Ŀ¼���в�ͬ·���������� 
	int		i_number;		// ���inode���еı��  ps:��0��ʼ����,��=-1��ʾ���� 
	int		i_size;			// �ļ���С���ֽ�Ϊ��λ 
	int		i_addr[10];		// �����ļ��߼���ź�������ת���Ļ��������� 

public:
	Inode();
	~Inode();
	void Reset();

	//����Inode�����е�������̿���������ȡ��Ӧ���ļ�����
	void ReadI();
	//����Inode�����е�������̿�������������д���ļ�
	void WriteI();
	//���ļ����߼����ת���ɶ�Ӧ�������̿��
	int Bmap(int lbn);

	//�������Inode�����ķ���ʱ�䡢�޸�ʱ��
	void IUpdate(int time);
	//�ͷ�Inode��Ӧ�ļ�ռ�õĴ��̿�
	void ITrunc();
	// ���Inode�����е����ݣ����ǲ���������inode���ӵı�־��i_flag\i_count\i_number
	void Clean();
	//���������Inode�ַ�������Ϣ�������ڴ�Inode��
	void ICopy(Buf* bp, int inumber);

};


/*
 * ��������ڵ�(DiskINode)�Ķ��壬�Ǵ����е�INode
 * ���Inodeλ���ļ��洢�豸�ϵ�
 * ���Inode���С�ÿ���ļ���Ψһ��Ӧ
 * �����Inode���������Ǽ�¼�˸��ļ�
 * ��Ӧ�Ŀ�����Ϣ��
 * ���Inode������ֶκ��ڴ�Inode���ֶ�
 * ���Ӧ�����INode���󳤶�Ϊ64�ֽڣ�
 * ÿ�����̿���Դ��512/64 = 8�����Inode
 * ע�⣺˳��Ҫ�ģ���Ϊ������ָ���ȡ�����ǰ�����ȡ
 */
class DiskInode
{
public:
	unsigned int d_mode;	/* ״̬�ı�־λ�������enum InodeFlag */
	int		d_nlink;		/* �ļ���������������ļ���Ŀ¼���в�ͬ·���������� */
	int		d_size;			/* �ļ���С���ֽ�Ϊ��λ */
	int		d_addr[10];		/* �����ļ��߼���ú�������ת���Ļ��������� */
	int		d_atime;		/* ������ʱ�� */
	int		d_mtime;		/* ����޸�ʱ�� */
	int		padding;		/* diskinodeΪ64�ֽ� */
public:
	DiskInode();
	~DiskInode();
};
