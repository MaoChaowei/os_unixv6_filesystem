#include"User.h"
#include"SysCall.h"
#include"BufferManager.h"

extern BufferManager g_bufferManager; //该对象负责管理缓存的分配和回收等
extern FileSystem g_fileSystem;//该对象负责管理文件系统存储资源
extern InodeTable g_inodeTable;//该对象管理内存的所有inode
extern OpenFileTable g_openFileTable;//该对象负责打开文件表项的管理
extern User g_user;//全局用户对象，为用户提供文件系统封装好的接口

SysCall::SysCall()
{
	this->fileSystem = &g_fileSystem;
	this->openFileTable = &g_openFileTable;
	this->inodeTable = &g_inodeTable;
	this->rootDirInode = inodeTable->IGet(FileSystem::ROOT_INODE_NO);
	rootDirInode->i_count += 0xff;//保护根目录的inode

}
SysCall::~SysCall()
{
	//nothing much
}

//Open()系统调用处理过程,功能：打开文件，建立文件的打开结构、i_count 为正数（i_count ++）
void SysCall::Open()
{
	Inode* pinode = this->NameI(SysCall::OPEN);
	if (!pinode)/* 没有找到相应的Inode */
		return;
	this->Open1(pinode,0);

}

//Creat()系统调用处理过程
/* 功能：创建一个新的文件
 * 效果：建立打开文件结构，i_count 为正数（应该是 1） */
void SysCall::Creat()
{
	Inode* pinode;
	int newACCMode = g_user.arg[1];//存放当前系统调用参数 文件类型：目录文件
	
	pinode = this->NameI(SysCall::CREATE);//搜索目录的模式为1，表示创建；若父目录不可写，出错返回

	//没有找到相应的INode，或NameI出错
	if (NULL == pinode) {
		if (g_user.errorCode!=User::U_NOERROR)
			return;

		pinode = this->MakNode(newACCMode);
		if (NULL == pinode)
			return;
		//如果创建的名字不存在，使用参数trf = 2来调用open1()
		this->Open1(pinode, 2);
		return;
	}
	//如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）
	this->Open1(pinode, 1);
	pinode->i_mode |= newACCMode;
}

//Open()、Creat()系统调用的公共部分，建立文件的打开结构！
//trf == 0由open调用
//trf == 1由creat调用，creat文件的时候搜索到同文件名的文件
//trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
//mode参数：打开文件模式，表示文件操作是 读、写还是读写
void SysCall::Open1(Inode* pINode, int trf)
{
	//create文件搜索到同名文件，释放其所占有的盘块
	if (1 == trf)
		pINode->ITrunc();
	//分配打开文件控制块File结构
	File* pFile = this->openFileTable->FAlloc();//在系统打开文件表中分配一个空闲的File结构
	if (NULL == pFile) {
		this->inodeTable->IPut(pINode);
		return;
	}
	pFile->f_inode = pINode;

	//为打开或者创建文件的各种资源都已成功分配，函数返回
	if (g_user.errorCode == 0)
		return;
	else { //如果出错则释放资源
		//释放打开文件描述符
		int fd = g_user.ar0[User::EAX];
		if (fd != -1) {
			g_user.ofiles.SetF(fd, NULL);
			//递减File结构和Inode的引用计数 ,File结构没有锁 f_count为0就是释放File结构了
			pFile->f_count--;
		}
		this->inodeTable->IPut(pINode);
	}
}
 //Close()系统调用处理过程 
void SysCall::Close()
{
	int fd = g_user.arg[0];
	// 获取打开文件控制块File结构 
	File* pfile = g_user.ofiles.GetF(fd);
	if (NULL == pfile)
		return;
	// 释放打开文件描述符fd，递减File结构引用计数
	g_user.ofiles.SetF(fd, NULL);
	this->openFileTable->CloseF(pfile);
}

//Seek()系统调用处理过程，arg0~2对应fd(文件句柄)、offset、偏移模式
void SysCall::Seek()
{
	File* pFile;
	int fd = g_user.arg[0];

	pFile = g_user.ofiles.GetF(fd);
	if (NULL == pFile)
		return; //若FILE不存在，GetF有设出错码

	int offset = g_user.arg[1];

	switch (g_user.arg[2]) {
		case 0:
			//读写位置设置为offset
			pFile->f_offset = offset;
			break;
		case 1:
			//读写位置加offset(可正可负)
			pFile->f_offset += offset;
			break;
		case 2:
			//读写位置调整为文件长度加offset
			pFile->f_offset = pFile->f_inode->i_size + offset;
			break;
		default:
			break;
	}
	cout << "* SysCall::Seek()>> 文件指针成功移动到 " << pFile->f_offset << endl;
}
//Read()系统调用处理过程
void SysCall::Read()
{
	//直接调用Rdwr()函数即可
	this->Rdwr(File::FREAD);
}
//Write()系统调用处理过程
void SysCall::Write()
{
	//直接调用Rdwr()函数即可
	this->Rdwr(File::FWRITE);
}

