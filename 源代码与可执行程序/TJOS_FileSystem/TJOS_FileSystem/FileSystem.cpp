#include<ctime>
#include"FileSystem.h"
#include"User.h"
#include"DiskDriver.h"
#include"OpenFileManager.h"
#include"BufferManager.h"

extern DiskDriver g_diskDriver;
extern BufferManager g_bufferManager; //�ö����������ķ���ͻ��յ�
extern InodeTable g_inodeTable;//�ö�������ڴ������inode
extern User g_user;//ȫ���û�����Ϊ�û��ṩ�ļ�ϵͳ��װ�õĽӿ�
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

//��ʽ��SuperBlock
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

//��ʽ�������ļ�ϵͳ
void FileSystem::FormatDevice()
{
	//��ʽ��SpuerBlock���ݽṹ
	this->FormatSuperBlock();
	//��ʽ������
	this->diskDriver->CreateDiskFile();
	//д��superblockռ�ݿռ�
	this->diskDriver->DiskWrite((unsigned char*)(superBlock),sizeof(SuperBlock),0);

	//���ø�Ŀ¼����һ��dinode��д��superblock��
	DiskInode rootDinode;//��Ŀ¼dinode
	rootDinode.d_mode |= Inode::IALLOC | Inode::IFDIR;
	rootDinode.d_nlink = 1;
	this->diskDriver->DiskWrite((unsigned char*)&rootDinode,sizeof(rootDinode));

	//��������ʼ������diskinode(��1��ʼ��0�̶����ڸ�Ŀ¼��/��)
	DiskInode emptyDinode;//��dinode
	for (int i = 1; i < FileSystem::INODE_NUMBER_ALL; ++i) {
		if (superBlock->s_ninode < SuperBlock::MAX_INODE) {//˳�����s_inode
			superBlock->s_inode[superBlock->s_ninode++] = i;
		}
		this->diskDriver->DiskWrite((unsigned char*)&emptyDinode, sizeof(emptyDinode));
	}

	//�����̿鿪ʼ��ʼ��
	char freeblk[FileSystem::BLOCK_SIZE], freeblk1[FileSystem::BLOCK_SIZE];
	memset(freeblk,0, FileSystem::BLOCK_SIZE);
	memset(freeblk1, 0, FileSystem::BLOCK_SIZE);

	//....mark����
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
	//�ٴ�д��superblock
	diskDriver->DiskWrite((unsigned char*)(superBlock), sizeof(SuperBlock), 0);

}

//ϵͳ��ʼ��ʱ����SuperBlock
void FileSystem::LoadSuperBlock()
{
	fseek(this->diskDriver->diskFilePointer,0,0);
	this->diskDriver->DiskRead((unsigned char*)(superBlock), sizeof(SuperBlock), 0);

}
//��SuperBlock������ڴ渱�����µ��洢�豸��SuperBlock��ȥ
void FileSystem::Update()
{
	Buf* pbuf;
	//�޸ı�־λ��fmod��Ϊ0��ʾ��ͬ��
	superBlock->s_fmod = 0;
	superBlock->s_time = (int)time(NULL);
	//���Ǵ��̵ĳ�����
	for (int i = 0; i < 2; i++) {
		int* p = (int*)superBlock + i * 128;/* 128=512/4 */
		pbuf = this->bufManager->GetBlk(FileSystem::SUPERBLOCK_START_SECTOR+i);
		memcpy(pbuf->b_addr,p,FileSystem::BLOCK_SIZE);//�ڴ��г�����д�뻺���
		this->bufManager->Bwrite(pbuf);//�����д�ش���
	}
	//DiskInode�����ݿ�ĸ���
	g_inodeTable.UpdateInodeTable();/*�����б��޸Ĺ����ڴ�Inode���µ���Ӧ���Inode��*/
	this->bufManager->Bflush();//�����������ɶ������ӳ�д�Ļ���ȫ�����������
}

