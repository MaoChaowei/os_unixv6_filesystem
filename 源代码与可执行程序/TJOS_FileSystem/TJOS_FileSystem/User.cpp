# include"User.h"
#include<vector>
#include<fstream>

extern SysCall g_sysCall;

// 用于传参检查，检查str是否为数字
bool isDegital(string str) {
	for (int i = 0; i < str.size(); i++) {
		if (str.at(i) == '-' && str.size() > 1)  // 有可能出现负数
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
	curDirIP = sysCall->rootDirInode;// 指向当前根目录的inode
	parDirIP = NULL;
	memset(this->arg,0,sizeof(this->arg));
}
User::~User()
{
	// nothing much
}
//列目录
void User::u_Ls()
{
	ls.clear();
	sysCall->Ls();
	if (checkError())
		return;
	cout << ls << endl;
}
//移动到
void User::u_Cd(string dirName)
{
	if (!checkPathName(dirName))
		return;
	sysCall->ChDir();
	checkError();
}
//创建目录
void User::u_MkDir(string dirName)
{
	if (!checkPathName(dirName))
		return;
	arg[1] = Inode::IFDIR;
	sysCall->Creat();
	if (checkError())
		return;
	cout << "SUC: 目录创建成功" <<endl;
}
//创建文件
void User::u_Create(string fileName)
{
	if (!checkPathName(fileName))
		return;
	arg[1] = (Inode::IREAD | Inode::IWRITE);//存放当前系统调用参数
	sysCall->Creat();
	if (checkError())
		return;
	cout << "SUC: 文件" << fileName << "创建成功!"<<endl;
}
//删除文件
void User::u_Delete(string fileName)
{
	if (!checkPathName(fileName))
		return;
	sysCall->UnLink();
	if (checkError())
		return;
	cout << "SUC: 文件" << fileName << "删除成功"<<endl;
}
//打开文件
void User::u_Open(string fileName)
{
	if (!checkPathName(fileName))
		return;
	arg[1] = (File::FREAD | File::FWRITE);//存放当前系统调用参数
	sysCall->Open();
	if (checkError())
		return;
	cout << "SUC: 文件" << fileName << "打开成功，其文件句柄fd为 " << ar0[User::EAX] << endl;
}
//关闭文件
void User::u_Close(string fd)
{
	if (!isDegital(fd)) {
		cout << "ERR: 请输入正确的文件句柄（fd号）！" << endl;
		return;
	}
	arg[0] = stoi(fd);//存放当前系统调用参数
	sysCall->Close();
	if (checkError())
		return;
	cout << "SUC: 文件关闭成功"<< endl;
}
//定位文件读写执政
void User::u_Seek(string fd, string offset, string origin)
{
	if (!isDegital(fd)) {
		cout << "ERR: 请输入正确的文件句柄（fd号）！" << endl;
		return;
	}
	if (!isDegital(offset)) {
		cout << "ERR: 请输入正确的偏移值<offset>！" << endl;
		return;
	}
	if (!isDegital(origin)) {
		cout << "ERR: 请输入正确的<mode>！" << endl;
		return;
	}
	arg[0] = stoi(fd);
	arg[1] = stoi(offset);
	arg[2] = stoi(origin);
	sysCall->Seek();
	checkError();
}
//写文件
void User::u_Write(string sfd, string inFile, string size)
{
	if (!isDegital(sfd)) {
		cout << "ERR: 请输入正确的文件句柄（fd号）！" << endl;
		return;
	}
	if (!isDegital(size)) {
		cout << "ERR: 请输入正确的写入字节数<N>！" << endl;
		return;
	}
	int fd = stoi(sfd), usize = 0;
	if (size.empty() || (usize = stoi(size)) < 0) {
		cout << "ERR: 参数<N>错误！必须大于等于零 \n";
		return;
	}
	char* buffer = new char[usize];
	fstream fin(inFile, ios::in | ios::binary);
	if (!fin.is_open()) {
		cout << "ERR: 文件" << inFile << "打开失败" << endl;
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
	cout << "SUC: 成功写入" << ar0[User::EAX] << "字节" << endl;
	delete[] buffer;
}
//读文件
void User::u_Read(string sfd, string outFile, string size)
{
	if (!isDegital(sfd)) {
		cout << "ERR: 请输入正确的文件句柄（fd号）！" << endl;
		return;
	}
	if (!isDegital(size)) {
		cout << "ERR: 请输入正确的读取字节数<N>！" << endl;
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

	cout << "SUC: 成功读出" << ar0[User::EAX] << "字节" << endl;
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
			cout << "ERR: 打开文件" << outFile << "失败" << endl;
			return;
		}
		fout.write(buffer, ar0[User::EAX]);
		fout.close();
		delete[] buffer;
		return;
	}
}

//重命名文件、文件夹
void User::u_Rename(string ori, string cur)
{
	string curDir = curDirPath;
	if (!checkPathName(ori) || !checkPathName(cur))
		return;
	
	if (ori.find('/') != string::npos)// 如果给出的是绝对路径形式
	{
		string nextDir = ori.substr(0,ori.find_last_of('/'));
		u_Cd(nextDir);// 去该重命名文件的父目录
		ori = ori.substr(ori.find_last_of('/')+1);
		if (cur.find('/') != string::npos)
			cur = cur.substr(cur.find_last_of('/')+1);
	}
	sysCall->Rename(ori,cur);
	u_Cd(curDir);

	if (checkError())
		return;
	cout << "SUC: 文件名修改成功！" << endl;
}
//打印树状目录
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
	if (errorCode != User::U_NOERROR) {//访问到文件
		cout << "ERR: 目录路径不存在！" << endl;
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
				cout << "ERR: 找不到文件或者文件夹!" << endl;
				break;
			case User::U_EBADF:
				cout << "ERR: 找不到文件句柄!" << endl;
				break;
			case User::U_EACCES:
				cout << "ERR: 权限不足!" << endl;
				break;
			case User::U_ENOTDIR:
				cout << "ERR: 文件夹不存在!" << endl;
				break;
			case User::U_ENFILE:
				cout << "ERR: 文件表溢出!" << endl;
				break;
			case User::U_EMFILE:
				cout << "ERR: 打开文件过多!" << endl;
				break;
			case User::U_EFBIG:
				cout << "ERR: 文件过大!" << endl;
				break;
			case User::U_ENOSPC:
				cout << "ERR: 磁盘空间不足!" << endl;
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
		cout << "ERR: 参数路径为空" << endl;
		return false;
	}

	if (path[0] == '/' || path.substr(0, 2) != "..")
		dirp = path;            //系统调用参数(一般用于Pathname)的指针
	else {
		if (curDirPath.back() != '/')
			curDirPath += '/';
		string pre = curDirPath;//当前工作目录完整路径 cd命令才会改变curDirPath的值
		unsigned int p = 0;
		//可以多重相对路径 .. ../../
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
			cout << "ERR: 文件名或文件夹名过长" << endl;
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
//内部打印树状目录
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
		if (errorCode != User::U_NOERROR) {//访问到文件
			errorCode = User::U_NOERROR;
			continue;
		}
		__userTree__(nextDir, depth + 1);
	}
	__userCd__(path);
	return;
}
