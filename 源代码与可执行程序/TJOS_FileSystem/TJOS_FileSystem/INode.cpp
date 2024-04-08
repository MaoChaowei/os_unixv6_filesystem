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

//����Inode�����е�������̿���������ȡ��Ӧ���ļ�����
void Inode::ReadI()
{
	int lbn;	/* �ļ��߼���� */
	int bn;		/* lbn��Ӧ�������̿�� */
	int offset;	/* ��ǰ�ַ�������ʼ����λ�� */
	int nbytes;	/* �������û�Ŀ�����ֽ����� */
	Buf* pbuf;

	if (0 == g_user.IOParam.m_Count) {
		/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
		return;
	}
	i_flag |= Inode::IACC;

	/* һ��һ���ַ���ض�������ȫ�����ݣ�ֱ�������ļ�β */
	while (User::U_NOERROR == g_user.errorCode && g_user.IOParam.m_Count!=0) {
		//1 ����lbn\bn\offset\nbytes
		lbn = bn = g_user.IOParam.m_Offset / Inode::BLOCK_SIZE;
		offset=g_user.IOParam.m_Offset% Inode::BLOCK_SIZE;
		/* ���͵��û������ֽ�������ȡ�������ʣ���ֽ����뵱ǰ�ַ�������Ч�ֽ�����Сֵ */
		nbytes = min(Inode::BLOCK_SIZE-offset,g_user.IOParam.m_Count);
		int remain = i_size - g_user.IOParam.m_Offset;
		if (remain <= 0)/* ����Ѷ��������ļ���β */
			return;
		nbytes = min(nbytes,remain);/* ���͵��ֽ�������ȡ����ʣ���ļ��ĳ��� */
		if ((bn = Bmap(lbn)) == 0)
			return;
		
		//2 ��bn�̿�����������
		pbuf = g_bufferManager.Bread(bn);

		unsigned char* start = pbuf->b_addr + offset;//������������ʼ��λ��
		//3 ������������д�뵽�û�Ŀ����
		memcpy(g_user.IOParam.m_Base,start,nbytes);

		//4 ����IO����
		g_user.IOParam.m_Base += nbytes;
		g_user.IOParam.m_Offset += nbytes;
		g_user.IOParam.m_Count -= nbytes;
		//�ͷŻ���
		g_bufferManager.Brelse(pbuf);
	}

}
//����Inode�����е�������̿�������������д���ļ�		PS:������̿����Bmap��ά�����ˣ�ֱ���ã�����ֻ���޸�i_size
void Inode::WriteI()
{
	int lbn;	// �ļ��߼���� 
	int bn;		// lbn��Ӧ�������̿��
	int offset;	// ��ǰ�ַ�������ʼ����λ��
	int nbytes;	// �������û�Ŀ�����ֽ�����
	Buf* pbuf;

	// ����Inode�����ʱ�־λ
	this->i_flag |= (Inode::IACC | Inode::IUPD);

	if (0 == g_user.IOParam.m_Count) {
		/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
		return;
	}
	while (User::U_NOERROR == g_user.errorCode && g_user.IOParam.m_Count != 0) {
		// 1 ����lbn\bn\offset\nbytes
		lbn = bn = g_user.IOParam.m_Offset / Inode::BLOCK_SIZE;
		offset = g_user.IOParam.m_Offset % Inode::BLOCK_SIZE;
		/* ���͵��û������ֽ�������ȡ�������ʣ���ֽ����뵱ǰ�ַ�������Ч�ֽ�����Сֵ */
		nbytes = min(Inode::BLOCK_SIZE - offset, g_user.IOParam.m_Count);
		if ((bn = Bmap(lbn)) == 0)
			return;

		// ���д������������һ���ַ��飬��Ϊ����仺��
		if (Inode::BLOCK_SIZE == nbytes)
			pbuf = g_bufferManager.GetBlk(bn);
		// �����ȶ���д
		else
			pbuf = g_bufferManager.Bread(bn);

		unsigned char* start = pbuf->b_addr + offset;//������������ʼ��λ��
		// ���û�������д�뻺���
		memcpy(start,g_user.IOParam.m_Base,nbytes);
		// ����IO����
		g_user.IOParam.m_Base += nbytes;
		g_user.IOParam.m_Offset += nbytes;
		g_user.IOParam.m_Count -= nbytes;

		if (g_user.errorCode != User::U_NOERROR)/* д�����г����ͷŻ��� */
			g_bufferManager.Brelse(pbuf);
		else /* д���������򣺻�����ӳ�д */
			g_bufferManager.Bdwrite(pbuf);
		
		//��ͨ�ļ���������
		if (i_size < g_user.IOParam.m_Offset)
			i_size = g_user.IOParam.m_Offset;

		i_flag |= Inode::IUPD;
	}
}

