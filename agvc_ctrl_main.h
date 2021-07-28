#ifndef AGVC_CTRL_MAIN_H
#define AGVC_CTRL_MAIN_H
#include "const_def.h"
#include "device.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
class Cagvc_ctrl_mgr
{
public:
	Cagvc_ctrl_mgr();
	~Cagvc_ctrl_mgr();
	system_net_info dnet_instance;
	RDB_NET rdb_instance;
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
	scada_report_manager *scada_report;
public:
	//设备对象
	vector<Cfac_info *> fac_list;
	vector<Cmeas_info *> meas_list;
	vector<Cbms_info *> bms_list;
	vector<Cpcs_info *> pcs_list;
	vector<Cstation_info *> station_list;
	vector<Cpoint_info *> point_list;
	vector<Cagvc_info *> agvc_list;
	vector<Cmicro_ctrl_info *> micro_ctrl_list;


	vector<Cgatepower_info * > gatepower_list;//关口电量表
	vector<Cstation_monthpower_info * > gatepower_list;//整站月电量分析表
	vector<Cstation_daypower_info * > gatepower_list;//整站日电量分析表

	int read_fac_info_table();
	int read_meas_info_table();
	int read_bms_info_table();
	int read_pcs_info_table();
	int read_station_info_table();
	int read_point_info_table();
	int read_agvc_info_table();
	int read_micro_ctrl_info_table();

    //202107新增 储能评价功能用表
	int red_gatepower_info_table();  //读取关口电量表
	int red_station_monthpower_info_table();  //整站月电量分析表
	int red_station_daypower_info_table();  //整站日电量分析表




