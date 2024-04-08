#include<iostream>
#include<string>
#include<sstream>
#include<iomanip>

#include"DiskDriver.h"
#include"FileSystem.h"
#include"OpenFileManager.h"
#include"User.h"

using namespace std;

//ȫ�ֱ���
DiskDriver g_diskDriver;
BufferManager g_bufferManager; //�ö����������ķ���ͻ��յ�
FileSystem g_fileSystem;//�ö���������ļ�ϵͳ�洢��Դ
InodeTable g_inodeTable;//�ö�������ڴ������inode
OpenFileTable g_openFileTable;//�ö�������ļ�����Ĺ���
SuperBlock g_superBlock;//ȫ��superblock������̵�inode�������̿飬���ڴ���
// ps g_user��غ���g_sysCall������
SysCall g_sysCall;// ȫ��ϵͳ���ö���
User g_user;//ȫ���û�����Ϊ�û��ṩ�ļ�ϵͳ��װ�õĽӿ�

//ui��صľֲ���������
void showTitle();
void showHelp();
void showDetail(string cmd);
bool AutoTest();


int main() {
	showTitle();
	showHelp();
	
	string option, str, arg0,arg1,arg2;

	while (1)
	{
		arg0 = arg1 = arg2 = "";
		cout << "mcwDisk: " << g_user.curDirPath << "> ";

		getline(cin, str);
		if (0 == str.size())
			continue;
		stringstream userin(str);//�洢�����ַ���

		userin >> option>>arg0>>arg1>>arg2;
		
		if ("fformat" == option) 
		{//��ʽ��
			g_openFileTable.Reset();
			g_inodeTable.Reset();
			g_bufferManager.FormatBuffer();
			g_fileSystem.FormatDevice();
			cout << ">> ������ļ�ϵͳ��ʽ����������ֹ...�ڴ������ٴ�ʹ�ã�" << endl;
			return 0;
		}
		else if ("ls" == option) 
		{//�鿴��ǰĿ¼
			g_user.u_Ls();
		}
		else if ("mkdir" == option)
		{//�½�Ŀ¼
			if (arg0[0] != '/')// �������㿪ʼ�ľ���·��
				arg0 = g_user.curDirPath + arg0;
			g_user.u_MkDir(arg0);
		}
		else if ("fcreat" == option)
		{//�����ļ�
			if(arg0[0]!='/')
				arg0 = g_user.curDirPath + arg0;
			g_user.u_Create(arg0);
		}
		else if ("fopen" == option) 
		{//���ļ�
			if (arg0[0] != '/')
				arg0 = g_user.curDirPath + arg0;
			g_user.u_Open(arg0);
		}
		else if ("fclose" == option) 
		{// �ر��ļ�
			g_user.u_Close(arg0);
		}
		else if ("fread" == option) 
		{// ��ȡ�ļ� arg0:fd arg1��N arg2:dst
			g_user.u_Read(arg0,arg2,arg1);
		}
		else if ("fwrite" == option) 
		{// д���ļ� arg0:src arg1:N arg2:fd
			g_user.u_Write(arg2,arg0,arg1);
		}
		else if ("flseek" == option) 
		{// ��λ�ļ�
			if (arg2 == "beg")
				g_user.u_Seek(arg0, arg1, string("0"));
			else if (arg2 == "cur")
				g_user.u_Seek(arg0, arg1, string("1"));
			else if (arg2 == "end")
				g_user.u_Seek(arg0, arg1, string("2"));
			else
				cout << "ERR: ��������help flseek�鿴ʹ�÷���~" << endl;
		}
		else if ("fdelete" == option) 
		{// ɾ���ļ�
			if (arg0[0] != '/')
				arg0 = g_user.curDirPath + arg0;
			g_user.u_Delete(arg0);
		}
		else if ("autotest" == option) 
		{// �Զ�����
			AutoTest();
		}
		//else if ("frename" == option)
		//{// ������
		//	g_user.u_Rename(arg0,arg1);
		//}
		else if ("shtree" == option)
		{
			g_user.u_Tree(arg0);
		}
		else if ("help" == option) {
			if (arg0 == "" || arg0 == "?")
				showHelp();
			else 
				showDetail(arg0);
		}
		else if ("exit" == option) 
		{// �˳�
			return 0;// buffermanager����ʱ��ɻ���д��
		}
		else if ("cd" == option) 
		{// ��ת
			g_user.u_Cd(arg0);
		}
		else {
			cout << "ERR:δʶ���ָ�����help�鿴�����ֲ�" << endl;
		}
	}

	return 0;
}