//���ļ����߼����ת���ɶ�Ӧ��"�����̿��"
int Inode::Bmap(int lbn)
{
	/* Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
	 * (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
	 * (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
	 * �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
	 * (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
	 * �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
	 * (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6) */

	Buf* pfirstBuf, * psecondBuf;
	int phyBlkno;	// ת����������̿��
	int* iTable;	// ���ڷ��������̿���һ�μ�ӡ����μ��������
	int index;

	//������Χ������0
	if (lbn >= Inode::HUGE_FILE_BLOCK) {
		g_user.errorCode = User::U_EFBIG;
		return 0;
	}

	//С���ļ����ӻ���������i_addr[0-5]�л�������̿�ż���
	if (lbn < 6) {
		phyBlkno = i_addr[lbn];
		//������߼���Ż�û����Ӧ�������̿����֮��Ӧ�������һ�������
		if (0 == phyBlkno && (pfirstBuf = g_fileSystem.Alloc()) != NULL) {
			phyBlkno = pfirstBuf->b_blkno;
			/*��Ϊ����ܿ������ϻ�Ҫ�õ��˴��·�������ݿ飬���Բ��������������������*/
			g_bufferManager.Bdwrite(pfirstBuf);
			i_addr[lbn] = phyBlkno;
			i_flag |= Inode::IUPD;
		}
		return phyBlkno;
	}

	//���;����ļ�
	/* �����߼����lbn��Ӧi_addr[]�е����� */
	if (lbn < Inode::LARGE_FILE_BLOCK) 
	{//�����ļ�
		index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
	}
	else
	{//�����ļ�
		index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
	}

	phyBlkno = i_addr[index];
	if (phyBlkno)// �����洢�����������ַ��� 
	{
		pfirstBuf = g_bufferManager.Bread(phyBlkno);
	}
	else// ��ʾ��������Ӧ�ļ���������
	{
		i_flag |= Inode::IUPD;
		/* ����һ�����̿��ż�������� */
		if ((pfirstBuf = g_fileSystem.Alloc()) == NULL)
			return 0;//����ʧ��
		i_addr[index] = pfirstBuf->b_blkno;
	}

	iTable = (int*)pfirstBuf->b_addr;//��char*ָ��ת��Ϊint*ָ�룬��Ϊÿ��������Ŀ��4B�����ڷ���
	if (index >= 8) {
		//���ھ����ļ��������pFirstBuf���Ƕ��μ��������
		//��������߼���ţ����ɶ��μ���������ҵ�һ�μ��������
		index= ((lbn - Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
		phyBlkno = iTable[index];

		if (phyBlkno) {
			// �ͷŶ��μ��������ռ�õĻ��棬������һ�μ�������� 
			g_bufferManager.Brelse(pfirstBuf);
			psecondBuf = g_bufferManager.Bread(phyBlkno);
		}
		else {
			if ((psecondBuf = g_fileSystem.Alloc()) == NULL) {
				//������̿�ʧ��
				g_bufferManager.Brelse(pfirstBuf);
				return 0;
			}
			//����ɹ�
			/* ���·����һ�μ����������̿�ţ�������μ����������Ӧ�� */
			iTable[index] = psecondBuf->b_blkno;
			/* �����ĺ�Ķ��μ���������ӳ�д��ʽ��������� */
			g_bufferManager.Bdwrite(pfirstBuf);
		}
		pfirstBuf = psecondBuf;
		/* ��iTableָ��һ�μ�������� */
		iTable = (int*)psecondBuf->b_addr;
	}

	/* �����߼����lbn����λ��һ�μ���������еı������index */
	if (lbn < Inode::LARGE_FILE_BLOCK)
		index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK;
	else
		index = (lbn - Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK;

	if ((phyBlkno = iTable[index]) == 0 && (psecondBuf = g_fileSystem.Alloc()) != NULL) {
		/* �����䵽���ļ������̿�ŵǼ���һ�μ���������� */
		phyBlkno = psecondBuf->b_blkno;
		iTable[index] = phyBlkno;
		/* �������̿顢���ĺ��һ�μ�����������ӳ�д��ʽ��������� */
		g_bufferManager.Bdwrite(psecondBuf);
		g_bufferManager.Bdwrite(pfirstBuf);
	}
	else  /* �ͷ�һ�μ��������ռ�û��� */
		g_bufferManager.Bdwrite(pfirstBuf);

	return phyBlkno;
}

//�������Inode�����ķ���ʱ�䡢�޸�ʱ��
void Inode::IUpdate(int time)
{
	Buf* pbuf;
	DiskInode dInode;
	//ע�⣺ֻ�е�IUPD��IACC��־֮һ�����ã�����Ҫ������ӦDiskInode
	//Ŀ¼����������������;����Ŀ¼�ļ���IACC��IUPD��־
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

		// ��dInode�е������ݸ��ǻ����еľ����Inode 
		unsigned char* p = pbuf->b_addr + (this->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
		DiskInode* pNode = &dInode;
		memcpy(p, pNode, sizeof(DiskInode));
		// ��������д�����Inode��ʵ�ָ������Inode��Ŀ�ġ�
		g_bufferManager.Bwrite(pbuf);
	}
}

//�ͷ�Inode��Ӧ�ļ�ռ�õĴ��̿�
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
	/* ����IUPD��־λ����ʾ���ڴ�Inode��Ҫͬ������Ӧ���Inode */
	i_flag |= Inode::IUPD;
	/* ����ļ���־ */
	i_mode &= ~(Inode::ILARG);
	i_nlink = 1;
}

// ���Inode�����е�����
void Inode::Clean()
{
	/* Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
	 * �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
	 * ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
	 * �����ʼ��Ϊ��Чֵ�� */
	this->i_mode = 0;
	this->i_nlink = 0;
	this->i_size = 0;
	memset(i_addr,0,sizeof(i_addr));
}

// ��������е����Inode��Ϣ��mode\size\nlink\addr[10]���������·�����ڴ�Inode�� 
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