	Cfac_info* find_fac_from_list(int fac_id);
	Cmeas_info* find_meas_from_list(int display_id);
	Cbms_info* find_bms_from_list(int display_id);
	Cpcs_info* find_pcs_from_list(int display_id);
	Cstation_info* find_station_from_list(int display_id);
	Cpoint_info* find_point_from_list(int display_id);
	Cagvc_info* find_agvc_from_list(int display_id);
	Cmicro_ctrl_info* find_micro_ctrl_from_list(int display_id);
	Cgatepower_info* find_gatepower_from_list(int display_id);   //查找某一条关口表电量记录
	Cgatepower_info* find_station_monthpower_from_list(int display_id);   //查找某一条整站月电量记录
	Cgatepower_info* find_station_daypower_from_list(int display_id);   //查找某一条整站日电量电量记录


public:
	//遥控遥调转发业务逻辑
	vector<yk_send_struct> yk_send_list;    //读取遥控转发表记录
	vector<yt_send_struct> yt_send_list;    //读取调节量转发表记录
	vector<yk_define_struct> yk_define_list;    //遥控定义表记录
	vector<sw_info_struct> sw_list; //开关表记录
	int read_yk_send_table();
	int read_yt_send_table();
	int read_yk_define_table();
	int read_sw_info_table();
	int find_yk_send_record(int fac_id, short send_no, yk_send_struct &yk_send);
	int find_yt_send_record(int fac_id, short send_no, yt_send_struct &yt_send);
	int find_yk_define_record(int sw_id, yk_define_struct &yk_define);
	int find_sw_info_record(int sw_id, sw_info_struct &sw_info);
	ctrl_cmd_struct *find_ctrl_cmd_record(int fac_id, short send_no);
	char *dnet_recv_buf;
	int recv_and_proc_dnet_report();
	ctrl_cmd_struct cmd_define;
	int unpacket_ctrl_cmd_dnet_report(app_buf_head dnet_app_head);
	int unpacket_yuan_wang_he_dnet_report(app_buf_head dnet_app_head);
	int packet_ctrl_cmd_dnet_report_ack(app_buf_head dnet_app_head, ctrl_cmd_struct cmd_define, int return_stat);
	int send_msg_to_scada_normal(ctrl_cmd_struct cmd, char derection);
public:
	void agvc_ctrl_init();
	void device_list_clear();
	void device_rdb_read();   //更新设备实时值
	//电站agvc相关功能
	yk_send_struct yk_send;
	yt_send_struct yt_send;
	void distribute_power(float p_ess);  //功率分配
	void ess_dispatcher_interact_process(int agvc_type); //储能电站与调度交互过程处理
	//void ess_station_calc();    //储能电站反馈调度计算相关
	void agc_ctrl_process();   //执行调度agvc应用功能
	void avc_ctrl_process();//发送无功调节
	void set_all_pcs_standby(); //设置全站PCS待机
	void set_all_pcs_stop();    //设置全站PCS停机
	//电站提供调度转发功能
	void forward_cmd_process(); //执行调度转发应用功能
	int yuanwanghe_ctrl();
	// void station_quick_test_ctrl();//用于电站验收测试系列功能
	int	get_duty_status();
	void pcs_alarm_level_ctrl(); //处理电站人工下发功率后的一级/二级三级故障逻辑 仅在AGC不允许控制,且电站为就地状态,且无就地运行策略情况下判定,否则会影响策略的功率分配逻辑
	void agvc_link_stat_check();//agvc通道状态监测/调度下发调节值超时监测
	////////////////////////////////////////////////////////////////以上和微网项目无关//////////////////////////////////////////////////////
	//卢俊峰 201903月整理
	//基础控制函数 遥控 遥调设置
	int set_on_ankerui(int a); //设置安科瑞表合闸 a变流器表中显示ID
	int set_off_ankerui(int a);
	int set_on_sifier(int a);   //设置斯菲尔表合闸 a变流器表中显示ID
	int set_off_sifier(int a);
	int set_on_sun(int a);  //控制光伏开机
	int set_off_sun(int a); //控制光伏关机
	//冷库设置
	int  lk_on( int a);
	int  lk_off(int a);
	int  lk_lowertemp(int a ,int b);  //考虑下如何判状态
	int  lk_uppertemp(int a ,int b); //考虑下如何判状态
	//485继电器
	//卷膜
	int roll1_on();
	int roll1_off();
	int roll2_on();
	int roll2_off();
	//遮阳帘
	int sunhide_on();
	int sunhide_off();
	//路灯
	int light1_on();
	int light1_off();
	int light2_on();
	int light2_off();
	//增氧泵
	int zengyang1_on();
	int zengyang1_off();
	int zengyang2_on();
	int zengyang2_off();
	//灌溉
	int diancifa_on();
	int diancifa_off();
	//地源热泵
	int rb1_on();
	int rb1_off();
	int rb1_cool_set(int b);
	int rb1_bot_set(int b);
	int rb2_on();
	int rb2_off();
	int rb2_cool_set(int b);
	int rb2_bot_set(int b);
	int rb3_on();
	int rb3_off();
	int rb3_cool_set(int b);
	int rb3_bot_set(int b);
	//PCS设备基本控制
	int PCS_On();
	int PCS_0ff();
	int PCS_Wait();
	int PCS_Charge_command(); //PCS充电使能
	int PCS_Out_command();   //PCS放电使能
	int PCS_VF_command();     //PCS VF使能
	int PCS_DianYa_command();  //PCS电压控制使能
	int PCS_Set_value(int a ,float b );      //设置值  b是值  a 是模式 // 1 恒功率充电值 2恒功率放电值 3恒电压充电值 4无功设定值 5VF 电压设置 6 VF 频率设置
	int PCS_Command(int a);               //使能： 1 PCS充电使能  2 PCS放电使能 3 PCS VF使能 4 PCS电压控制
	int PCS_Set_value_run(int a ,int b ); //设置值  a 是模式  b是值
	int PCS_remove_fault();
	int PCS_CharAndDisOn(int a ,float b);  //PCS充放电开机流程
	//能量路由器
	int Router_Left_OnGird();   //1开机  0关机 其他值无效
	int Router_Left_OffGird();   //1开机  0关机 其他值无效
	int Router_Right_OnGird();
	int Router_Right_OffGird();
	int Router_Left_OnVoltage();
	int Router_Left_OffVoltage();
	int Router_Right_OnVoltage();
	int Router_Right_OffVoltage();
	int Router_OnDC();
	int Router_OffDC();	
	int Router_Set_Voltage(float value);
	int Router_Set_Power(float value);
	int Router_RemovrFault();  //只能下1
	int Router_Clear_CMD();   //清除所有下发的功能使能 然后重新下发
	int RouterOff();   //关闭能量路由器步骤
	int RouterOn(float a);   //开启能量路由器步骤
	//获取能量路由器状态
	int get_Router_fault();  //需要合并
	int get_Left_OnOff();
	int get_Right_OnOff();
	int get_DC_OnOff();
	int get_Left_Voltage_OnOff();
	int get_Right_Voltage_OnOff();
	//获取PCS遥测和遥信
	int getPCSOnOff();  //PCS开关状态
	int getPCSfault();  //PCS故障状态
	int getPCSChar();  //PCS充电状态
	int getPCSDisChar();  //PCS放电状态
	int getPCSOnOffGrid(); //PCS并离网状态  判VF状态
	int getPCSGirdUa(); //PCS交流测A电压
	int getPCSGirdUb(); //PCS交流测B电压
	int getPCSGirdUc(); //PCS交流测C电压
	int checkPcsGridVoltage(); //检查PCS交流侧电压
	int getPCSDCVolTage(); //获取PCS直流侧电压   ID=12 容量
	int checkPcsDcVoltage(); //检查PCS直流电压电压
	float getPCSPower();   //获取当前PCS功率
	float getPointPower();//获取并网点功率
	float getYBPointPower();//获取杨北点功率
	//并网点开关控制
	int on_AC1_1();
	int off_AC1_1();
	int on_AC1_2();
	int off_AC1_2();
	//BMS控制
	int BMS_On();
	//获取BMS故障状态
	int get_alarm_level_1();
	int get_alarm_level_2();
	int get_alarm_level_3();
	//支路负荷控制
	int on_aem();  //G1柜总开关
	int off_aem();
	int sun300_1_off();
	int sun300_2_off();
	int sun200_off();
	int sun300_1_on();
	int sun300_2_on();
	int sun200_on();
	int off_G31();
	int on_G31();
	int on_G34();
	int on_G35();
	int on_G36();
	int on_G37();
	int on_G311();
	int off_G34();
	int off_G35();
	int off_G36();
	int off_G37();
	int off_G311();
	int on_dongli1(); //G39
	int on_dongli2();
	int off_dongli1();// G310
	int off_dongli2();   
	int on_G41();
	int on_G44();
	int	on_G43();
	int	on_G42();
	int off_G41();
	int off_G44();  //gG44
	int	off_G43(); //G43
	int	off_G42(); //G42
	int on_G61(); //水平轴
	int on_G62(); //卧式 
	int on_G63();  //储能 
	int on_G64(); // 
	int on_G65();  //
	int on_G66();  //直流备用 
	int on_G67(); //直流充电桩 
	int on_G68(); //直流备用 
	int off_G61();
	int off_G62();
	int off_G63();
	int off_G64(); 
	int off_G65();  
	int off_G66();
	int off_G67();
	int off_G68();
	int on_G71(); //7KW充电桩
	int on_G72();  //50KW充电桩
	int on_G73(); //路灯
	int on_G74(); //能量路由器端口1
	int on_G75(); //LORA 
	int on_G76(); //UPS 
	int on_G77();  //备用
	int on_G78();  //能量路由器端口3
	int on_G79(); //备用
	int off_G71();
	int off_G72();
	int off_G73();
	int off_G74();
	int off_G75();
	int off_G76();
	int off_G77();
	int off_G78();
	int off_G79();

