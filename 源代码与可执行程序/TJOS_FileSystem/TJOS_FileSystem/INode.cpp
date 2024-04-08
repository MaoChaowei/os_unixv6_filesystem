#include"INode.h"
#include"BufferManager.h"
#include"FileSystem.h"
#include"User.h"
#include<cmath>

extern User g_user;
extern BufferManager g_bufferManager;
extern FileSystem g_fileSystem;

Inode::Inode()
{
	this->i_mode = 0;
	this->i_nlink = 0;
	this->i_count = 0;
	this->i_number = -1;
	this->i_size = 0;
	memset(i_addr, 0, sizeof(i_addr));
}
Inode::~Inode()
{
	//nothing much
}
void Inode::Reset()
{
	i_mode = 0;
	i_count = 0;
	i_number = -1;
	i_size = 0;
	memset(i_addr, 0, sizeof(i_addr));
}

//根据Inode对象中的物理磁盘块索引表，读取相应的文件数据
void Inode::ReadI()
{
	int lbn;	/* 文件逻辑块号 */
	int bn;		/* lbn对应的物理盘块号 */
	int offset;	/* 当前字符块内起始传送位置 */
	int nbytes;	/* 传送至用户目标区字节数量 */
	Buf* pbuf;

	if (0 == g_user.IOParam.m_Count) {
		/* 需要读字节数为零，则返回 */
		return;
	}
	i_flag |= Inode::IACC;

	/* 一次一个字符块地读入所需全部数据，直至遇到文件尾 */
	while (User::U_NOERROR == g_user.errorCode && g_user.IOParam.m_Count!=0) {
		//1 计算lbn\bn\offset\nbytes
		lbn = bn = g_user.IOParam.m_Offset / Inode::BLOCK_SIZE;
		offset=g_user.IOParam.m_Offset% Inode::BLOCK_SIZE;
		/* 传送到用户区的字节数量，取读请求的剩余字节数与当前字符块内有效字节数较小值 */
		nbytes = min(Inode::BLOCK_SIZE-offset,g_user.IOParam.m_Count);
		int remain = i_size - g_user.IOParam.m_Offset;
		if (remain <= 0)/* 如果已读到超过文件结尾 */
			return;
		nbytes = min(nbytes,remain);/* 传送的字节数量还取决于剩余文件的长度 */
		if ((bn = Bmap(lbn)) == 0)
			return;
		
		//2 将bn盘块读到缓存块中
		pbuf = g_bufferManager.Bread(bn);

		unsigned char* start = pbuf->b_addr + offset;//缓存中数据起始读位置
		//3 将缓存中数据写入到用户目标区
		memcpy(g_user.IOParam.m_Base,start,nbytes);

		//4 更新IO参数
		g_user.IOParam.m_Base += nbytes;
		g_user.IOParam.m_Offset += nbytes;
		g_user.IOParam.m_Count -= nbytes;
		//释放缓存
		g_bufferManager.Brelse(pbuf);
	}

}
//根据Inode对象中的物理磁盘块索引表，将数据写入文件		PS:物理磁盘块表在Bmap中维护好了，直接用，最终只需修改i_size
void Inode::WriteI()
{
	int lbn;	// 文件逻辑块号 
	int bn;		// lbn对应的物理盘块号
	int offset;	// 当前字符块内起始传送位置
	int nbytes;	// 传送至用户目标区字节数量
	Buf* pbuf;

	// 设置Inode被访问标志位
	this->i_flag |= (Inode::IACC | Inode::IUPD);

	if (0 == g_user.IOParam.m_Count) {
		/* 需要读字节数为零，则返回 */
		return;
	}
	while (User::U_NOERROR == g_user.errorCode && g_user.IOParam.m_Count != 0) {
		// 1 计算lbn\bn\offset\nbytes
		lbn = bn = g_user.IOParam.m_Offset / Inode::BLOCK_SIZE;
		offset = g_user.IOParam.m_Offset % Inode::BLOCK_SIZE;
		/* 传送到用户区的字节数量，取读请求的剩余字节数与当前字符块内有效字节数较小值 */
		nbytes = min(Inode::BLOCK_SIZE - offset, g_user.IOParam.m_Count);
		if ((bn = Bmap(lbn)) == 0)
			return;

		// 如果写入数据正好满一个字符块，则为其分配缓存
		if (Inode::BLOCK_SIZE == nbytes)
			pbuf = g_bufferManager.GetBlk(bn);
		// 否则先读后写
		else
			pbuf = g_bufferManager.Bread(bn);

		unsigned char* start = pbuf->b_addr + offset;//缓存中数据起始读位置
		// 将用户区内容写入缓存块
		memcpy(start,g_user.IOParam.m_Base,nbytes);
		// 更新IO参数
		g_user.IOParam.m_Base += nbytes;
		g_user.IOParam.m_Offset += nbytes;
		g_user.IOParam.m_Count -= nbytes;

		if (g_user.errorCode != User::U_NOERROR)/* 写过程中出错，释放缓存 */
			g_bufferManager.Brelse(pbuf);
		else /* 写过程正常则：缓存块延迟写 */
			g_bufferManager.Bdwrite(pbuf);
		
		//普通文件长度增加
		if (i_size < g_user.IOParam.m_Offset)
			i_size = g_user.IOParam.m_Offset;

		i_flag |= Inode::IUPD;
	}
}

