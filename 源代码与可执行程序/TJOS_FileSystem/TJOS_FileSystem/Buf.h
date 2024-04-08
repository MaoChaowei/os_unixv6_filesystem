#pragma once
#include<iostream>
using namespace std;
/*
 * ������ƿ�buf����
 * ��¼����Ӧ�����ʹ���������Ϣ��
 * ͬʱ����I/O����飬��¼�û���
 * ��ص�I/O�����ִ�н����
 * 
 ���ļ�ϵͳֻʵ���˵��û��ĵ����̣��ʻ������ֻ������һ��������У�Ҳ�����ж��Ƿ���Ҫ������
 */

class Buf
{
public:
	/* b_flags�б�־λ 
	ֻ�õ���������־��B_DONE��B_DELWRI���ֱ��ʾ�Ѿ����IO���ӳ�д�ı�־��
	*/
	enum BufFlag	
	{
		B_DONE = 0x4,			/* I/O�������� */
		B_DELWRI = 0x80			/* �ӳ�д������Ӧ����Ҫ��������ʱ���ٽ�������д����Ӧ���豸�� */
	};

	unsigned int b_flags;	/* ������ƿ��־λ */

	int		padding;		/* 4�ֽ���䣬ʹ��b_forw��b_back��Buf������Devtab��
							 * �е��ֶ�˳���ܹ�һ�£�����ǿ��ת������� */
	/* ������ƿ���й���ָ�� */
	Buf* b_forw;
	Buf* b_back;
		
	int		b_wcount;		/* �贫�͵��ֽ��� */
	unsigned char* b_addr;	/* ָ��û�����ƿ�������Ļ��������׵�ַ */
	int		b_blkno;		/* ���̵��߼���� 0��ʼ */
	int		no;				/* ����һ��number��¼buf����ı�� */


	Buf()
	{
		b_flags = 0;
		b_forw = NULL;
		b_back = NULL;
		b_wcount = 0;
		b_addr = NULL;
		b_blkno = -1;
		no = 0;
	}

	//��ջ����
	void ClearBuf() {
		b_flags = 0;
		b_forw = NULL;
		b_back = NULL;
		b_wcount = 0;
		b_addr = NULL;
		b_blkno = -1;
		no = 0;
	}

};