	int on_rebeng(int a);
	int off_rebeng(int a);
	int on_kt01();
	int on_kt02();
	int on_kt03();
	int on_kt04();
	int on_kt05();
	int on_kt06();
	int on_kt07();  
	int on_kt08();
	int off_kt01();
	int off_kt02();
	int off_kt03();
	int off_kt04();
	int off_kt05();
	int off_kt06();
	int off_kt07();  
	int off_kt08();
	//读写电站表函数 
	int clearCMD();          //清除表中  控制命令
	int clearFaultReset();  //清除表中 故障复归命令
	int set_Station_RunState(float a);    //运行状态：1停止态;3并网态;2离网态;4故障态  
	int set_Station_RunStateInfo(float a);  // 运行状态信息：1离网启动中;2并网启动中;3离网停机中;4并网停机中;5无缝离网中;6无缝并网中;7故障复归中
	int set_Station_RunMode(int a);       //  运行模式：0,手动模式;1,自动模式
	int set_Station_ACGridCmd(int a);      //   1停机指令，2并网指令，3离网指令
	int set_Station_FaultReset(int a);     //     故障复归1
	int set_Station_BasicCtrl(int a);       //   基础运行控制：1选择，0没选
	int set_Station_OffgridManage(int a);   //    离网运行管理：1选择，0没选
	int set_Station_OffgridMSelect(int a);   //    离网策略选择：0没选，1离网功率平衡控制
	int set_Station_OngridManage(float a);    //    并网运行管理：1选择，0没选
	int set_Station_OngridMSelect(float a);  //  并网策略选择：0没选，1并网功率平衡控制，2削峰填谷策略控制, 3并网经济运行管理
	float read_Station_RunState();
	float read_Station_RunStateInfo();
	int read_Station_RunMode();
	int read_Station_ACGridCmd();
	int read_Station_FaultReset();
	int read_Station_BasicCtrl();
	int read_Station_OffgridManage();
	int read_Station_OffgridMSelect();
	float read_Station_OngridManage();
	float read_Station_OngridMSelect();
	//读削峰填谷时间
	int red_Station_TimePeak1St();         //第一段峰时开始间   8
	int red_Station_TimePeak1End();        //第一段峰时结束间   12
	int red_Station_TimePeak2St();          //第二段峰时开始间   17
	int red_Station_TimePeak2End();          //第二段峰时结束间  21
	int red_Station_TimeValley1St();         //第一段谷时开始间   0
	int red_Station_TimeValley1End();          //第一段谷时结束间  8
	//读取最大消纳模式下 晚上储能电池放电时间和放电功率
	int red_Station_TimeDisChar1St();         //放电开始间1
	int red_Station_TimeDisChar1End();        //放电结束时间2
	int red_Station_TimeDisChar2St();         //放电开始间2
	int red_Station_TimeDisChar2End();        //放电结束时间2
	int red_Station_DisCharPower();          //放电功率
	int red_Station_DisCharEnable();          //使能功能
	//电站表 //设置杨北变工作选择
	float red_YB_GridState();     //读YB系统状态 2离网 3并网
	int Set_YB_GritState(float a);      
	int set_YB_RunModet(float a);  //  策略选择：1恒压，2是小功率
	float read_YB_RunModet();
	int Save_YB_RunModet(float a);  // 保存设置的运行模式
	int set_sjt_percent(float a);  //设系数
	int set_yb_percent(float a);  
	float read_sjt_percent();  //松降头系数
	float read_yb_percent();  //杨北系数
	//青禾二期读农业控制的使能标志
	int red_Light_CtrlState();         
	int red_DTSD_CtrlState();       
	int red_Roll_CtrlState(); 
	int red_SunHade_CtrlState();  
	int red_Water_CtrlState(); 
	int red_Rb_CtrlState(); 

