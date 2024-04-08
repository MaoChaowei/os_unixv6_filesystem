#include"BufferManager.h"

extern DiskDriver g_diskDriver;

BufferManager::BufferManager()
{
	InitBufList();
	diskDriver = &g_diskDriver;
}
BufferManager::~BufferManager()
{
	//完成所有延迟写
	Bflush();
}

/* 申请一块缓存，用于读写设备上的字符块blkno。*/
//说明：自由队列中的缓存块都含有B_DONE标志，
//		若可重用：则获取的缓存块有B_DONE。
//		若不可重用：处理延迟写并清B_DELWRI、清掉B_DONE，返回该块。
Buf* BufferManager::GetBlk( int blkno)
{
	Buf* bp;
	//1\当前队列有可重用缓存块
	if (bufMap.find(blkno) != bufMap.end()) {
		bp = bufMap[blkno];
		NotAvail(bp);//从队列中取下该缓存块
		return bp;
	}

	//2\无可重用则申请队列的第一个缓存进行IO。
	bp = bFreeList.b_back;
	if (bp == &bFreeList) {
		cout << "没有可用的缓存块了！（Getblk）" << endl;
		return NULL;
	}
	NotAvail(bp);//取下队列的第一个缓存控制块
	bufMap.erase(bp->b_blkno);//清map中该块的旧记录
	
	//延迟写的判断与执行
	if(bp->b_flags&Buf::B_DELWRI)
		diskDriver->DiskWrite(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
	bp->b_flags &= ~(Buf::B_DELWRI| Buf::B_DONE);//清掉所有标志

	bufMap[blkno] = bp;
	bp->b_blkno = blkno;
	return bp;
}
/* 释放缓存控制块buf */
void BufferManager::Brelse(Buf* bp)
{
	//插入自由队列尾部
	ListAppend(bp);
}

/* 读一个磁盘块。blkno为目标磁盘块逻辑块号。 
返回的均是含有B_DONE标志的缓存块。*/
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

/* 写一个磁盘块 */
void BufferManager::Bwrite(Buf* bp)
{
	bp->b_flags &= ~(Buf::B_DELWRI);//清延迟写标志
	diskDriver->DiskWrite(bp->b_addr, BUFFER_SIZE, bp->b_blkno * BUFFER_SIZE);
	bp->b_flags |= Buf::B_DONE;
	Brelse(bp);
}

/* 延迟写磁盘块 */
void BufferManager::Bdwrite(Buf* bp)
{
	//加延迟写标志后放入自由队列
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	Brelse(bp);
}
/* 清空缓冲区内容 */
void BufferManager::ClrBuf(Buf* bp) 
{
	memset(bp->b_addr,0,BUFFER_SIZE);//用字节0填充该BUF
}

/* 将自由队列中延迟写的缓存全部输出到磁盘 */
void BufferManager::Bflush()
{
	Buf* bp = mBuf;
	for (int i = 0; i < NBUF; ++i,++bp) {
		if (bp->b_flags & Buf::B_DELWRI) {
			//清延迟写标志
			bp->b_flags &= ~(Buf::B_DELWRI);
			//写回disk
			diskDriver->DiskWrite(bp->b_addr,BUFFER_SIZE,bp->b_blkno*BUFFER_SIZE);
			//添加IODONE标志
			bp->b_flags |= Buf::B_DONE;
		}
	}
}

/* 格式化所有Buffer */
void BufferManager::FormatBuffer()
{
	for (int i = 0; i < NBUF; ++i)
		mBuf[i].ClearBuf();
	InitBufList();
}

/* 缓存队列初始化 */
void BufferManager::InitBufList()
{
	for (int i = 0; i < NBUF; ++i) {
		//前向指针set
		if (i!=0)
			mBuf[i].b_forw = mBuf + i - 1;
		else {
			mBuf[0].b_forw = &bFreeList;//第一个buf指向队头
			bFreeList.b_back = mBuf + 0;
		}
		//后向指针set
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
/* 从缓存队列中摘下指定的缓存控制块buf */
void BufferManager::NotAvail(Buf* bp)
{
	if (!bp->b_back)//impossible
		return;
	bp->b_forw->b_back = bp->b_back;
	bp->b_back->b_forw = bp->b_forw;
	bp->b_back = bp->b_forw = NULL;
}
/* 在缓存队列尾加入新buf */
void BufferManager::ListAppend(Buf* bp)
{
	if (bp->b_back!=NULL)//impossible
		return;

	bp->b_back = &bFreeList;
	bp->b_forw = bFreeList.b_forw;
	bFreeList.b_forw->b_back = bp;
	bFreeList.b_forw = bp;
}