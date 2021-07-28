#include <iostream>
#include "agvc_ctrl_main.h"
//#include <Windows.h>
#include "data_access.h"

using namespace std;


//Cagvc_ctrl_mgr agvc;

//extern "C" void do_nothing(int nn)
//{
//}
//
//extern "C" void kill_signal_process(int aa)
//{
//    return;
//}
//extern "C" void catch_interrupt_sig( int sig_no )
//{
//    if( sig_no == SIGUSR1 )
//    {
//        agvc.dnet_obj->write_log_at_once(0,0,"收到中断线程运行的信号");
//
//    }
//    else if( sig_no == SIGPIPE )
//    {
//        agvc.dnet_obj->write_log_at_once(0,0,"收到socket中断信号");
//    }
//}



int main(int argc, char *argv[])
{

	//sigset(SIGHUP,do_nothing);				//挂起中断
	//sigset(SIGALRM,do_nothing);				//时钟告警中断
	//signal(SIGTERM, kill_signal_process);	//接收到KILL信号

	////以下为了捕获unix的SIGPIPE消息
	////该程序段一定要加,否则unix收到SIGPIPE消息,默认动作为程序退出
	//struct sigaction new_action, old_action;
	//sigemptyset( &new_action.sa_mask );
	//new_action.sa_flags = 0;
	//new_action.sa_handler = catch_interrupt_sig;
	//sigaction( SIGUSR1, NULL, &old_action );
	//if( old_action.sa_handler != SIG_IGN )
	//    sigaction( SIGUSR1, &new_action, NULL );

	//sigemptyset( &new_action.sa_mask );
	//new_action.sa_flags = 0;
	//new_action.sa_handler = catch_interrupt_sig;
	//sigaction( SIGPIPE, NULL, &old_action );
	//if( old_action.sa_handler != SIG_IGN )
	//    sigaction( SIGPIPE, &new_action, NULL );

	//int		retcode;
	//pid_t	pid_other;
	//retcode = get_process_num_with_same_name(
	//            argc,			//本进程参数个数
	//            argv,			//本进程运行参数
	//            pid_other);		//其中一个同名进程的进程号
	//if (retcode > 1)
	//{
	//    printf("有:%d 个同名进程:%s，本进程退出\n",
	//          retcode-1,
	//          argv[0]);
	//    exit(0);
	//}
	////以上代码是在UNIX环境下判断进程是否被重复启动
	//agvc.main_loop();

	//cout << "Hello World!" << endl;
	//return 0;

	//	int		retcode;
	//	pid_t	pid_other;


	//retcode = get_process_num_with_same_name(
	//            argc,			//本进程参数个数
	//            argv,			//本进程运行参数
	//            pid_other);		//其中一个同名进程的进程号
	//if (retcode > 1)
	//{
	//    printf("有:%d 个同名进程:%s，本进程退出\n",
	//          retcode-1,
	//          argv[0]);
	//    exit(0);
	//}
	//以上代码是在UNIX环境下判断进程是否被重复启动


	Cagvc_ctrl_mgr agvc;

	agvc.main_loop();

	//cout << "Hello World!" << endl;
	//agvc.dnet_obj->write_log(0,5199,"helloworld");

	//OutputDebugString("sadsadadsadsad");
	//system("pause");

	return 0;
}



