# include"User.h"
#include<vector>
#include<fstream>

extern SysCall g_sysCall;

// ���ڴ��μ�飬���str�Ƿ�Ϊ����
bool isDegital(string str) {
	for (int i = 0; i < str.size(); i++) {
		if (str.at(i) == '-' && str.size() > 1)  // �п��ܳ��ָ���
			continue;
		if (str.at(i) > '9' || str.at(i) < '0') {
			return false;
		}
	}
	return true;
}

User::User()
{
	this->errorCode = U_NOERROR;
	this->sysCall = &g_sysCall;

	dirp = "/";
	curDirPath = "/";
	curDirIP = sysCall->rootDirInode;// ָ��ǰ��Ŀ¼��inode
	parDirIP = NULL;
	memset(this->arg,0,sizeof(this->arg));
}
User::~User()
{
	// nothing much
}
//��Ŀ¼
void User::u_Ls()
{
	ls.clear();
	sysCall->Ls();
	if (checkError())
		return;
	cout << ls << endl;
}
//�ƶ���
void User::u_Cd(string dirName)
{
	if (!checkPathName(dirName))
		return;
	sysCall->ChDir();
	checkError();
}
//����Ŀ¼
void User::u_MkDir(string dirName)
{
	if (!checkPathName(dirName))
		return;
	arg[1] = Inode::IFDIR;
	sysCall->Creat();
	if (checkError())
		return;
	cout << "SUC: Ŀ¼�����ɹ�" <<endl;
}
//�����ļ�
void User::u_Create(string fileName)
{
	if (!checkPathName(fileName))
		return;
	arg[1] = (Inode::IREAD | Inode::IWRITE);//��ŵ�ǰϵͳ���ò���
	sysCall->Creat();
	if (checkError())
		return;
	cout << "SUC: �ļ�" << fileName << "�����ɹ�!"<<endl;
}
//ɾ���ļ�
void User::u_Delete(string fileName)
{
	if (!checkPathName(fileName))
		return;
	sysCall->UnLink();
	if (checkError())
		return;
	cout << "SUC: �ļ�" << fileName << "ɾ���ɹ�"<<endl;
}
//���ļ�
void User::u_Open(string fileName)
{
	if (!checkPathName(fileName))
		return;
	arg[1] = (File::FREAD | File::FWRITE);//��ŵ�ǰϵͳ���ò���
	sysCall->Open();
	if (checkError())
		return;
	cout << "SUC: �ļ�" << fileName << "�򿪳ɹ������ļ����fdΪ " << ar0[User::EAX] << endl;
}
//�ر��ļ�
void User::u_Close(string fd)
{
	if (!isDegital(fd)) {
		cout << "ERR: ��������ȷ���ļ������fd�ţ���" << endl;
		return;
	}
	arg[0] = stoi(fd);//��ŵ�ǰϵͳ���ò���
	sysCall->Close();
	if (checkError())
		return;
	cout << "SUC: �ļ��رճɹ�"<< endl;
}
//��λ�ļ���дִ��
void User::u_Seek(string fd, string offset, string origin)
{
	if (!isDegital(fd)) {
		cout << "ERR: ��������ȷ���ļ������fd�ţ���" << endl;
		return;
	}
	if (!isDegital(offset)) {
		cout << "ERR: ��������ȷ��ƫ��ֵ<offset>��" << endl;
		return;
	}
	if (!isDegital(origin)) {
		cout << "ERR: ��������ȷ��<mode>��" << endl;
		return;
	}
	arg[0] = stoi(fd);
	arg[1] = stoi(offset);
	arg[2] = stoi(origin);
	sysCall->Seek();
	checkError();
}
//д�ļ�
void User::u_Write(string sfd, string inFile, string size)
{
	if (!isDegital(sfd)) {
		cout << "ERR: ��������ȷ���ļ������fd�ţ���" << endl;
		return;
	}
	if (!isDegital(size)) {
		cout << "ERR: ��������ȷ��д���ֽ���<N>��" << endl;
		return;
	}
	int fd = stoi(sfd), usize = 0;
	if (size.empty() || (usize = stoi(size)) < 0) {
		cout << "ERR: ����<N>���󣡱�����ڵ����� \n";
		return;
	}
	char* buffer = new char[usize];
	fstream fin(inFile, ios::in | ios::binary);
	if (!fin.is_open()) {
		cout << "ERR: �ļ�" << inFile << "��ʧ��" << endl;
		return;
	}
	fin.read(buffer, usize);
	fin.close();
	arg[0] = fd;
	arg[1] = (int)buffer;
	arg[2] = usize;
	sysCall->Write();

	if (checkError())
		return;
	cout << "SUC: �ɹ�д��" << ar0[User::EAX] << "�ֽ�" << endl;
	delete[] buffer;
}
//���ļ�
void User::u_Read(string sfd, string outFile, string size)
{
	if (!isDegital(sfd)) {
		cout << "ERR: ��������ȷ���ļ������fd�ţ���" << endl;
		return;
	}
	if (!isDegital(size)) {
		cout << "ERR: ��������ȷ�Ķ�ȡ�ֽ���<N>��" << endl;
		return;
	}
	int fd = stoi(sfd);
	int usize = stoi(size);
	char* buffer = new char[usize];
	arg[0] = fd;
	arg[1] = (int)buffer;
	arg[2] = usize;
	sysCall->Read();
	if (checkError())
		return;

	cout << "SUC: �ɹ�����" << ar0[User::EAX] << "�ֽ�" << endl;
	if (outFile == "") {
		for (unsigned int i = 0; i < ar0[User::EAX]; ++i)
			cout << (char)buffer[i];
		cout << endl;
		delete[] buffer;
		return;
	}
	else {
		fstream fout(outFile, ios::out | ios::binary);
		if (!fout) {
			cout << "ERR: ���ļ�" << outFile << "ʧ��" << endl;
			return;
		}
		fout.write(buffer, ar0[User::EAX]);
		fout.close();
		delete[] buffer;
		return;
	}
}

