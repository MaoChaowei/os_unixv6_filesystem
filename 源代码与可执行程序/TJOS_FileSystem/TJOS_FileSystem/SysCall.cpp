#include"User.h"
#include"SysCall.h"
#include"BufferManager.h"

extern BufferManager g_bufferManager; //�ö����������ķ���ͻ��յ�
extern FileSystem g_fileSystem;//�ö���������ļ�ϵͳ�洢��Դ
extern InodeTable g_inodeTable;//�ö�������ڴ������inode
extern OpenFileTable g_openFileTable;//�ö�������ļ�����Ĺ���
extern User g_user;//ȫ���û�����Ϊ�û��ṩ�ļ�ϵͳ��װ�õĽӿ�

SysCall::SysCall()
{
	this->fileSystem = &g_fileSystem;
	this->openFileTable = &g_openFileTable;
	this->inodeTable = &g_inodeTable;
	this->rootDirInode = inodeTable->IGet(FileSystem::ROOT_INODE_NO);
	rootDirInode->i_count += 0xff;//������Ŀ¼��inode

}
SysCall::~SysCall()
{
	//nothing much
}

//Open()ϵͳ���ô������,���ܣ����ļ��������ļ��Ĵ򿪽ṹ��i_count Ϊ������i_count ++��
void SysCall::Open()
{
	Inode* pinode = this->NameI(SysCall::OPEN);
	if (!pinode)/* û���ҵ���Ӧ��Inode */
		return;
	this->Open1(pinode,0);

}

//Creat()ϵͳ���ô������
/* ���ܣ�����һ���µ��ļ�
 * Ч�����������ļ��ṹ��i_count Ϊ������Ӧ���� 1�� */
void SysCall::Creat()
{
	Inode* pinode;
	int newACCMode = g_user.arg[1];//��ŵ�ǰϵͳ���ò��� �ļ����ͣ�Ŀ¼�ļ�
	
	pinode = this->NameI(SysCall::CREATE);//����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д��������

	//û���ҵ���Ӧ��INode����NameI����
	if (NULL == pinode) {
		if (g_user.errorCode!=User::U_NOERROR)
			return;

		pinode = this->MakNode(newACCMode);
		if (NULL == pinode)
			return;
		//������������ֲ����ڣ�ʹ�ò���trf = 2������open1()
		this->Open1(pinode, 2);
		return;
	}
	//���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()��
	this->Open1(pinode, 1);
	pinode->i_mode |= newACCMode;
}

//Open()��Creat()ϵͳ���õĹ������֣������ļ��Ĵ򿪽ṹ��
//trf == 0��open����
//trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
//trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
//mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
void SysCall::Open1(Inode* pINode, int trf)
{
	//create�ļ�������ͬ���ļ����ͷ�����ռ�е��̿�
	if (1 == trf)
		pINode->ITrunc();
	//������ļ����ƿ�File�ṹ
	File* pFile = this->openFileTable->FAlloc();//��ϵͳ���ļ����з���һ�����е�File�ṹ
	if (NULL == pFile) {
		this->inodeTable->IPut(pINode);
		return;
	}
	pFile->f_inode = pINode;

	//Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬��������
	if (g_user.errorCode == 0)
		return;
	else { //����������ͷ���Դ
		//�ͷŴ��ļ�������
		int fd = g_user.ar0[User::EAX];
		if (fd != -1) {
			g_user.ofiles.SetF(fd, NULL);
			//�ݼ�File�ṹ��Inode�����ü��� ,File�ṹû���� f_countΪ0�����ͷ�File�ṹ��
			pFile->f_count--;
		}
		this->inodeTable->IPut(pINode);
	}
}
 //Close()ϵͳ���ô������ 
void SysCall::Close()
{
	int fd = g_user.arg[0];
	// ��ȡ���ļ����ƿ�File�ṹ 
	File* pfile = g_user.ofiles.GetF(fd);
	if (NULL == pfile)
		return;
	// �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü���
	g_user.ofiles.SetF(fd, NULL);
	this->openFileTable->CloseF(pfile);
}

