#include<iostream>
#include<string>
#include<sstream>
#include<iomanip>

#include"DiskDriver.h"
#include"FileSystem.h"
#include"OpenFileManager.h"
#include"User.h"

using namespace std;

//全局变量
DiskDriver g_diskDriver;
BufferManager g_bufferManager; //该对象负责管理缓存的分配和回收等
FileSystem g_fileSystem;//该对象负责管理文件系统存储资源
InodeTable g_inodeTable;//该对象管理内存的所有inode
OpenFileTable g_openFileTable;//该对象负责打开文件表项的管理
SuperBlock g_superBlock;//全局superblock管理磁盘的inode和数据盘块，在内存中
// ps g_user务必后于g_sysCall声明！
SysCall g_sysCall;// 全局系统调用对象
User g_user;//全局用户对象，为用户提供文件系统封装好的接口

//ui相关的局部函数声明
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
		stringstream userin(str);//存储输入字符串

		userin >> option>>arg0>>arg1>>arg2;
		
		if ("fformat" == option) 
		{//格式化
			g_openFileTable.Reset();
			g_inodeTable.Reset();
			g_bufferManager.FormatBuffer();
			g_fileSystem.FormatDevice();
			cout << ">> 已完成文件系统格式化！程序终止...期待您的再次使用！" << endl;
			return 0;
		}
		else if ("ls" == option) 
		{//查看当前目录
			g_user.u_Ls();
		}
		else if ("mkdir" == option)
		{//新建目录
			if (arg0[0] != '/')// 传入根结点开始的绝对路径
				arg0 = g_user.curDirPath + arg0;
			g_user.u_MkDir(arg0);
		}
		else if ("fcreat" == option)
		{//创建文件
			if(arg0[0]!='/')
				arg0 = g_user.curDirPath + arg0;
			g_user.u_Create(arg0);
		}
		else if ("fopen" == option) 
		{//打开文件
			if (arg0[0] != '/')
				arg0 = g_user.curDirPath + arg0;
			g_user.u_Open(arg0);
		}
		else if ("fclose" == option) 
		{// 关闭文件
			g_user.u_Close(arg0);
		}
		else if ("fread" == option) 
		{// 读取文件 arg0:fd arg1：N arg2:dst
			g_user.u_Read(arg0,arg2,arg1);
		}
		else if ("fwrite" == option) 
		{// 写入文件 arg0:src arg1:N arg2:fd
			g_user.u_Write(arg2,arg0,arg1);
		}
		else if ("flseek" == option) 
		{// 定位文件
			if (arg2 == "beg")
				g_user.u_Seek(arg0, arg1, string("0"));
			else if (arg2 == "cur")
				g_user.u_Seek(arg0, arg1, string("1"));
			else if (arg2 == "end")
				g_user.u_Seek(arg0, arg1, string("2"));
			else
				cout << "ERR: 输入有误，help flseek查看使用方法~" << endl;
		}
		else if ("fdelete" == option) 
		{// 删除文件
			if (arg0[0] != '/')
				arg0 = g_user.curDirPath + arg0;
			g_user.u_Delete(arg0);
		}
		else if ("autotest" == option) 
		{// 自动测试
			AutoTest();
		}
		//else if ("frename" == option)
		//{// 重命名
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
		{// 退出
			return 0;// buffermanager析构时完成缓存写回
		}
		else if ("cd" == option) 
		{// 跳转
			g_user.u_Cd(arg0);
		}
		else {
			cout << "ERR:未识别的指令，输入help查看命令手册" << endl;
		}
	}

	return 0;
}