void showTitle() {
	cout << "|===============================================================================================|" << endl;
	cout << "|                                                                                               |" << endl;
	cout << "|                                      ��Unix�����ļ�ϵͳ                                       |" << endl;
	cout << "|                                      BY  2053459-ë���                                       |" << endl;
	cout << "|                                                                                               |" << endl;
	cout << "|===============================================================================================|" << endl << endl;
}																		 
void showHelp() {														 
	cout << "|" << std::left << setw(35) << "[ָ��]" << std::left << setw(60) << "[˵��]" << "|" << endl
		<< "|" << std::left << setw(35) << "��help <command>" << setw(60) << "��ָ��˵����<command>���ѡ����ʾ����ĳһָ��" << "|" << endl
		<< "|" << std::left << setw(35) << "��autotest" << std::left <<setw(60) << "���Զ������ļ�ϵͳ��ָ��" << "|" << endl
		<< "|" << std::left << setw(35) << "��fformat" << std::left << setw(60) << "����ʽ���ļ���" << "|" << endl
		<< "|" << std::left << setw(35) << "��ls" << std::left << setw(60) << "���г���ǰĿ¼�µ������ļ�" << "|" << endl
		<< "|" << std::left << setw(35) << "��mkdir <dirname>" << std::left << setw(60) << "��������Ŀ¼" << "|" << endl
		<< "|" << std::left << setw(35) << "��fcreat <filename>" << std::left << setw(60) << "���ڵ�ǰ�ļ�·���´������ļ�" << "|" << endl
		<< "|" << std::left << setw(35) << "��fopen <file_name>" << std::left << setw(60) << "����ĳ�ļ������ظ��ļ��ľ��ָ��fd" << "|" << endl
		<< "|" << std::left << setw(35) << "��fclose <fd>" << std::left << setw(60) << "���رվ��<fd>ָ����ļ�" << "|" << endl
		<< "|" << std::left << setw(35) << "��fread <fd_of_src> <N> <dst_file>" << std::left << setw(60) << "����ȡ���<fd_of_src>��ָ�ļ���<N>�ֽڵ�Ŀ���ļ�<dst_file>" << "|" << endl
		<< "|" << std::left << setw(35) << "  " << std::left << setw(60) << "  ���У�<dst_file>ȱʡ�������cmd����" << "|" << endl
		<<"|"<< std::left << setw(35) << "��fwrite <src_file> <N> <fd_of_dst>" << std::left << setw(60) << "����<src_file>д<N>�ֽڵ����<fd_of_dst>��ָ�ļ�" << "|" << endl
		<<"|"<< std::left << setw(35) << "��flseek <fd> <offset> <mode>" << std::left << setw(60) << "����λ�ļ���дָ�룬<mode>ȡbeg/cur/end���ֱ��ʾ�Ӿ��<fd>" << "|" << endl
		<<"|"<< std::left << setw(35) << " " << std::left << setw(60) << "  ��ָ�ļ��ġ���ʼ����ǰ������λ�á�ƫ��<offset>�ֽ�" << "|" << endl
		<<"|"<< std::left << setw(35) << "��fdelete <fd>" << std::left << setw(60) << "��ɾ�����<fd>��ָ�ļ�" << "|" << endl
		<<"|"<< std::left << setw(35) << "��cd <route_to_dir>" << std::left << setw(60) << "��ͨ����Ի����·������ĳĿ¼" << "|" << endl
		//<<"|"<< std::left << setw(35) << "��frename <old_name> <new_name>" << std::left << setw(60) << "���޸ĵ�ǰĿ¼��<old_name>�ļ���Ϊ<new_name>" << "|" << endl
		<<"|"<< std::left << setw(35) << "��shtree <dirname>" << std::left << setw(60) << "���г�<dirname>�µ��ļ����ṹ" << "|" << endl
		<<"|"<< std::left << setw(35) << "��exit" << std::left << setw(60) << "���˳�ϵͳ���ӳ�д�Ļ����ʱд��" << "|" << endl <<endl
		<< " [TIPS 1] ��ϵͳ֧�־���·�������·��������·���Ӹ�Ŀ¼��/����ʼ���确/user/work.txt��,���·��\n ֻ֧�ִӵ�ǰ�ļ�·���¿�ʼ���ݲ�֧��ͨ����../���ص���һĿ¼" << endl
		<<" [TIPS 2] ��ϵͳ����ָ���������Сд����"<<endl
		<<" [TIPS 3] ��ϵͳ�Ĵ󲿷��ļ�������ָ��ʹ���ļ����<fd>�������ļ�������ע��ָ���ʽ~"<<endl
		<< " [TIPS 4] �˳�ʱ�벻Ҫֱ�ӹر�cmd���ڣ�����exit�ٹرգ����򻺴�δ��д�ؽ����±���д������ݶ�ʧ��" << endl;
	cout << "|===============================================================================================|" << endl << endl;
}

