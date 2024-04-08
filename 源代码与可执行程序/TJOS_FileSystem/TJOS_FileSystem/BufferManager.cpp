#include"BufferManager.h"

extern DiskDriver g_diskDriver;

BufferManager::BufferManager()
{
	InitBufList();
	diskDriver = &g_diskDriver;
}
BufferManager::~BufferManager()
{
	//��������ӳ�д
	Bflush();
}

/* ����һ�黺�棬���ڶ�д�豸�ϵ��ַ���blkno��*/
//˵�������ɶ����еĻ���鶼����B_DONE��־��
//		�������ã����ȡ�Ļ������B_DONE��
//		���������ã������ӳ�д����B_DELWRI�����B_DONE�����ظÿ顣
Buf* BufferManager::GetBlk( int blkno)
{
	Buf* bp;
	//1\��ǰ�����п����û����
	if (bufMap.find(blkno) != bufMap.end()) {
		bp = bufMap[blkno];
		NotAvail(bp);//�Ӷ�����ȡ�¸û����
		return bp;
	}

	//2\�޿�������������еĵ�һ���������IO��
	bp = bFreeList.b_back;
	if (bp == &bFreeList) {
		cout << "û�п��õĻ�����ˣ���Getblk��" << endl;
		return NULL;
	}
	NotAvail(bp);//ȡ�¶��еĵ�һ��������ƿ�
	bufMap.erase(bp->b_blkno);//��map�иÿ�ľɼ�¼
	
	//�ӳ�д���ж���ִ��
	if(bp->b_flags&Buf::B_DELWRI)
		diskDriver->DiskWrite(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
	bp->b_flags &= ~(Buf::B_DELWRI| Buf::B_DONE);//������б�־

	bufMap[blkno] = bp;
	bp->b_blkno = blkno;
	return bp;
}
/* �ͷŻ�����ƿ�buf */
void BufferManager::Brelse(Buf* bp)
{
	//�������ɶ���β��
	ListAppend(bp);
}

/* ��һ�����̿顣blknoΪĿ����̿��߼���š� 
���صľ��Ǻ���B_DONE��־�Ļ���顣*/
Buf* BufferManager::Bread( int blkno)
{
	Buf* bp = GetBlk(blkno);
	if (bp->b_flags & Buf::B_DONE)
		return bp;
	else {
		diskDriver->DiskRead(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
		bp->b_flags |= Buf::B_DONE;
		return bp;
	}
}

/* дһ�����̿� */
void BufferManager::Bwrite(Buf* bp)
{
	bp->b_flags &= ~(Buf::B_DELWRI);//���ӳ�д��־
	diskDriver->DiskWrite(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
	bp->b_flags |= Buf::B_DONE;
	Brelse(bp);
}

/* �ӳ�д���̿� */
void BufferManager::Bdwrite(Buf* bp)
{
	//���ӳ�д��־��������ɶ���
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	Brelse(bp);
}
/* ��ջ��������� */
void BufferManager::ClrBuf(Buf* bp) 
{
	memset(bp->b_addr,0,BUFFER_SIZE);//���ֽ�0����BUF
}

/* �����ɶ������ӳ�д�Ļ���ȫ����������� */
void BufferManager::Bflush()
{
	Buf* bp = mBuf;
	for (int i = 0; i < NBUF; ++i,++bp) {
		if (bp->b_flags & Buf::B_DELWRI) {
			//���ӳ�д��־
			bp->b_flags &= ~(Buf::B_DELWRI);
			//д��disk
			diskDriver->DiskWrite(bp->b_addr,BUFFER_SIZE,bp->b_blkno*BUFFER_SIZE);
			//���IODONE��־
			bp->b_flags |= Buf::B_DONE;
		}
	}
}

/* ��ʽ������Buffer */
void BufferManager::FormatBuffer()
{
	for (int i = 0; i < NBUF; ++i)
		mBuf[i].ClearBuf();
	InitBufList();
}

/* ������г�ʼ�� */
void BufferManager::InitBufList()
{
	for (int i = 0; i < NBUF; ++i) {
		//ǰ��ָ��set
		if (i!=0)
			mBuf[i].b_forw = mBuf + i - 1;
		else {
			mBuf[0].b_forw = &bFreeList;//��һ��bufָ���ͷ
			bFreeList.b_back = mBuf + 0;
		}
		//����ָ��set
		if (i + 1 < NBUF)
			mBuf[i].b_back = mBuf + i + 1;
		else {
			mBuf[i].b_back = &bFreeList;
			bFreeList.b_forw = mBuf + i;
		}

		//addr no
		mBuf[i].b_addr = Buffer[i];
		mBuf[i].no = i;
	}
}
/* �ӻ��������ժ��ָ���Ļ�����ƿ�buf */
void BufferManager::NotAvail(Buf* bp)
{
	if (!bp->b_back)//impossible
		return;
	bp->b_forw->b_back = bp->b_back;
	bp->b_back->b_forw = bp->b_forw;
	bp->b_back = bp->b_forw = NULL;
}
/* �ڻ������β������buf */
void BufferManager::ListAppend(Buf* bp)
{
	if (bp->b_back!=NULL)//impossible
		return;

	bp->b_back = &bFreeList;
	bp->b_forw = bFreeList.b_forw;
	bFreeList.b_forw->b_back = bp;
	bFreeList.b_forw = bp;
}