void showTitle() {
	cout << "|===============================================================================================|" << endl;
	cout << "|                                                                                               |" << endl;
	cout << "|                                      类Unix二级文件系统                                       |" << endl;
	cout << "|                                      BY  2053459-毛朝炜                                       |" << endl;
	cout << "|                                                                                               |" << endl;
	cout << "|===============================================================================================|" << endl << endl;
}																		 
void showHelp() {														 
	cout << "|" << std::left << setw(35) << "[指令]" << std::left << setw(60) << "[说明]" << "|" << endl
		<< "|" << std::left << setw(35) << "·help <command>" << setw(60) << "·指令说明，<command>项可选，表示介绍某一指令" << "|" << endl
		<< "|" << std::left << setw(35) << "·autotest" << std::left <<setw(60) << "·自动测试文件系统各指令" << "|" << endl
		<< "|" << std::left << setw(35) << "·fformat" << std::left << setw(60) << "·格式化文件卷" << "|" << endl
		<< "|" << std::left << setw(35) << "·ls" << std::left << setw(60) << "·列出当前目录下的所有文件" << "|" << endl
		<< "|" << std::left << setw(35) << "·mkdir <dirname>" << std::left << setw(60) << "·创建新目录" << "|" << endl
		<< "|" << std::left << setw(35) << "·fcreat <filename>" << std::left << setw(60) << "·在当前文件路径下创建新文件" << "|" << endl
		<< "|" << std::left << setw(35) << "·fopen <file_name>" << std::left << setw(60) << "·打开某文件，返回该文件的句柄指针fd" << "|" << endl
		<< "|" << std::left << setw(35) << "·fclose <fd>" << std::left << setw(60) << "·关闭句柄<fd>指向的文件" << "|" << endl
		<< "|" << std::left << setw(35) << "·fread <fd_of_src> <N> <dst_file>" << std::left << setw(60) << "·读取句柄<fd_of_src>所指文件的<N>字节到目标文件<dst_file>" << "|" << endl
		<< "|" << std::left << setw(35) << "  " << std::left << setw(60) << "  其中，<dst_file>缺省则输出到cmd窗口" << "|" << endl
		<<"|"<< std::left << setw(35) << "·fwrite <src_file> <N> <fd_of_dst>" << std::left << setw(60) << "·从<src_file>写<N>字节到句柄<fd_of_dst>所指文件" << "|" << endl
		<<"|"<< std::left << setw(35) << "·flseek <fd> <offset> <mode>" << std::left << setw(60) << "·定位文件读写指针，<mode>取beg/cur/end，分别表示从句柄<fd>" << "|" << endl
		<<"|"<< std::left << setw(35) << " " << std::left << setw(60) << "  所指文件的“起始、当前、结束位置”偏移<offset>字节" << "|" << endl
		<<"|"<< std::left << setw(35) << "·fdelete <fd>" << std::left << setw(60) << "·删除句柄<fd>所指文件" << "|" << endl
		<<"|"<< std::left << setw(35) << "·cd <route_to_dir>" << std::left << setw(60) << "·通过相对或绝对路径进入某目录" << "|" << endl
		//<<"|"<< std::left << setw(35) << "·frename <old_name> <new_name>" << std::left << setw(60) << "·修改当前目录下<old_name>文件名为<new_name>" << "|" << endl
		<<"|"<< std::left << setw(35) << "·shtree <dirname>" << std::left << setw(60) << "·列出<dirname>下的文件树结构" << "|" << endl
		<<"|"<< std::left << setw(35) << "·exit" << std::left << setw(60) << "·退出系统，延迟写的缓存此时写回" << "|" << endl <<endl
		<< " [TIPS 1] 本系统支持绝对路径和相对路径，绝对路径从根目录‘/’开始，如‘/user/work.txt’,相对路径\n 只支持从当前文件路径下开始，暂不支持通过‘../’回到上一目录" << endl
		<<" [TIPS 2] 本系统所有指令及参数均大小写敏感"<<endl
		<<" [TIPS 3] 本系统的大部分文件操作的指令使用文件句柄<fd>，而非文件名，请注意指令格式~"<<endl
		<< " [TIPS 4] 退出时请不要直接关闭cmd窗口！输入exit再关闭，否则缓存未能写回将导致本次写入的数据丢失！" << endl;
	cout << "|===============================================================================================|" << endl << endl;
}