	int red_ControlTime();  //农业控制循环周期
	//读取环境变量 根据环境变量来控制自动操作
	//读取路灯开关时间
	int red_Station_LightSt();       
	int red_Station_LightEnd();        
	// 读取溶解氧的设置值
	int red_Station_Ox();       
	// 读取大棚温度的设置值
	int red_Station_DaPengTemp();        
	// 读取灌溉阀土壤湿度的设置值
	int red_Station_wet();        
	//读7号大棚温度上下限设置值
	int red_Station_TempUp();        
	int red_Station_TempDown();        
	//读7号大棚辐射上下限设置值
	int red_Station_FuSheUp();        
	int red_Station_FuSheDown();        
	//青禾二期策略
	int LightCtrl();
	int RbCtrl();
	int RollCtrl();
	int SunHideCtrl();
	int ZengYangCtrl();
	int WaterCtrl();
	//获取遥测 和 遥信
	int getSJTPccState();  //松降头并网点状态
	int getYBPccState();  //杨北并网点状态
	int getSJTGridUa();  //松降头A相电压
	int getSJTGridUb();
	int getSJTGridUc();
	int getYBGridUa();   //杨北A相电压
	int getYBGridUb();
	int getYBGridUc();
	int getMgUa();  //微网母线A相电压
	int getMgUb();
	int getMgUc();
	int getYBMgUa();  //获取杨北区域微网电压
	int getYBMgUb();
	int getYBMgUc();
	//获取SOC
	int getSOC();       //ID=30 容量字段
	//获取光伏直流侧电压
	int  getPVdc();    //ID=30  容量字段
	//检查微网电压 是否正常
	int checkMgVolt();
	//检查微网电压 是否为0
	int checkMgVoltNone();
	//获取负荷支路开关状态
	int get3001sunOnOff();
	int get3002sunOnOff();
	int get2000sunOnOff();
	int getAEMOnOff();  //G1总
	int getG31OnOff();   //G3总
	int getG34OnOff();   //G34
	int getG35OnOff();   //G35
	int getG36OnOff();   //G36
	int getG37OnOff();   //G37
	int getG39OnOff();   //动力1
	int getG310OnOff();  //动力2
	int getG311OnOff();   //G311
	int getG41OnOff();  //G4总
	int getG42OnOff(); //空调
	int getG43OnOff(); //地源热泵
	int getG44OnOff();  //冷库
	int getG61OnOff();  //水平轴
	int getG62OnOff();  //卧式
	int getG63OnOff();  //储能
	int getG64OnOff();  //卧式  注意一下ID
	int getG65OnOff();  //储能  注意一下ID
	int getG66OnOff();  //直流备用
	int getG67OnOff();  //直流充电桩
	int getG68OnOff();  //直流备用
	int getG71OnOff();  //7KW充电桩
	int getG72OnOff();  //50KW充电桩
	int getG73OnOff();  //路灯
	int getG74OnOff();  //能量路由器端口1
	int getG75OnOff();  //LORA
	int getG76OnOff();  //UPS
	int getG77OnOff();  //备用
	int getG78OnOff();  //能量路由器端口3
	int getG79OnOff();  //备用
	//微网状态判断
	int system_state();//项目系统状态判断
	int yb_system_state();//杨北变系统状态
	//策略状态转换主函数 基础运行控制
	void GridBasicCtrl();   //青禾
	void YBGridBasicCtrl();   //青禾
	//策略基本函数
	int ongrid_start();//并网开机
	int offgrid_start();//离网开机
	int offgrid_ongrid();//离网到并网（同期）
	int ongrid_offgrid();//并网到离网（被动离网）
	int all_stop();//全站停机
	int TongqQi();  //同期命令
	int remove_fault();  //微网故障态复归
	//杨北区域策略
	int  YB_ongrid_smallpower();  //杨北变并网小功率输电模式
	int  YB_offgrid_setvoltage(); //杨北变离网恒压模式
	//策略高级函数
	int OffPowerCtrl(); //离网功率平衡控制
	int OnPowerCtrl();  //并网-功率平衡控制
	int MPeakFillValley(); //削峰填谷
	int MAbsorb();  //最大消纳
	int NightDisChar();//夜晚放电
	///////////////////////////测试函数/////用于调试过程中 /////////
	void  test_station_cmd();
	void  test_equipment_onoff(); 
	//主循环
	void main_loop();   //主循环
};
#endif // AGVC_CTRL_MAIN_H