//Seek()ϵͳ���ô�����̣�arg0~2��Ӧfd(�ļ����)��offset��ƫ��ģʽ
void SysCall::Seek()
{
	File* pFile;
	int fd = g_user.arg[0];

	pFile = g_user.ofiles.GetF(fd);
	if (NULL == pFile)
		return; //��FILE�����ڣ�GetF���������

	int offset = g_user.arg[1];

	switch (g_user.arg[2]) {
		case 0:
			//��дλ������Ϊoffset
			pFile->f_offset = offset;
			break;
		case 1:
			//��дλ�ü�offset(�����ɸ�)
			pFile->f_offset += offset;
			break;
		case 2:
			//��дλ�õ���Ϊ�ļ����ȼ�offset
			pFile->f_offset = pFile->f_inode->i_size + offset;
			break;
		default:
			break;
	}
	cout << "* SysCall::Seek()>> �ļ�ָ��ɹ��ƶ��� " << pFile->f_offset << endl;
}
//Read()ϵͳ���ô������
void SysCall::Read()
{
	//ֱ�ӵ���Rdwr()��������
	this->Rdwr(File::FREAD);
}
//Write()ϵͳ���ô������
void SysCall::Write()
{
	//ֱ�ӵ���Rdwr()��������
	this->Rdwr(File::FWRITE);
}

//��дϵͳ���ù������ִ���
void SysCall::Rdwr(enum File::FileFlags mode)
{
	File* pFile;
	//����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ
	pFile = g_user.ofiles.GetF(g_user.arg[0]);
	if (NULL == pFile) //�����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ��������
		return;
	//��ȡ��д������IOParam
	g_user.IOParam.m_Base = (unsigned char*)g_user.arg[1]; //Ŀ�껺������ַ
	g_user.IOParam.m_Count = g_user.arg[2];                //Ҫ���/д���ֽ���
	g_user.IOParam.m_Offset = pFile->f_offset; //�����ļ���ʼ��λ��

	//����mode���ж���д
	if (File::FREAD == mode)
		pFile->f_inode->ReadI();
	else
		pFile->f_inode->WriteI();

	//���ݶ�д�������ƶ��ļ���дƫ��ָ��
	pFile->f_offset += (g_user.arg[2] - g_user.IOParam.m_Count);
	//����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ
	g_user.ar0[User::EAX] = g_user.arg[2] - g_user.IOParam.m_Count;

}

//Ŀ¼��������·��ת��Ϊ��Ӧ��INode����INode,����NULL��ʾĿ¼����ʧ�ܣ������Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ� 
Inode* SysCall::NameI(enum DirSearchMode mode)
{
	Inode* pINode = g_user.curDirIP;
	Buf* pbuf;
	int freeEntryOffset; //�Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ����
	unsigned int index = 0, nindex = 0;

	// �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ����������ӽ��̵�ǰ����Ŀ¼��ʼ����
	// ·��ͨ��drip�����
	if ('/' == g_user.dirp[0]) {
		nindex = ++index + 1;
		pINode = this->rootDirInode;
	}
	//���ѭ����ÿ�δ���pathname��һ��·������
	while (1) {
		//����������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳�
		if (g_user.errorCode != User::U_NOERROR)
			break;
		//����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ�����
		if (nindex > g_user.dirp.length())
			return pINode;
		//���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳�
		if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {//�ļ����ͣ�Ŀ¼�ļ�
			g_user.errorCode = User::U_ENOTDIR;//�ļ��в�����
			break;
		}

		//��Pathname�е�ǰ׼������ƥ���·������������dbuf[]�У����ں�Ŀ¼����бȽϡ�
		nindex = g_user.dirp.find_first_of('/', index);
		memset(g_user.dbuf, 0, sizeof(g_user.dbuf));
		memcpy(g_user.dbuf, g_user.dirp.data() + index, (nindex == (unsigned int)string::npos ? g_user.dirp.length() : nindex) - index);
		index = nindex + 1;

		//�ڲ�ѭ�����֣�����dbuf[]�е�·���������������Ѱƥ���Ŀ¼��
		g_user.IOParam.m_Offset= 0;
		g_user.IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);//����ΪĿ¼����� �����հ׵�Ŀ¼��
		freeEntryOffset = 0;
		pbuf = NULL;
		while (1) {
			// ��Ŀ¼���Ѿ�������� 
			if (0 == g_user.IOParam.m_Count) {
				if (NULL != pbuf)
					g_bufferManager.Brelse(pbuf);
				//����Ǵ������ļ�
				if (SysCall::CREATE == mode && nindex >= g_user.dirp.length()) {
					//����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ�
					g_user.parDirIP = pINode;
					if (freeEntryOffset) //�˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ����
						g_user.IOParam.m_Offset = freeEntryOffset - sizeof(DirectoryEntry); //������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ�
					else
						pINode->i_flag |= Inode::IUPD;
					//�ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()��������
					return NULL;
				}
				//Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����˳�
				g_user.errorCode = User::U_ENOENT;
				goto out;
			}
			// �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿�
			if (0 == g_user.IOParam.m_Offset % Inode::BLOCK_SIZE) {
				if (pbuf)
					g_bufferManager.Brelse(pbuf);
				//����Ҫ���������̿��
				int phyBlkno = pINode->Bmap(g_user.IOParam.m_Offset / Inode::BLOCK_SIZE);
				pbuf = g_bufferManager.Bread(phyBlkno);
			}
			// û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����curDirEnt
			memcpy(&g_user.curDirEnt, pbuf->b_addr+ (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(g_user.curDirEnt));
			g_user.IOParam.m_Offset += sizeof(DirectoryEntry);
			g_user.IOParam.m_Count--;
			// ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ����
			if (0 == g_user.curDirEnt.m_ino) {
				if (0 == freeEntryOffset)
					freeEntryOffset = g_user.IOParam.m_Offset;
				//��������Ŀ¼������Ƚ���һĿ¼��
				continue;
			}

			if (!memcmp(g_user.dbuf, &g_user.curDirEnt.name, sizeof(DirectoryEntry) - 4))
				break;
		}

		// ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname�е�ǰ·������ƥ��ɹ���
		// ����ƥ��pathname����һ·��������ֱ������'\0'����
		if (pbuf)
			g_bufferManager.Brelse(pbuf);

		// �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����g_user.curDirEnt.m_ino��
		if (SysCall::DELETE == mode && nindex >= g_user.dirp.length())
			return pINode;

		// ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode
		this->inodeTable->IPut(pINode);
		pINode = this->inodeTable->IGet(g_user.curDirEnt.m_ino);
		// �ص����While(true)ѭ��������ƥ��Pathname����һ·������
		
		//��ȡʧ��
		if (NULL == pINode) 
			return NULL;
	}

out:
	this->inodeTable->IPut(pINode);
	return NULL;
}