bool AutoTest() {
	cout << "|============================================自动测试程序======================================|" << endl << endl;
	cout << "|<1>mkdir测试：\n";
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

	cout << "\n<2>ls、cd测试：\n";
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "ls" << endl;
	g_user.u_Ls();
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "cd /home" << endl;
	g_user.u_Cd("/home");
	cout << "mcwDisk: " << g_user.curDirPath << "> " << "ls" << endl;
	g_user.u_Ls();

	cout << "\n|<3>fcreat、fopen、fwrite测试：\n";
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


	cout << "\n|<4>文件读写相关指令测试：\n";
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

	cout << "\n自动测试结束" << endl << endl;
	g_user.u_Cd("/");
	return true;
}
void showDetail(string cmd) {
	if ("autotest" == cmd) {
		cout << "[指令]autotest\n";
		cout << "[说明]自动测试指令，通过该指令自动测试各文件指令。\n";
	}
	else if ("fformat" == cmd) {
		cout << "[指令]fformat\n";
		cout << "[说明]格式化文件卷，通过此命令您得到一个崭新的磁盘。\n";
	}
	else if ("ls" == cmd) {
		cout << "[指令]ls\n";
		cout << "[说明]列出当前目录下的所有文件。"<<endl;
	}
	else if ("mkdir" == cmd) {
		cout << "[指令]mkdir <dirname>\n";
		cout << "[说明]创建新目录，<dirname>可采用绝对路径在任意位置创建、或直接输入名字在当前目录下新建。" 
			<< endl<<"[用例]·mkdir userfile\t·mkdir /userfile\t·mkdir /root/ABC/EFG/userfile"<<endl;
	}
	else if ("fcreat" == cmd) {
		cout << "[指令]fcreat <filename>\n";
		cout << "[说明]在当前文件路径下创建新文件"
			<< endl << "[用例]·fcreat mydiary.txt" << endl;
	}
	else if ("fopen" == cmd) {
		cout << "[指令]fopen <file_name>\n";
		cout << "[说明]打开某文件，返回该文件的句柄指针fd，支持相对或绝对路径。"
			<< endl << "[用例]·fopen mydiary.txt\t·fopen /root/ABC/EFG/mydiary.txt" << endl;
	}
	else if ("fclose" == cmd) {
		cout << "[指令]fclose <fd>\n";
		cout << "[说明]关闭某文件，要求文件已经存在且打开"
			<< endl << "[用例]·fclose 10 关闭fd=10指向的文件" << endl;
	}
	else if ("fread" == cmd) {
		cout << "[指令]fread <fd_of_src> <N> <dst_file>\n";
		cout << "[说明]读取句柄<fd_of_src>所指文件的<N>字节到目标文件<dst_file>,其中，<dst_file>缺省则输出到cmd窗口"
			<< endl << "[用例]·fread 15 100 mydiary.txt 读取磁盘中fd=15的文件的100个字节到mydiary.txt中" << endl;
	}
	else if ("fwrite" == cmd) {
		cout << "[指令]fwrite <src_file> <N> <fd_of_dst>\n";
		cout << "[说明]从<src_file>写<N>字节到句柄<fd_of_dst>所指文件"
			<< endl << "[用例]·write mydiary.txt 100 12 将mydiary.txt中前100字节写入到磁盘中fd=15的文件中" << endl;
	}
	else if ("flseek" == cmd) {
		cout << "[指令]flseek <fd> <offset> <mode>\n";
		cout << "[说明]定位文件读写指针，<mode>取beg/cur/end，分别表示从句柄<fd>所指文件的“起始、当前、结束位置”偏移<offset>字节"
			<< endl << "[用例]·flseek 1 0 beg 调整fd=1所指文件的读写指针到文件起始字节"<<endl
			<<"      ·flseek 1 10 cur 调整fd=1所指文件的读写指针后移10字节" << endl
			<< "      ·flseek 1 -10 end 调整fd=1所指文件的读写指针到文件末尾倒数第10字节" << endl;
	}
	else if ("fdelete" == cmd) {
		cout << "[指令]fdelete <fd>\n";
		cout << "[说明]删除句柄<fd>所指文件"
			<< endl << "[用例]fdelete 10" << endl;
	}
	else if ("cd" == cmd) {
		cout << "[指令]cd <route_to_dir>\n";
		cout << "[说明]通过相对或绝对路径进入某目录"
			<< endl << "[用例]·cd / 进入根目录" << endl;
	}
	//else if ("frename" == cmd) {
	//	cout << "[指令]frename <old_name> <new_name>\n";
	//	cout << "[说明]修改当前目录下<old_name>文件名为<new_name>"
	//		<< endl << "[用例]·frename Afile Bfile" << endl;
	//}
	else if ("shtree" == cmd) {
		cout << "[指令]shtree <dirname>\n";
		cout << "[说明]列出<dirname>下的文件树结构"<< endl;
	}
	else if ("exit" == cmd) {
		cout << "[指令]exit\n";
		cout << "[说明]退出系统，延迟写的缓存此时写回" << endl;
	}
	else {
		cout << "未识别的命令，输入help查看所有命令~" << endl;
	}
}