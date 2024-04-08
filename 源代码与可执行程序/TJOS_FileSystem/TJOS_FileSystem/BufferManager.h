#pragma once
#include<iostream>
#include"Buf.h"
#include"DiskDriver.h"
#include<map>

using namespace std;

/* BufManager���ϲ��ļ�ϵͳ�ṩͳһ�ķ���catch�Ľӿڣ���ӵ���diskdriverʵ�� */
class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 100;		// ������ƿ顢������������ 
	static const int BUFFER_SIZE = 512;	// ��������С�� ���ֽ�Ϊ��λ

private:
	Buf bFreeList;						// �������ɶ��� 
	Buf mBuf[NBUF];						// ������ƿ����� 
	unsigned char Buffer[NBUF][BUFFER_SIZE];// ���������� 
	DiskDriver* diskDriver;				// ָ����̹�������ȫ�ֶ���
	map<int, Buf*> bufMap;				// �Ǽ�buf���������blkno��map�����ڿ��ٲ����Ƿ������ 

public:
	BufferManager();
	~BufferManager();
	Buf* GetBlk(int blkno);				// ����һ�黺�棬���ڶ�д�豸�ϵ��ַ���blkno��
	void Brelse(Buf* bp);				// �ͷŻ�����ƿ�buf 
	Buf* Bread(int blkno);				// ��һ�����̿顣blknoΪĿ����̿��߼���š� 
	void Bwrite(Buf* bp);				// дһ�����̿� 
	void Bdwrite(Buf* bp);				// �ӳ�д���̿� 
	void ClrBuf(Buf* bp);				// ��ջ��������� 
	void Bflush();						// ���������ӳ�д�Ļ���ȫ����������� 
	void FormatBuffer();				// ��ʽ������Buffer 

private:
	void InitBufList();					// ������кͻ�����ʼ�� 
	void NotAvail(Buf* bp);				// �ӻ��������ժ��ָ���Ļ�����ƿ�buf 
	void ListAppend(Buf* bp);			// �ڻ������β������buf 
};