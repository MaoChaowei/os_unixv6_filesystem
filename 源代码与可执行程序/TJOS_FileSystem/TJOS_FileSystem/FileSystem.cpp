#include<ctime>
#include"FileSystem.h"
#include"User.h"
#include"DiskDriver.h"
#include"OpenFileManager.h"
#include"BufferManager.h"

extern DiskDriver g_diskDriver;
extern BufferManager g_bufferManager; //该对象负责管理缓存的分配和回收等
extern InodeTable g_inodeTable;//该对象管理内存的所有inode
extern User g_user;//全局用户对象，为用户提供文件系统封装好的接口
extern SuperBlock g_superBlock;

/***************************FileSystem********************************/
FileSystem::FileSystem()
{
	this->diskDriver = &g_diskDriver;
	this->superBlock = &g_superBlock;
	this->bufManager = &g_bufferManager;

	if (!diskDriver->isMounted())
		this->FormatDevice();
	else
		this->LoadSuperBlock();

}
FileSystem::~FileSystem()
{
	this->Update();
	this->diskDriver = NULL;
	this->superBlock = NULL;
}

//格式化SuperBlock
void FileSystem::FormatSuperBlock()
{
	this->superBlock->s_isize = FileSystem::INODE_ZONE_SIZE;
	this->superBlock->s_fsize = FileSystem::DISK_SECTOR_NUMBER;
	this->superBlock->s_nfree = 0;
	this->superBlock->s_free[0] = -1;
	this->superBlock->s_ninode = 0;
	this->superBlock->s_fmod = 0;
	time((time_t*)&superBlock->s_time);
}

//格式化整个文件系统
void FileSystem::FormatDevice()
{
	//格式化SpuerBlock数据结构
	this->FormatSuperBlock();
	//格式化磁盘
	this->diskDriver->CreateDiskFile();
	//写入superblock占据空间
	this->diskDriver->DiskWrite((unsigned char*)(superBlock),sizeof(SuperBlock),0);

	//重置根目录（第一个dinode，写在superblock后）
	DiskInode rootDinode;//根目录dinode
	rootDinode.d_mode |= Inode::IALLOC | Inode::IFDIR;
	rootDinode.d_nlink = 1;
	this->diskDriver->DiskWrite((unsigned char*)&rootDinode,sizeof(rootDinode));

	//接下来初始化其余diskinode(从1开始，0固定用于根目录“/”)
	DiskInode emptyDinode;//空dinode
	for (int i = 1; i < FileSystem::INODE_NUMBER_ALL; ++i) {
		if (superBlock->s_ninode < SuperBlock::MAX_INODE) {//顺序填充s_inode
			superBlock->s_inode[superBlock->s_ninode++] = i;
		}
		this->diskDriver->DiskWrite((unsigned char*)&emptyDinode, sizeof(emptyDinode));
	}

	//空闲盘块开始初始化
	char freeblk[FileSystem::BLOCK_SIZE], freeblk1[FileSystem::BLOCK_SIZE];
	memset(freeblk,0, FileSystem::BLOCK_SIZE);
	memset(freeblk1, 0, FileSystem::BLOCK_SIZE);

	//....mark存疑
	for (int i = 0; i < FileSystem::DATA_ZONE_SIZE; ++i) {
		if (superBlock->s_nfree >= SuperBlock::MAX_FREE) {
			memcpy(freeblk1, &superBlock->s_nfree, sizeof(int) + sizeof(superBlock->s_free));
			diskDriver->DiskWrite((unsigned char*)&freeblk1, FileSystem::BLOCK_SIZE);
			superBlock->s_nfree = 0;
		}
		else
			diskDriver->DiskWrite((unsigned char*)&freeblk, FileSystem::BLOCK_SIZE);
		superBlock->s_free[superBlock->s_nfree++] = i + FileSystem::DATA_ZONE_START_SECTOR;
	}

	time((time_t*)&superBlock->s_time);
	//再次写入superblock
	diskDriver->DiskWrite((unsigned char*)(superBlock), sizeof(SuperBlock), 0);

}

//系统初始化时读入SuperBlock
void FileSystem::LoadSuperBlock()
{
	fseek(this->diskDriver->diskFilePointer,0,0);
	this->diskDriver->DiskRead((unsigned char*)(superBlock), sizeof(SuperBlock), 0);

}
//将SuperBlock对象的内存副本更新到存储设备的SuperBlock中去
void FileSystem::Update()
{
	Buf* pbuf;
	//修改标志位，fmod置为0表示已同步
	superBlock->s_fmod = 0;
	superBlock->s_time = (int)time(NULL);
	//覆盖磁盘的超级块
	for (int i = 0; i < 2; i++) {
		int* p = (int*)superBlock + i * 128;/* 128=512/4 */
		pbuf = this->bufManager->GetBlk(FileSystem::SUPERBLOCK_START_SECTOR+i);
		memcpy(pbuf->b_addr,p,FileSystem::BLOCK_SIZE);//内存中超级块写入缓存块
		this->bufManager->Bwrite(pbuf);//缓存块写回磁盘
	}
	//DiskInode、数据块的更新
	g_inodeTable.UpdateInodeTable();/*将所有被修改过的内存Inode更新到对应外存Inode中*/
	this->bufManager->Bflush();//将缓存块的自由队列中延迟写的缓存全部输出到磁盘
}