/* ��Creat()ϵͳ����ʹ�ã�����Ϊ�������ļ������ں���Դ
*Ϊ�´������ļ�д�µ�i�ڵ���µ�Ŀ¼����ص�pInode�Ǹ��ļ����ڴ�i�ڵ㣬���е�i_count�� 1��
*�ڳ����������� WriteDir��������������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�*/
Inode* SysCall::MakNode(int mode)
{
	Inode* pINode;
	//����һ������DiskInode������������ȫ�����
	pINode = this->fileSystem->IAlloc();
	if (NULL == pINode)
		return NULL;

	pINode->i_flag = (Inode::IACC | Inode::IUPD);//Inode��������Ҫ����
	pINode->i_mode = mode | Inode::IALLOC;//�ļ���ʹ��
	pINode->i_nlink = 1;//·����Ϊ1
	//��Ŀ¼��д��Ŀ¼�ļ�
	this->WriteDir(pINode);
	return pINode;//���ظ��ļ���inodeָ��
}

//ȡ���ļ�
void SysCall::UnLink()
{
	// ɾ���ļ����д���й¶
	Inode* pINode;// ��ɾ���ļ�inode
	Inode* pDeleteINode; // ��Ŀ¼inode

	//��DELETEģʽ����Ŀ¼��pDeleteINodeΪ��Ŀ¼Inode
	pDeleteINode = this->NameI(SysCall::DELETE);
	if (NULL == pDeleteINode)
		return;

	// ȡ��Ҫɾ�����ļ�inode
	pINode = this->inodeTable->IGet(g_user.curDirEnt.m_ino);
	if (NULL == pINode)
		return;
	//д��������Ŀ¼��
	g_user.IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	g_user.IOParam.m_Base = (unsigned char*)&g_user.curDirEnt;
	g_user.IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

	g_user.curDirEnt.m_ino = 0;
	pDeleteINode->WriteI();//����������д�뻺��飨�������ʱд�ش��̣�
	//�޸�inode��
	pINode->i_nlink--;
	pINode->i_flag |= Inode::IUPD;

	this->inodeTable->IPut(pDeleteINode);
	this->inodeTable->IPut(pINode);
}

//��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼����Լ���Ŀ¼��д�븸Ŀ¼��ͬʱ�޸ĸ�Ŀ¼�ļ���inode��д�ش���
void SysCall::WriteDir(Inode* pINode)
{
	//����Ŀ¼����INode��Ų���
	g_user.curDirEnt.m_ino = pINode->i_number;
	//����Ŀ¼����pathname��������
	memcpy(g_user.curDirEnt.name, g_user.dbuf, DirectoryEntry::DIRSIZ);

	// �ڽ��д������ݵĶ�дǰ��Ҫͨ���޸Ĳ��������ƴ�ʲô���ݣ�from �û�Ŀ����
	g_user.IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	g_user.IOParam.m_Base = (unsigned char*)&g_user.curDirEnt;

	//��Ŀ¼��д�븸Ŀ¼�ļ�
	g_user.parDirIP->WriteI();
	this->inodeTable->IPut(g_user.parDirIP);
}

