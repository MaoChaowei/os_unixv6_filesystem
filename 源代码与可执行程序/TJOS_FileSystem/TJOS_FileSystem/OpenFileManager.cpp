#include"OpenFileManager.h"
#include"User.h"

extern User g_user;
extern BufferManager g_bufferManager; //�ö����������ķ���ͻ��յ�
extern InodeTable g_inodeTable;//�ö�������ڴ������inode
extern OpenFileTable g_openFileTable;//�ö�������ļ�����Ĺ���
extern FileSystem g_fileSystem;//�ö���������ļ�ϵͳ�洢��Դ

/*******************************OPENFILETABLE*************************************/

OpenFileTable::OpenFileTable()
{
	//nothing to do here
}
OpenFileTable::~OpenFileTable()
{
	//nothing to do here
}

//��������ˢ��
void OpenFileTable::Reset()
{
	for (int i = 0; i < OpenFileTable::MAX_FILES; i++) {
		m_File[i].Reset();
	}
}
//��ϵͳ���ļ����з���һ�����е�File�ṹ  
//���ã����̴��ļ������������ҵĿ�����  ֮ �±�  д�� u_ar0[EAX]
File* OpenFileTable::FAlloc()
{
	//�ڽ��̴��ļ����������л�ȡһ�������� 
	int fd = g_user.ofiles.AllocFreeSlot();

	if (fd < 0) //����ʧ��
	{
		return NULL;
	}
	//�ڴ��ļ������ҿ�����Ŀ�����������̴��ļ���Ĺ���
	for (int i = 0; i < OpenFileTable::MAX_FILES; i++)
	{
		if (m_File[i].f_count == 0) {
			//�������̴��ļ���������������File�ṹ�Ĺ���
			g_user.ofiles.SetF(fd,&m_File[i]);
			//ά��FILE�ṹ
			m_File[i].f_count++;
			m_File[i].f_offset = 0;

			return &m_File[i];
		}
	}

	cout << "ϵͳ���ļ�����û�п��еı������ʧ�ܣ�" << endl;
	g_user.errorCode = User::U_ENFILE;
	return NULL;

}
//�Դ��ļ����ƿ�File�ṹ�����ü���f_count��1�������ü���f_countΪ0�����ͷ�File�ṹ��
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

// �������INode��Ż�ȡ��ӦINode��
// �����INode�Ѿ����ڴ��У�������������,���ظ��ڴ�INode��
// ��������ڴ��У���������ڴ沢���ظ��ڴ�INode
Inode* InodeTable::IGet( int inumber)
{
	Inode* ip;
	int idx = IsLoaded(inumber);

	/* �Ѿ����ڴ��� */
	if (idx >= 0){	
		ip = &m_Inode[idx];
		++ip->i_count;//���������
		return ip;
	}

	/* �����ڴ��� */
	//��ȡһ������inode
	ip = GetFreeInode();

	// ���ڴ�Inode���������������Inodeʧ�� 
	if (!ip) {
		cout << "ERR:��ȡ����inodeʧ�ܣ�inodetable������IGet��" << endl;
		g_user.errorCode = User::U_ENFILE;
		return NULL;
	}

	// ����inode�ɹ��������Inode�����·�����ڴ�Inode
	ip->i_number = inumber;
	++ip->i_count;

	// �������Inode���뻺���� 
	int actual_blkno = FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR;//�߼�inumber������blkno��ת��
	Buf* pbuf = g_bufferManager.Bread(actual_blkno);

	// ���������е����Inode��Ϣ�������·�����ڴ�Inode�� 
	ip->ICopy(pbuf,inumber);
	// �ͷŻ���
	g_bufferManager.Brelse(pbuf);
	return ip;
}

/* close�ļ�ʱ�����Iput
 *      ��Ҫ���Ĳ������ڴ�i�ڵ���� i_count--����Ϊ0���ͷ��ڴ� i�ڵ㡢���иĶ�д�ش���
 * �����ļ�;��������Ŀ¼�ļ������������󶼻�Iput���ڴ�i�ڵ㡣·�����ĵ�����2��·������һ���Ǹ�
 *   Ŀ¼�ļ�������������д������ļ�������ɾ��һ�������ļ���������������д���ɾ����Ŀ¼����ô
 *   	���뽫���Ŀ¼�ļ�����Ӧ���ڴ� i�ڵ�д�ش��̡�
 *   	���Ŀ¼�ļ������Ƿ��������ģ����Ǳ��뽫����i�ڵ�д�ش��̡�*/
void InodeTable::IPut(Inode* pNode)
{
	//��ǰ����Ϊ���ø��ڴ�INode��Ψһ���̣���׼���ͷŸ��ڴ�INode
	if (pNode->i_count == 1) {
		//�����ļ��Ѿ�û��Ŀ¼·��ָ����
		if (pNode->i_nlink <= 0) {
			//�ͷŸ��ļ�ռ�ݵ������̿�
			pNode->ITrunc();
			pNode->i_mode = 0;
			//�ͷŶ�Ӧ�����INode
			m_FileSystem->IFree(pNode->i_number);
		}
		//�������INode��Ϣ
		pNode->IUpdate((int)time(NULL));
		//����ڴ�INode�����б�־λ
		pNode->i_flag = 0;
		//�����ڴ�inode���еı�־֮һ����һ����i_count == 0
		pNode->i_number = -1;
	}
	pNode->i_count--;
}

/*�����б��޸Ĺ����ڴ�Inode���µ���Ӧ���Inode��*/
void InodeTable::UpdateInodeTable()
{
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		if (m_Inode[i].i_count) {
			m_Inode[i].IUpdate((int)time(NULL));
		}
	}
}

/*�����Ϊinumber�����inode�Ƿ����ڴ濽����������򷵻ظ��ڴ�Inode���ڴ�Inode���е����� */
int InodeTable::IsLoaded(int inumber)
{
	// �ڴ�inode����Ѱ�ң��Ҹ�inodeҪ���ڴ�򿪽ṹ(��ָ�����FILE�ṹ)
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		if (m_Inode[i].i_number == inumber && m_Inode[i].i_count != 0)
			return i;
	}
	return -1;
}

/*���ڴ�Inode����Ѱ��һ�����е��ڴ�Inode*/
Inode* InodeTable::GetFreeInode()
{
	for (int i = 0; i < InodeTable::MAX_INODE; ++i) {
		if (m_Inode[i].i_count == 0)
			return &m_Inode[i];
	}
	return NULL;
}