/* 磁盘Inode节点的分配与回收算法设计与实现，在存储设备上分配一个空闲外存INode，一般用于创建新的文件 */
Inode* FileSystem::IAlloc()
{
	Buf* pbuf;
	Inode* pinode;
	int ino;	/* 分配到的空闲外存Inode编号 */

	//如果超级块索引结点表为空
	if (this->superBlock->s_ninode <= 0)
	{
		ino = -1;
		//遍历inode区，查找空闲diskinode
		for (int i = 0; i < this->superBlock->s_isize; ++i) 
		{
			//读取含8个diskinode的磁盘块到缓存块
			pbuf = this->bufManager->Bread(FileSystem::INODE_ZONE_START_SECTOR+i);
			int* p = (int*)pbuf->b_addr;
			//遍历各diskinode
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR;++j) {
				++ino;
				int mode = *(p+j*FileSystem::INODE_SIZE/sizeof(int));//定位到d_mode，初始化为0
				if (mode)
					continue;//占有则继续
				//如果外存inode的i_mode == 0，此时并不能确定该inode是空闲的，
				//因为有可能是内存inode没有写到磁盘上, 所以要继续搜索内存inode中是否有相应的项
				if (g_inodeTable.IsLoaded(ino) == -1) {
					//inodetable中没有对应内存inode，所以可以加入到超级块的空闲inode表中
					superBlock->s_inode[superBlock->s_ninode++] = ino;
					//满了就不能加入了,结束遍历
					if (superBlock->s_ninode >= SuperBlock::MAX_INODE)
						break;
				}
			}
			this->bufManager->Brelse(pbuf);
			//满了就不能加入了,结束遍历
			if (superBlock->s_ninode >= SuperBlock::MAX_INODE)
				break;
		}//至此重新填充了s_inode

		if (superBlock->s_ninode <= 0) 
		{
			//s_inode仍然为空，说明inode区已经全部占满
			g_user.errorCode = User::U_ENOSPC;
			return NULL;
		}
	}
	//从索引表“栈顶”获取空闲diskinode编号
	ino = superBlock->s_inode[--superBlock->s_ninode];
	//将该diskinode读入内存
	pinode = g_inodeTable.IGet(ino);
	if (pinode == NULL) {
		cout << "ERR:diskinode读入内存失败！（IAlloc）" << endl;
		return NULL;
	}

	pinode->Clean();
	superBlock->s_fmod = 1;
	return pinode;

}
//释放编号为number的外存INode，一般用于删除文件，只需考虑要不要加入superblock
void FileSystem::IFree(int number)
{
	/* 如果超级块直接管理的空闲外存Inode超过100个，
	 * 同样让释放的外存Inode散落在磁盘Inode区中。 */
	if (superBlock->s_ninode >= SuperBlock::MAX_INODE) {
		return;
	}
	superBlock->s_inode[superBlock->s_ninode++] = number;
	superBlock->s_fmod = 1;/* 设置SuperBlock被修改标志 */

}

//在存储设备上分配空闲磁盘块，从s_free中取出（分组链接方式 
Buf* FileSystem::Alloc()
{
	int blkno;//分配到的空闲磁盘块编号
	Buf* pbuf;//存储该空闲磁盘内容的缓存块

	//从索引表“栈顶”获取空闲磁盘块编号
	blkno = superBlock->s_free[--superBlock->s_nfree];
	//若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块
	if (blkno <= 0) {
		superBlock->s_nfree = 0;
		g_user.errorCode = User::U_ENOSPC;
		return NULL;
	}
	//如果该组栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号
	//需要将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中
	if (superBlock->s_nfree <= 0) {
		pbuf = this->bufManager->Bread(blkno);						/* 读入该空闲磁盘块 */
		int* p = (int*)pbuf->b_addr;								/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		superBlock->s_nfree = *p++;									/* 首先读出空闲盘块数s_nfree */
		memcpy(superBlock->s_free, p, sizeof(superBlock->s_free));	/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		this->bufManager->Brelse(pbuf);								/* 缓存使用完毕，释放以便被其它进程使用 */
	}

	//成功得到一块磁盘
	pbuf = this->bufManager->GetBlk(blkno);//申请缓存
	if (pbuf)
		this->bufManager->ClrBuf(pbuf);//清空缓存
	superBlock->s_fmod = 1;//设置SuperBlock被修改标志

	return pbuf;
}

 //释放存储设备上编号为blkno的磁盘块，加入s_free（分组链接方式 
void FileSystem::Free(int blkno)
{
	Buf* pbuf;
	//如果当前分组已经满了，要当前分组更新到blkno磁盘中
	if (superBlock->s_nfree >= SuperBlock::MAX_FREE) {
		pbuf = this->bufManager->GetBlk(blkno);//为该磁盘申请缓存
		//将superblock中的内容写入blkno的缓存
		int* p = (int*)pbuf->b_addr;
		*p++ = superBlock->s_nfree;
		memcpy(p, superBlock->s_free, sizeof(int) * SuperBlock::MAX_FREE);
		//更新superblk
		superBlock->s_nfree = 0;
		//将该缓存写回磁盘
		this->bufManager->Bwrite(pbuf);
	}
	//更新superblk
	superBlock->s_free[superBlock->s_nfree++] = blkno;
	superBlock->s_fmod = 1;
}