//�ı䵱ǰ����Ŀ¼
void SysCall::ChDir()
{
	Inode* pINode;
	// ͨ�����ļ���ʽ������ǰĿ¼
	pINode = this->NameI(SysCall::OPEN);
	if (NULL == pINode)
		return;
	//���������ļ�����Ŀ¼�ļ�
	if ((pINode->i_mode & Inode::IFMT) != Inode::IFDIR) {
		g_user.errorCode = User::U_ENOTDIR;
		this->inodeTable->IPut(pINode);
		return;
	}
	g_user.curDirIP = pINode;
	//·�����ǴӸ�Ŀ¼'/'��ʼ����������curdir������ϵ�ǰ·������
	if (g_user.dirp[0] != '/')
		g_user.curDirPath += g_user.dirp;
	else //����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼
		g_user.curDirPath = g_user.dirp;
	if (g_user.curDirPath.back() != '/')
		g_user.curDirPath.push_back('/');
}

 //�г���ǰINode�ڵ���ļ���
void SysCall::Ls()
{
	Inode* pINode = g_user.curDirIP;
	Buf* pbuf = NULL;

	g_user.IOParam.m_Offset = 0;
	g_user.IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);// ��ǰĿ¼����

	while (g_user.IOParam.m_Count) {
		if (0 == g_user.IOParam.m_Offset % Inode::BLOCK_SIZE) {// ������һ��buf
			if (pbuf)// �ͷ�֮
				g_bufferManager.Brelse(pbuf);
			// �������߼��ļ���Ӧ����һ���̿�
			int phyBlkno = pINode->Bmap(g_user.IOParam.m_Offset / Inode::BLOCK_SIZE);
			pbuf = g_bufferManager.Bread(phyBlkno);
		}
		// ��ȡ�µ�Ŀ¼��
		memcpy(&g_user.curDirEnt, pbuf->b_addr + (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(g_user.curDirEnt));
		// ����IO����
		g_user.IOParam.m_Offset += sizeof(DirectoryEntry);
		g_user.IOParam.m_Count--;

		if (0 == g_user.curDirEnt.m_ino)
			continue;

		// ����ǰ��������Ŀ¼�����Ƽ���ls��
		g_user.ls += g_user.curDirEnt.name;
		g_user.ls += "\n";
	}

	if (pbuf)// �ͷ�֮
		g_bufferManager.Brelse(pbuf);

}
//�������ļ����ļ���,ori��cur�ǵ��������֣�����·��
void SysCall::Rename(string ori, string cur)
{
	Inode* pINode = g_user.curDirIP;// Ŀ¼�ļ�inode
	Buf* pbuf = NULL;

	g_user.IOParam.m_Offset = 0;
	g_user.IOParam.m_Count = pINode->i_size / sizeof(DirectoryEntry);// ��ǰĿ¼����

	while (g_user.IOParam.m_Count) {
		if (0 == g_user.IOParam.m_Offset % Inode::BLOCK_SIZE) {// ������һ��buf
			if (pbuf)
				g_bufferManager.Brelse(pbuf);
			// �������߼��ļ���Ӧ����һ���̿�
			int phyBlkno = pINode->Bmap(g_user.IOParam.m_Offset / Inode::BLOCK_SIZE);
			pbuf = g_bufferManager.Bread(phyBlkno);
		}

		// ��������Ŀ¼���¼��temp��
		DirectoryEntry tmp;
		memcpy(&tmp, pbuf->b_addr + (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(g_user.curDirEnt));

		// ��ǰĿ¼��������oriһ��
		if (strcmp(tmp.name, ori.c_str()) == 0) {
			// ����ǰĿ¼��������޸�Ϊcur
			strcpy(tmp.name, cur.c_str());
			memcpy(pbuf->b_addr + (g_user.IOParam.m_Offset % Inode::BLOCK_SIZE), &tmp, sizeof(g_user.curDirEnt));
		}
		// ����IO����
		g_user.IOParam.m_Offset += sizeof(DirectoryEntry);
		g_user.IOParam.m_Count--;
	}

	if (pbuf)// �ͷ�֮
		g_bufferManager.Brelse(pbuf);
}