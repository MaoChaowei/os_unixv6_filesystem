#include"OpenFileManager.h"
#include"User.h"

extern User g_user;
extern BufferManager g_bufferManager; //该对象负责管理缓存的分配和回收等
extern InodeTable g_inodeTable;//该对象管理内存的所有inode
extern OpenFileTable g_openFileTable;//该对象负责打开文件表项的管理
extern FileSystem g_fileSystem;//该对象负责管理文件系统存储资源

/*******************************OPENFILETABLE*************************************/

OpenFileTable::OpenFileTable()
{
	//nothing to do here
}
OpenFileTable::~OpenFileTable()
{
	//nothing to do here
}

//用于重启刷新
void OpenFileTable::Reset()
{
	for (int i = 0; i < OpenFileTable::MAX_FILES; i++) {
		m_File[i].Reset();
	}
}
//在系统打开文件表中分配一个空闲的File结构  
//作用：进程打开文件描述符表中找的空闲项  之 下标  写入 u_ar0[EAX]
File* OpenFileTable::FAlloc()
{
	//在进程打开文件描述符表中获取一个空闲项 
	int fd = g_user.ofiles.AllocFreeSlot();

	if (fd < 0) //查找失败
	{
		return NULL;
	}
	//在打开文件表中找空闲项目，并完成与进程打开文件表的勾连
	for (int i = 0; i < OpenFileTable::MAX_FILES; i++)
	{
		if (m_File[i].f_count == 0) {
			//建立进程打开文件表中描述符项与File结构的勾连
			g_user.ofiles.SetF(fd,&m_File[i]);
			//维护FILE结构
			m_File[i].f_count++;
			m_File[i].f_offset = 0;

			return &m_File[i];
		}
	}

	cout << "系统打开文件表中没有空闲的表项，分配失败！" << endl;
	g_user.errorCode = User::U_ENFILE;
	return NULL;

}
//对打开文件控制块File结构的引用计数f_count减1，若引用计数f_count为0，则释放File结构。
void OpenFileTable::CloseF(File* pFile)
{
	pFile->f_count--;
	if (pFile->f_count <= 0)
		g_inodeTable.IPut(pFile->f_inode);
}

/*******************************INODETABLE*************************************/

InodeTable::InodeTable()
{
	m_FileSystem = &g_fileSystem;
}
InodeTable::~InodeTable()
{
	//nothing much
}

void InodeTable::Reset()
{
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		m_Inode[i].Reset();
	}
}

// 根据外存INode编号获取对应INode。
// 如果该INode已经在内存中，增加其引用数,返回该内存INode；
// 如果不在内存中，则将其读入内存并返回该内存INode
Inode* InodeTable::IGet( int inumber)
{
	Inode* ip;
	int idx = IsLoaded(inumber);

	/* 已经在内存中 */
	if (idx >= 0){	
		ip = &m_Inode[idx];
		++ip->i_count;//添加引用数
		return ip;
	}

	/* 不在内存中 */
	//获取一个空闲inode
	ip = GetFreeInode();

	// 若内存Inode表已满，分配空闲Inode失败 
	if (!ip) {
		cout << "ERR:获取空闲inode失败，inodetable已满（IGet）" << endl;
		g_user.errorCode = User::U_ENFILE;
		return NULL;
	}

	// 分配inode成功，将外存Inode读入新分配的内存Inode
	ip->i_number = inumber;
	++ip->i_count;

	// 将该外存Inode读入缓冲区 
	int actual_blkno = FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR;//逻辑inumber到物理blkno的转换
	Buf* pbuf = g_bufferManager.Bread(actual_blkno);

	// 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中 
	ip->ICopy(pbuf,inumber);
	// 释放缓存
	g_bufferManager.Brelse(pbuf);
	return ip;
}

/* close文件时会调用Iput
 *      主要做的操作：内存i节点计数 i_count--；若为0，释放内存 i节点、若有改动写回磁盘
 * 搜索文件途径的所有目录文件，搜索经过后都会Iput其内存i节点。路径名的倒数第2个路径分量一定是个
 *   目录文件，如果是在其中创建新文件、或是删除一个已有文件；再如果是在其中创建删除子目录。那么
 *   	必须将这个目录文件所对应的内存 i节点写回磁盘。
 *   	这个目录文件无论是否经历过更改，我们必须将它的i节点写回磁盘。*/
void InodeTable::IPut(Inode* pNode)
{
	//当前进程为引用该内存INode的唯一进程，且准备释放该内存INode
	if (pNode->i_count == 1) {
		//若该文件已经没有目录路径指向它
		if (pNode->i_nlink <= 0) {
			//释放该文件占据的数据盘块
			pNode->ITrunc();
			pNode->i_mode = 0;
			//释放对应的外存INode
			m_FileSystem->IFree(pNode->i_number);
		}
		//更新外存INode信息
		pNode->IUpdate((int)time(NULL));
		//清除内存INode的所有标志位
		pNode->i_flag = 0;
		//这是内存inode空闲的标志之一，另一个是i_count == 0
		pNode->i_number = -1;
	}
	pNode->i_count--;
}

/*将所有被修改过的内存Inode更新到对应外存Inode中*/
void InodeTable::UpdateInodeTable()
{
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		if (m_Inode[i].i_count) {
			m_Inode[i].IUpdate((int)time(NULL));
		}
	}
}

/*检查编号为inumber的外存inode是否有内存拷贝，如果有则返回该内存Inode在内存Inode表中的索引 */
int InodeTable::IsLoaded(int inumber)
{
	// 内存inode表中寻找，且该inode要有内存打开结构(有指向其的FILE结构)
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		if (m_Inode[i].i_number == inumber && m_Inode[i].i_count != 0)
			return i;
	}
	return -1;
}

/*在内存Inode表中寻找一个空闲的内存Inode*/
Inode* InodeTable::GetFreeInode()
{
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		if (m_Inode[i].i_count == 0)
			return &m_Inode[i];
	}
	return NULL;
}