//读写系统调用公共部分代码
void SysCall::Rdwr(enum File::FileFlags mode)
{
	File* pFile;
	//根据Read()/Write()的系统调用参数fd获取打开文件控制块结构
	pFile = g_user.ofiles.GetF(g_user.arg[0]);
	if (NULL == pFile) //不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了
		return;
	//获取读写参数到IOParam
	g_user.IOParam.m_Base = (unsigned char*)g_user.arg[1]; //目标缓冲区首址
	g_user.IOParam.m_Count = g_user.arg[2];                //要求读/写的字节数
	g_user.IOParam.m_Offset = pFile->f_offset; //设置文件起始读位置

	//根据mode进行读或写
	if (File::FREAD == mode)
		pFile->f_inode->ReadI();
	else
		pFile->f_inode->WriteI();

	//根据读写字数，移动文件读写偏移指针
	pFile->f_offset += (g_user.arg[2] - g_user.IOParam.m_Count);
	//返回实际读写的字节数，修改存放系统调用返回值的核心栈单元
	g_user.ar0[User::EAX] = g_user.arg[2] - g_user.IOParam.m_Count;

}

//目录搜索，将路径转化为相应的INode返回INode,返回NULL表示目录搜索失败，否则是根指针，指向文件的内存打开i节点 
Inode* SysCall::NameI(enum DirSearchMode mode)
{
	Inode* pINode = g_user.curDirIP;
	Buf* pbuf;
	int freeEntryOffset; //以创建文件模式搜索目录时，记录空闲目录项的偏移量
	unsigned int index = 0, nindex = 0;

	// 如果该路径是'/'开头的，从根目录开始搜索，否则从进程当前工作目录开始搜索
	// 路径通过drip传入的
	if ('/' == g_user.dirp[0]) {
		nindex = ++index + 1;
		pINode = this->rootDirInode;
	}
	//外层循环：每次处理pathname中一段路径分量
	while (1) {
		//如果出错则释放当前搜索到的目录文件Inode，并退出
		if (g_user.errorCode != User::U_NOERROR)
			break;
		//整个路径搜索完毕，返回相应Inode指针。目录搜索成功返回
		if (nindex > g_user.dirp.length())
			return pINode;
		//如果要进行搜索的不是目录文件，释放相关Inode资源则退出
		if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {//文件类型：目录文件
			g_user.errorCode = User::U_ENOTDIR;//文件夹不存在
			break;
		}

		//将Pathname中当前准备进行匹配的路径分量拷贝到dbuf[]中，便于和目录项进行比较。
		nindex = g_user.dirp.find_first_of('/', index);
		memset(g_user.dbuf, 0, sizeof(g_user.dbuf));
		memcpy(g_user.dbuf, g_user.dirp.data() + index, (nindex == (unsigned int)string::npos ? g_user.dirp.length() : nindex) - index);
		index = nindex + 1;

		//内层循环部分：对于dbuf[]中的路径名分量，逐个搜寻匹配的目录项
		g_user.IOParam.m_Offset= 0;
		g_user.IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);//设置为目录项个数 ，含空白的目录项
		freeEntryOffset = 0;
		pbuf = NULL;
		while (1) {
			// 对目录项已经搜索完毕 
			if (0 == g_user.IOParam.m_Count) {
				if (NULL != pbuf)
					g_bufferManager.Brelse(pbuf);
				//如果是创建新文件
				if (SysCall::CREATE == mode && nindex >= g_user.dirp.length()) {
					//将父目录Inode指针保存起来，以后写目录项WriteDir()函数会用到
					g_user.parDirIP = pINode;
					if (freeEntryOffset) //此变量存放了空闲目录项位于目录文件中的偏移量
						g_user.IOParam.m_Offset = freeEntryOffset - sizeof(DirectoryEntry); //将空闲目录项偏移量存入u区中，写目录项WriteDir()会用到
					else
						pINode->i_flag |= Inode::IUPD;
					//找到可以写入的空闲目录项位置，NameI()函数返回
					return NULL;
				}
				//目录项搜索完毕而没有找到匹配项，释放相关Inode资源，并退出
				g_user.errorCode = User::U_ENOENT;
				goto out;
			}
			// 已读完目录文件的当前盘块，需要读入下一目录项数据盘块
			if (0 == g_user.IOParam.m_Offset % Inode::BLOCK_SIZE) {
				if (pbuf)
					g_bufferManager.Brelse(pbuf);
				//计算要读的物理盘块号
				int phyBlkno = pINode->Bmap(g_user.IOParam.m_Offset / Inode::BLOCK_SIZE);
				pbuf = g_bufferManager.Bread(phyBlkno);
			}
			// 没有读完当前目录项盘块，则读取下一目录项至curDirEnt
			memcpy(&g_user.curDirEnt, pbuf->b_addr+ (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(g_user.curDirEnt));
			g_user.IOParam.m_Offset += sizeof(DirectoryEntry);
			g_user.IOParam.m_Count--;
			// 如果是空闲目录项，记录该项位于目录文件中偏移量
			if (0 == g_user.curDirEnt.m_ino) {
				if (0 == freeEntryOffset)
					freeEntryOffset = g_user.IOParam.m_Offset;
				//跳过空闲目录项，继续比较下一目录项
				continue;
			}

			if (!memcmp(g_user.dbuf, &g_user.curDirEnt.name, sizeof(DirectoryEntry) - 4))
				break;
		}

		// 从内层目录项匹配循环跳至此处，说明pathname中当前路径分量匹配成功了
		// 还需匹配pathname中下一路径分量，直至遇到'\0'结束
		if (pbuf)
			g_bufferManager.Brelse(pbuf);

		// 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在g_user.curDirEnt.m_ino中
		if (SysCall::DELETE == mode && nindex >= g_user.dirp.length())
			return pINode;

		// 匹配目录项成功，则释放当前目录Inode，根据匹配成功的目录项m_ino字段获取相应下一级目录或文件的Inode
		this->inodeTable->IPut(pINode);
		pINode = this->inodeTable->IGet(g_user.curDirEnt.m_ino);
		// 回到外层While(true)循环，继续匹配Pathname中下一路径分量
		
		//获取失败
		if (NULL == pINode) 
			return NULL;
	}

out:
	this->inodeTable->IPut(pINode);
	return NULL;
}