//�������ļ����ļ���
void User::u_Rename(string ori, string cur)
{
	string curDir = curDirPath;
	if (!checkPathName(ori) || !checkPathName(cur))
		return;
	
	if (ori.find('/') != string::npos)// ����������Ǿ���·����ʽ
	{
		string nextDir = ori.substr(0,ori.find_last_of('/'));
		u_Cd(nextDir);// ȥ���������ļ��ĸ�Ŀ¼
		ori = ori.substr(ori.find_last_of('/')+1);
		if (cur.find('/') != string::npos)
			cur = cur.substr(cur.find_last_of('/')+1);
	}
	sysCall->Rename(ori,cur);
	u_Cd(curDir);

	if (checkError())
		return;
	cout << "SUC: �ļ����޸ĳɹ���" << endl;
}
//��ӡ��״Ŀ¼
void User::u_Tree(string path)
{
	if (curDirPath.length() > 1 && curDirPath.back() == '/')
		curDirPath.pop_back();
	string curDir = curDirPath;

	if (path == "")
		path = curDir;

	if (!checkPathName(path))
		return;

	path = dirp;
	__userCd__(path);
	if (errorCode != User::U_NOERROR) {//���ʵ��ļ�
		cout << "ERR: Ŀ¼·�������ڣ�" << endl;
		errorCode = User::U_NOERROR;
		__userCd__(curDir);
		return;
	}
	cout << "|---" << (path == "/" ? "/" : path.substr(path.find_last_of('/') + 1)) << endl;
	__userTree__(path, 0);
	__userCd__(curDir);

}


bool User::checkError()
{
	if (errorCode != U_NOERROR) {
		switch (errorCode) {
			case User::U_ENOENT:
				cout << "ERR: �Ҳ����ļ������ļ���!" << endl;
				break;
			case User::U_EBADF:
				cout << "ERR: �Ҳ����ļ����!" << endl;
				break;
			case User::U_EACCES:
				cout << "ERR: Ȩ�޲���!" << endl;
				break;
			case User::U_ENOTDIR:
				cout << "ERR: �ļ��в�����!" << endl;
				break;
			case User::U_ENFILE:
				cout << "ERR: �ļ������!" << endl;
				break;
			case User::U_EMFILE:
				cout << "ERR: ���ļ�����!" << endl;
				break;
			case User::U_EFBIG:
				cout << "ERR: �ļ�����!" << endl;
				break;
			case User::U_ENOSPC:
				cout << "ERR: ���̿ռ䲻��!" << endl;
				break;
			default:
				break;
		}

		errorCode = U_NOERROR;
		return true;
	}
	return false;
}

bool User::checkPathName(string path)
{
	if (path.empty()) {
		cout << "ERR: ����·��Ϊ��" << endl;
		return false;
	}

	if (path[0] == '/' || path.substr(0, 2) != "..")
		dirp = path;            //ϵͳ���ò���(һ������Pathname)��ָ��
	else {
		if (curDirPath.back() != '/')
			curDirPath += '/';
		string pre = curDirPath;//��ǰ����Ŀ¼����·�� cd����Ż�ı�curDirPath��ֵ
		unsigned int p = 0;
		//���Զ������·�� .. ../../
		for (; pre.length() > 3 && p < path.length() && path.substr(p, 2) == ".."; ) {
			pre.pop_back();
			pre.erase(pre.find_last_of('/') + 1);
			p += 2;
			p += p < path.length() && path[p] == '/';
		}
		dirp = pre + path.substr(p);
	}

	if (dirp.length() > 1 && dirp.back() == '/')
		dirp.pop_back();

	for (unsigned int p = 0, q = 0; p < dirp.length(); p = q + 1) {
		q = dirp.find('/', p);
		q = min(q, (unsigned int)dirp.length());
		if (q - p > DirectoryEntry::DIRSIZ) {
			cout << "ERR: �ļ������ļ���������" << endl;
			return false;
		}
	}
	return true;
}

void User::__userCd__(string dirName)
{
	if (!checkPathName(dirName))
		return;
	sysCall->ChDir();
}
//�ڲ���ӡ��״Ŀ¼
void User::__userTree__(string path, int depth)
{
	vector<string> dirs;
	string nextDir;
	ls.clear();
	sysCall->Ls();
	for (int i = 0, p = 0; i < ls.length(); ) {
		p = ls.find('\n', i);
		dirs.emplace_back(ls.substr(i, p - i));
		i = p + 1;
	}
	for (int i = 0; i < dirs.size(); i++) {
		nextDir = (path == "/" ? "" : path) + '/' + dirs[i];
		for (int i = 0; i < depth + 1; i++)
			cout << "|   ";
		cout << "|---" << dirs[i] << endl;
		__userCd__(nextDir);
		if (errorCode != User::U_NOERROR) {//���ʵ��ļ�
			errorCode = User::U_NOERROR;
			continue;
		}
		__userTree__(nextDir, depth + 1);
	}
	__userCd__(path);
	return;
}
