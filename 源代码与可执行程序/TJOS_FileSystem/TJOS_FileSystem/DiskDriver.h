#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
using namespace std;

/* DiskDriver�ฺ�������̣����ļ�ϵͳֻ����һ���豸���̣��ʽ�UnxiV6++�д��̹���
����Ҫ�Ĳ�������ɸ��� */


class DiskDriver {
public:				
	
	FILE* diskFilePointer;	//ָ����̵��ļ�ָ��

public:
	DiskDriver();

	~DiskDriver();

	//���������ļ�����֮ǰ���У���Ḳ��֮��
	void CreateDiskFile();

	//���´򿪴����ļ�
	//void getClean();

	//�����̾����ļ��Ƿ����
	bool isMounted();
	

	//ʵ��д�����ļ�
	void DiskWrite(const void* ptr, size_t size, int offset = -1, size_t whence = SEEK_SET);

	//ʵ�ֶ������ļ�
	void DiskRead(void* ptr, size_t size, int offset = -1, size_t whence = SEEK_SET);

};