/* 被Creat()系统调用使用，用于为创建新文件分配内核资源
*为新创建的文件写新的i节点和新的目录项，返回的pInode是该文件的内存i节点，其中的i_count是 1。
*在程序的最后会调用 WriteDir，在这里把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。*/
Inode* SysCall::MakNode(int mode)
{
	Inode* pINode;
	//分配一个空闲DiskInode，里面内容已全部清空
	pINode = this->fileSystem->IAlloc();
	if (NULL == pINode)
		return NULL;

	pINode->i_flag = (Inode::IACC | Inode::IUPD);//Inode被访问需要更新
	pINode->i_mode = mode | Inode::IALLOC;//文件被使用
	pINode->i_nlink = 1;//路径数为1
	//将目录项写入目录文件
	this->WriteDir(pINode);
	return pINode;//返回该文件的inode指针
}

//取消文件
void SysCall::UnLink()
{
	// 删除文件夹有磁盘泄露
	Inode* pINode;// 待删除文件inode
	Inode* pDeleteINode; // 父目录inode

	//以DELETE模式搜索目录，pDeleteINode为父目录Inode
	pDeleteINode = this->NameI(SysCall::DELETE);
	if (NULL == pDeleteINode)
		return;

	// 取得要删除的文件inode
	pINode = this->inodeTable->IGet(g_user.curDirEnt.m_ino);
	if (NULL == pINode)
		return;
	//写入清零后的目录项
	g_user.IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	g_user.IOParam.m_Base = (unsigned char*)&g_user.curDirEnt;
	g_user.IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

	g_user.curDirEnt.m_ino = 0;
	pDeleteINode->WriteI();//将数据内容写入缓存块（缓存块延时写回磁盘）
	//修改inode项
	pINode->i_nlink--;
	pINode->i_flag |= Inode::IUPD;

	this->inodeTable->IPut(pDeleteINode);
	this->inodeTable->IPut(pINode);
}