bool AutoTest() {
	cout << "|============================================�Զ����Գ���======================================|" << endl << endl;
	cout << "|<1>mkdir���ԣ�\n";
	cout << "mcwDisk: "<< g_user.curDirPath << "> " << "mkdir /bin" << endl;
	g_user.u_MkDir("/bin");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /etc" << endl;
	g_user.u_MkDir("/etc");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /home" << endl;
	g_user.u_MkDir("/home");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /dev" << endl;
	g_user.u_MkDir("/dev");

	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /home/texts" << endl;
	g_user.u_MkDir("/home/texts");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /home/reports" << endl;
	g_user.u_MkDir("/home/reports");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /home/photos" << endl;
	g_user.u_MkDir("/home/photos");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "shtree /" << endl;
	g_user.u_Tree("/");

	cout << "\n<2>ls��cd���ԣ�\n";
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "ls" << endl;
	g_user.u_Ls();
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "cd /home" << endl;
	g_user.u_Cd("/home");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "ls" << endl;
	g_user.u_Ls();

	cout << "\n|<3>fcreat��fopen��fwrite���ԣ�\n";
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "cd /home/texts" << endl;
	g_user.u_Cd("/home/texts");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fcreat ReadMe.txt" << endl;
	g_user.u_Create("ReadMe.txt");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fopen ReadMe.txt" << endl;
	g_user.u_Open("ReadMe.txt");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fwrite ReadMe.txt 2069 8" << endl;
	g_user.u_Write("8","ReadMe.txt","2069");

	cout << "mcwDisk: " << g_user.curDirPath << "> " << "cd /home/reports" << endl;
	g_user.u_Cd("/home/reports");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fcreat Report.pdf" << endl;
	g_user.u_Create("Report.pdf");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fopen Report.pdf" << endl;
	g_user.u_Open("Report.pdf");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fwrite ReportDemo.pdf 206834 10" << endl;
	g_user.u_Write("10", "ReportDemo.pdf", "206834");

	cout << "mcwDisk: " << g_user.curDirPath << "> " << "cd /home/photos" << endl;
	g_user.u_Cd("/home/photos");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fcreat myPic.png" << endl;
	g_user.u_Create("myPic.png");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fopen myPic.png" << endl;
	g_user.u_Open("myPic.png");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fwrite picture.png 604047 10" << endl;
	g_user.u_Write("12", "picture.png", "604047");

	cout << "mcwDisk: " << g_user.curDirPath << "> " << "shtree /" << endl;
	g_user.u_Tree("/");


	cout << "\n|<4>�ļ���д���ָ����ԣ�\n";
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "mkdir /test" << endl;
	g_user.u_MkDir("/test");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "cd /test" << endl;
	g_user.u_Cd("/test");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fcreat Jerry" << endl;
	g_user.u_Create("Jerry");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "shtree /" << endl;
	g_user.u_Tree("/");

	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fopen Jerry" << endl;
	g_user.u_Open("Jerry");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fwrite usercin.txt 800 15" << endl;
	g_user.u_Write("15","usercin.txt","800");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "flseek 15 500 beg" << endl;
	g_user.u_Seek("15","500","0");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fread 15 500 abc.txt" << endl;
	g_user.u_Read("15", "abc.txt", "500");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "flseek 15 500 beg" << endl;
	g_user.u_Seek("15", "500", "0");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fread 15 500" << endl;
	g_user.u_Read("15", "", "500");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "fclose 15" << endl;
	g_user.u_Close("15");

	cout << "\n�Զ����Խ���" << endl << endl;
	g_user.u_Cd("/");
	return true;
}
void showDetail(string cmd) {
	if ("autotest" == cmd) {
		cout << "[ָ��]autotest\n";
		cout << "[˵��]�Զ�����ָ�ͨ����ָ���Զ����Ը��ļ�ָ�\n";
	}
	else if ("fformat" == cmd) {
		cout << "[ָ��]fformat\n";
		cout << "[˵��]��ʽ���ļ���ͨ�����������õ�һ��ո�µĴ��̡�\n";
	}
	else if ("ls" == cmd) {
		cout << "[ָ��]ls\n";
		cout << "[˵��]�г���ǰĿ¼�µ������ļ���"<<endl;
	}
	else if ("mkdir" == cmd) {
		cout << "[ָ��]mkdir <dirname>\n";
		cout << "[˵��]������Ŀ¼��<dirname>�ɲ��þ���·��������λ�ô�������ֱ�����������ڵ�ǰĿ¼���½���" 
			<< endl<<"[����]��mkdir userfile\t��mkdir /userfile\t��mkdir /root/ABC/EFG/userfile"<<endl;
	}
	else if ("fcreat" == cmd) {
		cout << "[ָ��]fcreat <filename>\n";
		cout << "[˵��]�ڵ�ǰ�ļ�·���´������ļ�"
			<< endl << "[����]��fcreat mydiary.txt" << endl;
	}
	else if ("fopen" == cmd) {
		cout << "[ָ��]fopen <file_name>\n";
		cout << "[˵��]��ĳ�ļ������ظ��ļ��ľ��ָ��fd��֧����Ի����·����"
			<< endl << "[����]��fopen mydiary.txt\t��fopen /root/ABC/EFG/mydiary.txt" << endl;
	}
	else if ("fclose" == cmd) {
		cout << "[ָ��]fclose <fd>\n";
		cout << "[˵��]�ر�ĳ�ļ���Ҫ���ļ��Ѿ������Ҵ�"
			<< endl << "[����]��fclose 10 �ر�fd=10ָ����ļ�" << endl;
	}
	else if ("fread" == cmd) {
		cout << "[ָ��]fread <fd_of_src> <N> <dst_file>\n";
		cout << "[˵��]��ȡ���<fd_of_src>��ָ�ļ���<N>�ֽڵ�Ŀ���ļ�<dst_file>,���У�<dst_file>ȱʡ�������cmd����"
			<< endl << "[����]��fread 15 100 mydiary.txt ��ȡ������fd=15���ļ���100���ֽڵ�mydiary.txt��" << endl;
	}
	else if ("fwrite" == cmd) {
		cout << "[ָ��]fwrite <src_file> <N> <fd_of_dst>\n";
		cout << "[˵��]��<src_file>д<N>�ֽڵ����<fd_of_dst>��ָ�ļ�"
			<< endl << "[����]��write mydiary.txt 100 12 ��mydiary.txt��ǰ100�ֽ�д�뵽������fd=15���ļ���" << endl;
	}
	else if ("flseek" == cmd) {
		cout << "[ָ��]flseek <fd> <offset> <mode>\n";
		cout << "[˵��]��λ�ļ���дָ�룬<mode>ȡbeg/cur/end���ֱ��ʾ�Ӿ��<fd>��ָ�ļ��ġ���ʼ����ǰ������λ�á�ƫ��<offset>�ֽ�"
			<< endl << "[����]��flseek 1 0 beg ����fd=1��ָ�ļ��Ķ�дָ�뵽�ļ���ʼ�ֽ�"<<endl
			<<"      ��flseek 1 10 cur ����fd=1��ָ�ļ��Ķ�дָ�����10�ֽ�" << endl
			<< "      ��flseek 1 -10 end ����fd=1��ָ�ļ��Ķ�дָ�뵽�ļ�ĩβ������10�ֽ�" << endl;
	}
	else if ("fdelete" == cmd) {
		cout << "[ָ��]fdelete <fd>\n";
		cout << "[˵��]ɾ�����<fd>��ָ�ļ�"
			<< endl << "[����]fdelete 10" << endl;
	}
	else if ("cd" == cmd) {
		cout << "[ָ��]cd <route_to_dir>\n";
		cout << "[˵��]ͨ����Ի����·������ĳĿ¼"
			<< endl << "[����]��cd / �����Ŀ¼" << endl;
	}
	//else if ("frename" == cmd) {
	//	cout << "[ָ��]frename <old_name> <new_name>\n";
	//	cout << "[˵��]�޸ĵ�ǰĿ¼��<old_name>�ļ���Ϊ<new_name>"
	//		<< endl << "[����]��frename Afile Bfile" << endl;
	//}
	else if ("shtree" == cmd) {
		cout << "[ָ��]shtree <dirname>\n";
		cout << "[˵��]�г�<dirname>�µ��ļ����ṹ"<< endl;
	}
	else if ("exit" == cmd) {
		cout << "[ָ��]exit\n";
		cout << "[˵��]�˳�ϵͳ���ӳ�д�Ļ����ʱд��" << endl;
	}
	else {
		cout << "δʶ����������help�鿴��������~" << endl;
	}
}