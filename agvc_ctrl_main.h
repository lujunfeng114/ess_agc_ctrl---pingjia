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

	vector<Cgatepower_info * > gatepower_list;                    //关口电量表
	vector<Cstation_monthpower_info * > station_monthpower_list;  //整站月电量分析表
	vector<Cstationpower_info * > stationpower_list;             //站用电分析表

	vector<Cstation_daypower_info * > station_daypower_list;      //整站日电量分析表
    vector<Cunit_monthpower_info * > unit_monthpower_list;        //储能单元月电量分析表
	vector<Cunit_daypower_info * > unit_daypower_list;            //储能单元日电量分析表
 	vector<Cunit_runstate_info * > unit_runstate_list;            //储能单元运行信息表
 	vector<Cmonth_runanalyse_info * > month_runanalyse_list;      //储能月运行分析表
 	vector<Cunit_runtime_info * > Cunit_runtime_list;            //储能运行时间分析表




	int read_fac_info_table();
	int read_meas_info_table();
	int read_bms_info_table();
	int read_pcs_info_table();
	int read_station_info_table();
	int read_point_info_table();
	int read_agvc_info_table();
	int read_micro_ctrl_info_table();

    //202107新增 储能评价功能用表
	int read_gatepower_info_table();                    //读取关口电量表
	int read_station_monthpower_info_table();           //整站月电量分析表

	int read_stationpower_info_table();               //站用电电量分析表

	int read_station_daypower_info_table();             //整站日电量分析表

	int read_unit_monthpower_info_table();              //储能单元月电量分析表
	int read_unit_daypower_info_table();                //储能单元日电量分析表
	int read_unit_runstate_info_table();                //储能单元运行信息表

	int read_month_runanalyse_info_table();              //储能月运行信息表
    int read_unit_runtime_infi_table();                  //储能运行时间表



	Cfac_info* find_fac_from_list(int fac_id);
	Cmeas_info* find_meas_from_list(int display_id);
	Cbms_info* find_bms_from_list(int display_id);
	Cpcs_info* find_pcs_from_list(int display_id);
	Cstation_info* find_station_from_list(int display_id);
	Cpoint_info* find_point_from_list(int display_id);
	Cagvc_info* find_agvc_from_list(int display_id);
	Cmicro_ctrl_info* find_micro_ctrl_from_list(int display_id);

	Cgatepower_info* find_gatepower_from_list(int display_id);                     //查找某一条关口表电量记录
	Cstation_monthpower_info* find_station_monthpower_from_list(int display_id);   //查找某一条整站月电量记录
	Cstationpower_info* find_stationpower_from_list(int display_id);              //查找某一条站用电电量记录

	Cstation_daypower_info* find_station_daypower_from_list(int display_id);       //查找某一条整站日电量电量记录
	Cunit_monthpower_info* find_unit_monthpower_from_list(int display_id);   
	Cunit_daypower_info* find_unit_daypower_from_list(int display_id);  
	Cunit_runstate_info* find_unit_runstate_from_list(int display_id);  
	Cmonth_runanalyse_info* find_month_runanalyse_from_list(int display_id);  

	Cunit_runtime_info* find_unit_runtime_from_list(int display_id);  

public:
	//遥控遥调转发业务逻辑
	vector<yk_send_struct> yk_send_list;             //读取遥控转发表记录
	vector<yt_send_struct> yt_send_list;             //读取调节量转发表记录
	vector<yk_define_struct> yk_define_list;         //遥控定义表记录
	vector<sw_info_struct> sw_list;                  //开关表记录
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
	void agvc_ctrl_init();//初始化函数
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
	


	
	 //关口电量表数据转存档到整站日电量表和整站月电量表	

	 void save_gatepower_to_dayanmonthpower_value(int num=1);	//默认一个关口表，选择第一行记录
	
	 //储能单元日电量表	
	
	 void save_runstate_to_unitpower_value();
	
	
	 //储能运行分析信息
	 void battery_run_analyse();	
	
	
	
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
	

	//主循环
	void main_loop();   //主循环
};
#endif // AGVC_CTRL_MAIN_H