//向父目录的目录文件写入一个目录项：把自己的目录项写入父目录，同时修改父目录文件的inode、写回磁盘
void SysCall::WriteDir(Inode* pINode)
{
	//设置目录项中INode编号部分
	g_user.curDirEnt.m_ino = pINode->i_number;
	//设置目录项中pathname分量部分
	memcpy(g_user.curDirEnt.name, g_user.dbuf, DirectoryEntry::DIRSIZ);

	// 在进行磁盘内容的读写前总要通过修改参数来控制传什么内容（from 用户目标区
	g_user.IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	g_user.IOParam.m_Base = (unsigned char*)&g_user.curDirEnt;

	//将目录项写入父目录文件
	g_user.parDirIP->WriteI();
	this->inodeTable->IPut(g_user.parDirIP);
}

//改变当前工作目录
void SysCall::ChDir()
{
	Inode* pINode;
	// 通过打开文件方式搜索当前目录
	pINode = this->NameI(SysCall::OPEN);
	if (NULL == pINode)
		return;
	//搜索到的文件不是目录文件
	if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {
		g_user.errorCode = User::U_ENOTDIR;
		this->inodeTable->IPut(pINode);
		return;
	}
	g_user.curDirIP = pINode;
	//路径不是从根目录'/'开始，则在现有curdir后面加上当前路径分量
	if (g_user.dirp[0] != '/')
		g_user.curDirPath += g_user.dirp;
	else //如果是从根目录'/'开始，则取代原有工作目录
		g_user.curDirPath = g_user.dirp;
	if (g_user.curDirPath.back() != '/')
		g_user.curDirPath.push_back('/');
}

 //列出当前INode节点的文件项
void SysCall::Ls()
{
	Inode* pINode = g_user.curDirIP;
	Buf* pbuf = NULL;

	g_user.IOParam.m_Offset = 0;
	g_user.IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);// 当前目录项数

	while (g_user.IOParam.m_Count) {
		if (0 == g_user.IOParam.m_Offset % Inode::BLOCK_SIZE) {// 读完了一个buf
			if (pbuf)// 释放之
				g_bufferManager.Brelse(pbuf);
			// 继续读逻辑文件对应的下一磁盘块
			int phyBlkno = pINode->Bmap(g_user.IOParam.m_Offset / Inode::BLOCK_SIZE);
			pbuf = g_bufferManager.Bread(phyBlkno);
		}
		// 读取新的目录项
		memcpy(&g_user.curDirEnt, pbuf->b_addr + (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(g_user.curDirEnt));
		// 更新IO参数
		g_user.IOParam.m_Offset += sizeof(DirectoryEntry);
		g_user.IOParam.m_Count--;

		if (0 == g_user.curDirEnt.m_ino)
			continue;

		// 将当前遍历到的目录项名称加入ls中
		g_user.ls += g_user.curDirEnt.name;
		g_user.ls += "\n";
	}

	if (pbuf)// 释放之
		g_bufferManager.Brelse(pbuf);

}
//重命名文件、文件夹,ori和cur是单纯的名字，不是路径
void SysCall::Rename(string ori, string cur)
{
	Inode* pINode = g_user.curDirIP;// 目录文件inode
	Buf* pbuf = NULL;

	g_user.IOParam.m_Offset = 0;
	g_user.IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);// 当前目录项数

	while (g_user.IOParam.m_Count) {
		if (0 == g_user.IOParam.m_Offset % Inode::BLOCK_SIZE) {// 读完了一个buf
			if (pbuf)
				g_bufferManager.Brelse(pbuf);
			// 继续读逻辑文件对应的下一磁盘块
			int phyBlkno = pINode->Bmap(g_user.IOParam.m_Offset / Inode::BLOCK_SIZE);
			pbuf = g_bufferManager.Bread(phyBlkno);
		}

		// 将读到的目录项记录在temp中
		DirectoryEntry tmp;
		memcpy(&tmp, pbuf->b_addr + (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(g_user.curDirEnt));

		// 当前目录项名字与ori一致
		if (strcmp(tmp.name, ori.c_str()) == 0) {
			// 将当前目录项的名称修改为cur
			strcpy(tmp.name, cur.c_str());
			memcpy(pbuf->b_addr + (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), &tmp, sizeof(g_user.curDirEnt));
		}
		// 更新IO参数
		g_user.IOParam.m_Offset += sizeof(DirectoryEntry);
		g_user.IOParam.m_Count--;
	}

	if (pbuf)// 释放之
		g_bufferManager.Brelse(pbuf);
}