/* ����Inode�ڵ�ķ���������㷨�����ʵ�֣��ڴ洢�豸�Ϸ���һ���������INode��һ�����ڴ����µ��ļ� */
Inode* FileSystem::IAlloc()
{
	Buf* pbuf;
	Inode* pinode;
	int ino;	/* ���䵽�Ŀ������Inode��� */

	//�����������������Ϊ��
	if (this->superBlock->s_ninode <= 0)
	{
		ino = -1;
		//����inode�������ҿ���diskinode
		for (int i = 0; i < this->superBlock->s_isize; ++i) 
		{
			//��ȡ��8��diskinode�Ĵ��̿鵽�����
			pbuf = this->bufManager->Bread(FileSystem::INODE_ZONE_START_SECTOR+i);
			int* p = (int*)pbuf->b_addr;
			//������diskinode
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR;++j) {
				++ino;
				int mode = *(p+j*FileSystem::INODE_SIZE/sizeof(int));//��λ��d_mode����ʼ��Ϊ0
				if (mode)
					continue;//ռ�������
				//������inode��i_mode == 0����ʱ������ȷ����inode�ǿ��еģ�
				//��Ϊ�п������ڴ�inodeû��д��������, ����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
				if (g_inodeTable.IsLoaded(ino) == -1) {
					//inodetable��û�ж�Ӧ�ڴ�inode�����Կ��Լ��뵽������Ŀ���inode����
					superBlock->s_inode[superBlock->s_ninode++] = ino;
					//���˾Ͳ��ܼ�����,��������
					if (superBlock->s_ninode >= SuperBlock::MAX_INODE)
						break;
				}
			}
			this->bufManager->Brelse(pbuf);
			//���˾Ͳ��ܼ�����,��������
			if (superBlock->s_ninode >= SuperBlock::MAX_INODE)
				break;
		}//�������������s_inode

		if (superBlock->s_ninode <= 0) 
		{
			//s_inode��ȻΪ�գ�˵��inode���Ѿ�ȫ��ռ��
			g_user.errorCode = User::U_ENOSPC;
			return NULL;
		}
	}
	//��������ջ������ȡ����diskinode���
	ino = superBlock->s_inode[--superBlock->s_ninode];
	//����diskinode�����ڴ�
	pinode = g_inodeTable.IGet(ino);
	if (pinode == NULL) {
		cout << "ERR:diskinode�����ڴ�ʧ�ܣ���IAlloc��" << endl;
		return NULL;
	}

	pinode->Clean();
	superBlock->s_fmod = 1;
	return pinode;

}
//�ͷű��Ϊnumber�����INode��һ������ɾ���ļ���ֻ�迼��Ҫ��Ҫ����superblock
void FileSystem::IFree(int number)
{
	/* ���������ֱ�ӹ���Ŀ������Inode����100����
	 * ͬ�����ͷŵ����Inodeɢ���ڴ���Inode���С� */
	if (superBlock->s_ninode >= SuperBlock::MAX_INODE) {
		return;
	}
	superBlock->s_inode[superBlock->s_ninode++] = number;
	superBlock->s_fmod = 1;/* ����SuperBlock���޸ı�־ */

}

//�ڴ洢�豸�Ϸ�����д��̿飬��s_free��ȡ�����������ӷ�ʽ 
Buf* FileSystem::Alloc()
{
	int blkno;//���䵽�Ŀ��д��̿���
	Buf* pbuf;//�洢�ÿ��д������ݵĻ����

	//��������ջ������ȡ���д��̿���
	blkno = superBlock->s_free[--superBlock->s_nfree];
	//����ȡ���̿���Ϊ�㣬���ʾ�ѷ��価���еĿ��д��̿�
	if (blkno <= 0) {
		superBlock->s_nfree = 0;
		g_user.errorCode = User::U_ENOSPC;
		return NULL;
	}
	//�������ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��
	//��Ҫ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]��
	if (superBlock->s_nfree <= 0) {
		pbuf = this->bufManager->Bread(blkno);						/* ����ÿ��д��̿� */
		int* p = (int*)pbuf->b_addr;								/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		superBlock->s_nfree = *p++;									/* ���ȶ��������̿���s_nfree */
		memcpy(superBlock->s_free, p, sizeof(superBlock->s_free));	/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		this->bufManager->Brelse(pbuf);								/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
	}

	//�ɹ��õ�һ�����
	pbuf = this->bufManager->GetBlk(blkno);//���뻺��
	if (pbuf)
		this->bufManager->ClrBuf(pbuf);//��ջ���
	superBlock->s_fmod = 1;//����SuperBlock���޸ı�־

	return pbuf;
}

 //�ͷŴ洢�豸�ϱ��Ϊblkno�Ĵ��̿飬����s_free���������ӷ�ʽ 
void FileSystem::Free(int blkno)
{
	Buf* pbuf;
	//�����ǰ�����Ѿ����ˣ�Ҫ��ǰ������µ�blkno������
	if (superBlock->s_nfree >= SuperBlock::MAX_FREE) {
		pbuf = this->bufManager->GetBlk(blkno);//Ϊ�ô������뻺��
		//��superblock�е�����д��blkno�Ļ���
		int* p = (int*)pbuf->b_addr;
		*p++ = superBlock->s_nfree;
		memcpy(p, superBlock->s_free, sizeof(int) * SuperBlock::MAX_FREE);
		//����superblk
		superBlock->s_nfree = 0;
		//���û���д�ش���
		this->bufManager->Bwrite(pbuf);
	}
	//����superblk
	superBlock->s_free[superBlock->s_nfree++] = blkno;
	superBlock->s_fmod = 1;
}