//将文件的逻辑块号转换成对应的"物理盘块号"
int Inode::Bmap(int lbn)
{
	/* Unix V6++的文件索引结构：(小型、大型和巨型文件)
	 * (1) i_addr[0] - i_addr[5]为直接索引表，文件长度范围是0 - 6个盘块；
	 * (2) i_addr[6] - i_addr[7]存放一次间接索引表所在磁盘块号，每磁盘块
	 * 上存放128个文件数据盘块号，此类文件长度范围是7 - (128 * 2 + 6)个盘块；
	 * (3) i_addr[8] - i_addr[9]存放二次间接索引表所在磁盘块号，每个二次间接
	 * 索引表记录128个一次间接索引表所在磁盘块号，此类文件长度范围是
	 * (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6) */

	Buf* pfirstBuf, * psecondBuf;
	int phyBlkno;	// 转换后的物理盘块号
	int* iTable;	// 用于访问索引盘块中一次间接、两次间接索引表
	int index;

	//超出范围，返回0
	if (lbn >= Inode::HUGE_FILE_BLOCK) {
		g_user.errorCode = User::U_EFBIG;
		return 0;
	}

	//小型文件：从基本索引表i_addr[0-5]中获得物理盘块号即可
	if (lbn < 6) {
		phyBlkno = i_addr[lbn];
		//如果该逻辑块号还没有相应的物理盘块号与之对应，则分配一个物理块
		if (0 == phyBlkno && (pfirstBuf = g_fileSystem.Alloc()) != NULL) {
			phyBlkno = pfirstBuf->b_blkno;
			/*因为后面很可能马上还要用到此处新分配的数据块，所以不急于立刻输出到磁盘上*/
			g_bufferManager.Bdwrite(pfirstBuf);
			i_addr[lbn] = phyBlkno;
			i_flag |= Inode::IUPD;
		}
		return phyBlkno;
	}

	//大型巨型文件
	/* 计算逻辑块号lbn对应i_addr[]中的索引 */
	if (lbn < Inode::LARGE_FILE_BLOCK) 
	{//大型文件
		index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
	}
	else
	{//巨型文件
		index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
	}

	phyBlkno = i_addr[index];
	if (phyBlkno)// 读出存储间接索引表的字符块 
	{
		pfirstBuf = g_bufferManager.Bread(phyBlkno);
	}
	else// 表示不存在相应的间接索引表块
	{
		i_flag |= Inode::IUPD;
		/* 分配一空闲盘块存放间接索引表 */
		if ((pfirstBuf = g_fileSystem.Alloc()) == NULL)
			return 0;//分配失败
		i_addr[index] = pfirstBuf->b_blkno;
	}

	iTable = (int*)pfirstBuf->b_addr;//将char*指针转换为int*指针，因为每个索引条目是4B，便于访问
	if (index >= 8) {
		//对于巨型文件的情况，pFirstBuf中是二次间接索引表，
		//还需根据逻辑块号，经由二次间接索引表找到一次间接索引表
		index= ((lbn - Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
		phyBlkno = iTable[index];

		if (phyBlkno) {
			// 释放二次间接索引表占用的缓存，并读入一次间接索引表 
			g_bufferManager.Brelse(pfirstBuf);
			psecondBuf = g_bufferManager.Bread(phyBlkno);
		}
		else {
			if ((psecondBuf = g_fileSystem.Alloc()) == NULL) {
				//分配磁盘块失败
				g_bufferManager.Brelse(pfirstBuf);
				return 0;
			}
			//分配成功
			/* 将新分配的一次间接索引表磁盘块号，记入二次间接索引表相应项 */
			iTable[index] = psecondBuf->b_blkno;
			/* 将更改后的二次间接索引表延迟写方式输出到磁盘 */
			g_bufferManager.Bdwrite(pfirstBuf);
		}
		pfirstBuf = psecondBuf;
		/* 令iTable指向一次间接索引表 */
		iTable = (int*)psecondBuf->b_addr;
	}

	/* 计算逻辑块号lbn最终位于一次间接索引表中的表项序号index */
	if (lbn < Inode::LARGE_FILE_BLOCK)
		index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK;
	else
		index = (lbn - Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK;

	if ((phyBlkno = iTable[index]) == 0 && (psecondBuf = g_fileSystem.Alloc()) != NULL) {
		/* 将分配到的文件数据盘块号登记在一次间接索引表中 */
		phyBlkno = psecondBuf->b_blkno;
		iTable[index] = phyBlkno;
		/* 将数据盘块、更改后的一次间接索引表用延迟写方式输出到磁盘 */
		g_bufferManager.Bdwrite(psecondBuf);
		g_bufferManager.Bdwrite(pfirstBuf);
	}
	else  /* 释放一次间接索引表占用缓存 */
		g_bufferManager.Bdwrite(pfirstBuf);

	return phyBlkno;
}

//更新外存Inode的最后的访问时间、修改时间
void Inode::IUpdate(int time)
{
	Buf* pbuf;
	DiskInode dInode;
	//注意：只有当IUPD和IACC标志之一被设置，才需要更新相应DiskInode
	//目录搜索，不会设置所途径的目录文件的IACC和IUPD标志
	if (i_flag & (Inode::IUPD | Inode::IACC)) {
		pbuf = g_bufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR+i_number/FileSystem::INODE_NUMBER_PER_SECTOR);
		dInode.d_mode = i_mode;
		dInode.d_nlink = i_nlink;
		dInode.d_size = i_size;
		memcpy(dInode.d_addr, i_addr, sizeof(dInode.d_addr));
		if (i_flag & Inode::IACC)
			dInode.d_atime = time;
		if (i_flag & Inode::IUPD)
			dInode.d_mtime = time;

		// 用dInode中的新数据覆盖缓存中的旧外存Inode 
		unsigned char* p = pbuf->b_addr + (this->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
		DiskInode* pNode = &dInode;
		memcpy(p, pNode, sizeof(DiskInode));
		// 将新内容写回外存Inode，实现更新外存Inode的目的。
		g_bufferManager.Bwrite(pbuf);
	}
}

//释放Inode对应文件占用的磁盘块
void Inode::ITrunc()
{
	Buf* pfirstBuf, * psecondBuf;

	for (int i = 9; i >= 0; --i) {
		if (i_addr[i]) {
			if (i >= 6) {
				pfirstBuf = g_bufferManager.Bread(i_addr[i]);
				int* pfirst = (int*)pfirstBuf->b_addr;
				for (int j = Inode::BLOCK_SIZE / sizeof(int) - 1; j >= 0; --j) {
					if (pfirst[j]) {
						if (i >= 8) {
							psecondBuf = g_bufferManager.Bread(pfirst[j]);
							int* psecond = (int*)psecondBuf->b_addr;
							for (int k = Inode::BLOCK_SIZE / sizeof(int) - 1; k >= 0; --k) {
								if (psecond[k])
									g_fileSystem.Free(psecond[k]);
							}
							g_bufferManager.Brelse(psecondBuf);
						}
						g_fileSystem.Free(pfirst[j]);
					}
				}
				g_bufferManager.Brelse(pfirstBuf);
			}
			g_fileSystem.Free(i_addr[i]);
			i_addr[i] = 0;
		}
	}
	i_size = 0;
	/* 增设IUPD标志位，表示此内存Inode需要同步到相应外存Inode */
	i_flag |= Inode::IUPD;
	/* 清大文件标志 */
	i_mode &= ~(Inode::ILARG);
	i_nlink = 1;
}

// 清空Inode对象中的数据
void Inode::Clean()
{
	/* Inode::Clean()特定用于IAlloc()中清空新分配DiskInode的原有数据，
	 * 即旧文件信息。Clean()函数中不应当清除i_dev, i_number, i_flag, i_count,
	 * 这是属于内存Inode而非DiskInode包含的旧文件信息，而Inode类构造函数需要
	 * 将其初始化为无效值。 */
	this->i_mode = 0;
	this->i_nlink = 0;
	this->i_size = 0;
	memset(i_addr,0,sizeof(i_addr));
}

// 将缓存块中的外存Inode信息（mode\size\nlink\addr[10]）拷贝到新分配的内存Inode中 
void Inode::ICopy(Buf* bp, int inumber)
{
	DiskInode& dInode = *(DiskInode*)(bp->b_addr + (inumber % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode));
	i_mode = dInode.d_mode;
	i_size = dInode.d_size;
	i_nlink = dInode.d_nlink;
	memcpy(i_addr,dInode.d_addr,sizeof(i_addr));
}

DiskInode::DiskInode()
{
	this->d_mode = 0;
	this->d_nlink = 0;
	this->d_size = 0;
	memset(d_addr, 0, sizeof(d_addr));
	this->d_atime = 0;
	this->d_mtime = 0;
}

DiskInode::~DiskInode()
{
	//nothing much
}