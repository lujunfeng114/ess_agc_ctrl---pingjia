#ifndef DEVICE_H
#define DEVICE_H
#include "const_def.h"
#include "data_access.h"
class Cmicro_ctrl_info
{
public:
	Cmicro_ctrl_info(Cdata_access *data_obj);//将序号当成自己的显示序号来处理
	~Cmicro_ctrl_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;
	int display_id;
	char name[128];
	int	fac_id;//厂站ID
	int ctrl_data;//控制动作值
	char ctrl_format;//控制数据格式
	char ctrl_type;//控制类型，设置点控制
	char ctrl_mode;//控制模式，直接控制或者预置控制
	int add_no;//公共地址
	int	dot_no;//点号
	int control_sq;//控制序号  因为表结构无法修改,暂且配置为显示序号
	short display_id_col;
	short name_col;
	short fac_id_col;//厂站ID
	short ctrl_data_col;//控制动作值
	short ctrl_format_col;//控制数据格式
	short ctrl_type_col;//控制类型，设置点控制
	short ctrl_mode_col;//控制模式，直接控制或者预置控制
	short add_no_col;//公共地址
	short dot_no_col;//点号
	short control_sq_col;//控制序号  因为表结构无法修改,暂且配置为显示序号
public:
	int read_micro_ctrl_rdb();
};
//电池堆BMS信息
class Cbms_info
{
public:
	Cbms_info(Cdata_access *_data_obj);
	~Cbms_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char  name[128];
	float  soc_min;    //允许放电最小soc
	float  soc_max;    //允许充电最大soc
	float  soc_now;    //当前soc
	float  kwh;        //堆额定容量
	float  ava_char;   //可充电量
	float  ava_disc;//可放电量
	short display_id_col;
	short name_col;
	short soc_min_col;
	short soc_max_col;
	short soc_now_col;
	short kwh_col;
	short ava_char_col;
	short ava_dischar_col;
public:
	int read_bms_rdb();     //读取bms实时值信息
};




class Cpcs_info
{
public:
	Cpcs_info(Cdata_access *_data_obj);
	~Cpcs_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int	record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char name[128];
	float ac_ua;              //交流A相电压
	float ac_ub;
	float ac_uc;
	char    on_off_stat;         //开关机状态  分合闸状态                                              
	float	p_max_char;	    //最小有功功率        //青禾 PCS充电状态                //左侧并网
	float	p_max_disc;	    //最大有功功率表     //青禾 PCS放电状态                //右侧并网
	float   q_max_char;     //最小无功值        //青禾 PCSVF状态                   //左侧稳压
	float   q_max_disc;     //最大无功值        //青禾 PCS控电压状态               //右侧稳压
	char fault_total;            //PCS总故障                                       //故障状态
	char standby_stat;           //待机状态                                       //直流输出开关    
	float capacity; //容量   ID=30号 SOC容量  ID=12 PCS直流侧电压
	float  p;                  //当前有功功率                                   //ID=12  PCS功率
	////////////////////////以下未用到/////////////////////////////
	float  q;                  //当前无功功率
	char if_use;                 //手动设定该PCS是否参与功率计算
	char ctrl_mode;              //远程就地状态
	int run_mode;               //运行模式  按照调度给定的模式计算后填表
	char ctrl_condition;         //受控条件 用于判定当前PCS是否可以下发功率
	char on_off_grid_stat;       //并离网状态
	char if_alarm;          //总告警
	char if_fault;			//总故障
	float	p_set;				//有功下发值
	float	q_set;				//无功下发值
	char alarm_level_1;     //一级告警
	char alarm_level_2;     //二级告警
	char alarm_level_3;     //三级告警
	int alarm_level_1_act;//一级告警动作(%)
	int alarm_level_2_act;//二级告警动作(%)
	int alarm_level_3_act;//三级告警动作(%)
	short display_id_col;
	short name_col;
	short p_col;                 //当前有功功率
	short q_col;                 //当前无功功率
	short p_max_char_col;        //最大充电功率
	short p_max_disc_col;        //最大放电功率
	short if_use_col;            //手动设定该PCS是否参与功率计算
	short fault_total_col;       //PCS总故障
	short ctrl_mode_col;         //远程就地状态
	short run_mode_col;          //运行模式  按照调度给定的模式计算后填表
	short ctrl_condition_col;    //受控条件 用于判定当前PCS是否可以下发功率
	short on_off_stat_col;       //开关机状态
	short on_off_grid_stat_col;  //并离网状态
	short standby_stat_col;      //待机状态
	short if_alarm_col;          //总告警
	short if_fault_col;			//总故障
	short q_max_char_col;     //无功最大值
	short q_max_disc_col;     //无功最小值
	short capacity_col;
	short alarm_level_1_col;     //一级告警
	short alarm_level_2_col;     //二级告警
	short alarm_level_3_col;     //三级告警
	short alarm_level_1_act_col;//一级告警动作(%)
	short alarm_level_2_act_col;//二级告警动作(%)
	short alarm_level_3_act_col;//三级告警动作(%)
	short ac_ua_col;          //A相电压
	short ac_ub_col; 
	short ac_uc_col; 
	Cbms_info *bms;             //该PCS对应的BMS信息
	//PCS遥调通道信息
	Cmicro_ctrl_info *emergency_reset_micro_ctrl;//PCS源网荷紧急复归控制
	//  Cmicro_ctrl_info *yt_p_micro_ctrl;  //pcs有功调节对应的微电网控制定义记录
	Cmicro_ctrl_info *yt_dc_kailu_micro_ctrl;  //开路电压  
	Cmicro_ctrl_info *yt_a_kailu_micro_ctrl;  //开路电流
	Cmicro_ctrl_info *yt_1_micro_ctrl;  //系数1
	Cmicro_ctrl_info *yt_2_micro_ctrl;  //系数2
	Cmicro_ctrl_info *yt_3_micro_ctrl;  //系数3
	Cmicro_ctrl_info *yt_4_micro_ctrl;  //系数4
	//卢俊峰 20190307 青禾 新增
	Cmicro_ctrl_info *yt_q_micro_ctrl;  //pcs无功调节对应的微电网控制定义记录  
	Cmicro_ctrl_info *yt_p_chong_micro_ctrl;  //pcs有功调节对应的微电网控制定义记录
	Cmicro_ctrl_info *yt_p_fang_micro_ctrl;  //pcs有功调节对应的微电网控制定义记录
	Cmicro_ctrl_info *yt_voltage_micro_ctrl;  //pcs恒压浮充  
	Cmicro_ctrl_info *yt_vf_ac_micro_ctrl;  //pcs离网电压
	Cmicro_ctrl_info *yt_vf_hz_micro_ctrl;  //pcs离网频率
	Cmicro_ctrl_info *yt_pz_micro_ctrl;  // 作为PZ的遥调控制开关  4096合 8192分
	Cmicro_ctrl_info *yt_aemfen_micro_ctrl;  //作为AEM表的遥调控制开关 分
	Cmicro_ctrl_info *yt_aemhe_micro_ctrl;  //作为AEM表的遥调控制开关 合
	Cmicro_ctrl_info *yt_sunon_micro_ctrl;  //作为sun光伏逆变器的开机
	Cmicro_ctrl_info *yt_sunoff_micro_ctrl;  //作为sun光伏逆变器的关机
	Cmicro_ctrl_info *yk_p_chong_micro_ctrl;  //pcs有功调节对应的微电网控制定义记录
	Cmicro_ctrl_info *yk_p_fang_micro_ctrl;  //pcs有功调节对应的微电网控制定义记录
	Cmicro_ctrl_info *yk_poweron_micro_ctrl;	//pcs起机对应的微电网控制定义记录
	Cmicro_ctrl_info *yk_poweroff_micro_ctrl;	//pcs停机对应的微电网控制定义记录
	Cmicro_ctrl_info *yk_standby_micro_ctrl;	//PCS设备待机对应的微电网控制定义记录
	Cmicro_ctrl_info *yk_vf_micro_ctrl;		//pcs vf使能
	Cmicro_ctrl_info *yk_voltage_micro_ctrl;	//pcs 恒压浮充使能
	Cmicro_ctrl_info *yk_failure_micro_ctrl;	//pcs 故障复归
	Cmicro_ctrl_info *yk_offnet_micro_ctrl;	// 主动离网   //用于斯菲尔表  AC1-1 3号继电器  //目前PCS没有主动离网功能
	Cmicro_ctrl_info *yk_tongqi_micro_ctrl;	// 同期   //用于斯菲尔表 G6-5 3号继电器
	//能量路由器微电网控制定义表//
	Cmicro_ctrl_info *yt_router_left_ongird_onoff_micro_ctrl;
	Cmicro_ctrl_info *yt_router_right_ongird_onoff_micro_ctrl;
	Cmicro_ctrl_info *yt__router_dconoff_micro_ctrl;
	Cmicro_ctrl_info *yt__router_left_voltage_onoff_micro_ctrl;
	Cmicro_ctrl_info *yt__router_right_voltage_onoff_micro_ctrl;
	Cmicro_ctrl_info *yt_router_power_micro_ctrl;
	Cmicro_ctrl_info *yt_router_voltage_micro_ctrl;
	Cmicro_ctrl_info *yt_router_removefault_micro_ctrl;  
	// 青禾二期
	Cmicro_ctrl_info *yt_artu1_3_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu1_4_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu1_5_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu1_6_micro_ctrl; 
	Cmicro_ctrl_info *yt_artu1_7_micro_ctrl; 
	Cmicro_ctrl_info *yt_artu1_8_micro_ctrl; 
	Cmicro_ctrl_info *yt_artu2_3_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu2_4_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu2_5_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu2_6_micro_ctrl; 
	Cmicro_ctrl_info *yt_artu2_7_micro_ctrl; 
	Cmicro_ctrl_info *yt_artu2_8_micro_ctrl; 
	Cmicro_ctrl_info *yt_artu3_3_micro_ctrl;  
	Cmicro_ctrl_info *yt_artu3_4_micro_ctrl;  
	Cmicro_ctrl_info *yt_dtsd1_micro_ctrl; 
	Cmicro_ctrl_info *yt_dtsd2_micro_ctrl; 
	Cmicro_ctrl_info *yt_light1_micro_ctrl; 
	Cmicro_ctrl_info *yt_light2_micro_ctrl;
	Cmicro_ctrl_info *yt_lkonoff_micro_ctrl;  
	Cmicro_ctrl_info *yt_lklowertemp_micro_ctrl;  
	Cmicro_ctrl_info *yt_lkuppertemp_micro_ctrl;  
	Cmicro_ctrl_info *yt_rb1_onoff_micro_ctrl; 
	Cmicro_ctrl_info *yt_rb1_cooltemp_micro_ctrl;
	Cmicro_ctrl_info *yt_rb1_hottemp_micro_ctrl;
	Cmicro_ctrl_info *yt_rb2_onoff_micro_ctrl; 
	Cmicro_ctrl_info *yt_rb2_cooltemp_micro_ctrl;
	Cmicro_ctrl_info *yt_rb2_hottemp_micro_ctrl;
	Cmicro_ctrl_info *yt_rb3_onoff_micro_ctrl; 
	Cmicro_ctrl_info *yt_rb3_cooltemp_micro_ctrl;
	Cmicro_ctrl_info *yt_rb3_hottemp_micro_ctrl;
	Cmicro_ctrl_info *yt_water_3_micro_ctrl;  
	Cmicro_ctrl_info *yt_water_4_micro_ctrl;  
	Cmicro_ctrl_info *yt_water_5_micro_ctrl;  
	Cmicro_ctrl_info *yt_water_6_micro_ctrl; 
	Cmicro_ctrl_info *yt_water_7_micro_ctrl; 
	Cmicro_ctrl_info *yt_water_8_micro_ctrl; 
public:
	int read_pcs_rdb();           //回读刷新函数
	//////////////////////////////以下未用到//////////////////////////////////////////////////////
	bool check_status();         //PCS状态检查
	bool check_power_status(float p_ess);   //检查当前PCS是否具备对p_ess功率的调节能力
	//(该函数考虑了包括PCS状态检查/过充/过放等情况下的逻辑判定,
	//仅用于功率分配函数的分配权重参与逻辑) 
	int emergency_reset();       //PCS源网荷紧急复归控制
	void try_power_to_pcs(float p_try, float q_try);  //尝试给符合条件的PCS发送功率调节值
	//////////////////////////////以上未用到//////////////////////////////////////////////////////	
	/////////////////////////////通用遥控遥调函数 卢俊峰 201903///////////////////	
	int set_YT(Cmicro_ctrl_info *yt_micro_ctrl,float value);
	int set_YK(Cmicro_ctrl_info *yk_micro_ctrl);
	///////////////////////////能量路由器底层控制函数 卢俊峰 201903///  ///////////
	////////////////////////////////////////////////////////////////////////
	int set_router_left_ongird_onoff(float value);   //左侧并网使能
	int set_router_right_ongird_onoff(float value);  //右侧并网使能
	int set_router_dconoff(float value);  //DC直流断路器
	int set_router_left_voltage_onoff(float value); //左侧稳压
	int set_router_right_voltage_onoff(float value); //右侧稳压
	int set_router_voltage(float value);  //设电压
	int set_router_power(float value);          //设功率
	int set_router_removefault(float value);   // 故障复归
	//////////////////////////////////以下PCS设备底层基本操作函数 之前写的 也测过了 就不去改简写了/////////////////////////////
	//PCS 基本操作
	int set_standby();         //设置PCS待机
	int set_poweron();          //设置PCS开机
	int set_poweroff();         //设置PCS关机
	//同期命令 G6-5下发 此处也放到PCS里面
	int set_tongqi();//G6-5下发同期命令
	//PCS  遥调
	//添加 放电值设定 ，恒压浮充电压设定，VF 电压值，VF频率
	int set_const_p_power_fang(float p);    //放电
	int set_const_p_power_chong(float p);    //设定恒功率控制模式下有功功率输入
	int set_const_q_power(int q);    //设定无功功率输出
	int set_const_dc(int v);//恒压浮充电压设定
	int set_const_vf_ac(int v);//VF 电压值
	int set_const_vf_hz(int h);//VF频率
	//放电使能，充电使能，恒压浮充使能，故障复归使能，离网VF使能
	int set_power_fang();		//放电使能
	int set_power_chong();		//充电使能
	int set_const_voltage_dc(); //恒压浮充使能
	int set_failure_recovery(); //故障复归使能
	int set_off_grid();         //离网VF使能
	//卢俊峰 20190307 青禾 新增
	int set_const_pz_micro_ctrl(int p);  // 作为PZ的遥调控制开关  4096合 8192分
	int set_const_aemfen_micro_ctrl(int p);  //作为AEM表的遥调控制开关 分
	int set_const_aemhe_micro_ctrl(int p);  //作为AEM表的遥调控制开关 合
	int set_const_sunon_micro_ctrl(int p);  //作为sun光伏逆变器的开机
	int set_const_sunoff_micro_ctrl(int p);  //作为sun光伏逆变器的关机
	//青禾二期
	int set_artu1_3_onoff(float value);
	int set_artu1_4_onoff(float value);
	int set_artu1_5_onoff(float value);
	int set_artu1_6_onoff(float value);
	int set_artu1_7_onoff(float value);
	int set_artu1_8_onoff(float value);
	int set_artu2_3_onoff(float value);
	int set_artu2_4_onoff(float value);
	int set_artu2_5_onoff(float value);
	int set_artu2_6_onoff(float value);
	int set_artu2_7_onoff(float value);
	int set_artu2_8_onoff(float value);
	int set_artu3_3_onoff(float value);
	int set_artu3_4_onoff(float value);

	int set_light1_onoff(float value);
	int set_light2_onoff(float value);
	int set_dtsd1_onoff(float value);          //dtsd
	int set_dtsd2_onoff(float value);          //dtsd
	int set_lk_onoff(float value);          //冷库开关机
	int set_lk_lowertemp(float value);          //设温度下限
	int set_lk_uppertemp(float value);          //设温度上限
	int set_rb1_onoff(float value);          //热泵开关机
	int set_rb1_cooltemp(float value);          //设热泵制冷温度
	int set_rb1_hottemp(float value);          //设热泵制热温度
	int set_rb2_onoff(float value);          //热泵开关机
	int set_rb2_cooltemp(float value);          //设热泵制冷温度
	int set_rb2_hottemp(float value);          //设热泵制热温度
	int set_rb3_onoff(float value);          //热泵开关机
	int set_rb3_cooltemp(float value);          //设热泵制冷温度
	int set_rb3_hottemp(float value);          //设热泵制热温度

	int set_artu4_3_onoff(float value);    //电磁阀1~6
	int set_artu4_4_onoff(float value);
	int set_artu4_5_onoff(float value);
	int set_artu4_6_onoff(float value);
	int set_artu4_7_onoff(float value);
	int set_artu4_8_onoff(float value);
};


//储能电站表  非厂站信息表
class Cstation_info
{
public:
	Cstation_info(Cdata_access *_data_obj);
	~Cstation_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int	record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char  name[128];
	//微电网运行控制字 直接写表
	float cap_r;         //电站额定容量   运行状态：1停止态;2并网态;3离网态;4故障态 （电站额定容量）
	float p_r;           //电站额定功率   运行状态信息：1离网启动中;2并网启动中;3离网停机中;4并网停机中;5无缝离网中;6无缝并网中;7故障复归中  （额定功率）    
	int bay_num;        //间隔数量   运行模式：0,手动模式;1,自动模式    （间隔数量）
	int run_bay_num;    //运行间隔数量  1停机指令，2并网指令，3离网指令  （运行间隔）
	int stop_bay_num;  //停运间隔数量  故障复归1                       （停运间隔）
	int bats_num;      //基础运行控制：1选择，0没选                           （电池簇数量）
	int pcs_num;      //离网运行管理：1选择，0没选                            （PCS数量）
	int bms_num;      // 离网策略选择：0没选，1离网功率平衡控制  （电站运行状态） 
	char  plt_run_mod;  //    预留   tinyint  小范围  char显示有问题
	float day_ongrid_energy;//当日上网电量  并网运行管理：1选择，0没选  
	float day_downgrid_energy;//当日下网电量  并网策略选择：0没选，1并网功率平衡控制，2削峰填谷策略控制, 3并网经济运行管理 
	//(注：建议开发组新建一个微网控制字表 里面都是INT)
	float soc_total;       //电站总SOC     //杨北工作模式 1 恒压
	float soc_up;          //电站SOC上限  //保存设置的杨北设置的当前手动设置目标工作状态，用不界面显示，因为soc_total设置完 会被清掉。
	float  soc_low;                       //保存杨北根据系统条件判断得到的当前杨北的并离网状态 2离网 3并网
	float ava_kwh_disc;    //可放电量    //松降头平衡系数 （0 ~1） 默认0.9
	float ava_kwh_char;    //可充电量    //杨北平衡系数  （0 ~1） 默认0.5
	////////////////以下无用//////////////
	float status;           //电站投运状态    
	int plt_run_status; 
	float latitude;     //!!!临时使用字段,用于下发各pcs有功值
	float longtitude;   //!!!临时使用字段,用于下发各pcs无功值
	short plt_run_mod_col;    //!!!临时使用字段,用于定义储能电站全站有功无功下发.
	short latitude_col;     //!!!临时使用字段,用于下发各pcs有功值
	short longtitude_col;   //!!!临时使用字段,用于下发各pcs无功值
	short day_ongrid_energy_col;//当日上网电量  
	short day_downgrid_energy_col;//当日下网电量  
	short display_id_col;
	short name_col;
	short cap_r_col;    //电站额定容量
	short p_r_col;      //电站额定功率
	short bay_num_col; //间隔数量               （重庆项目：当作基础运行控制选择）
	short run_bay_num_col;        //运行间隔数量 
	short stop_bay_num_col;       //停运间隔数量  
	short bats_num_col;
	short pcs_num_col;
	short bms_num_col;
	short plt_run_status_col;     //电站运行状态  
	short ava_kwh_disc_col;    //可放电量
	short ava_kwh_char_col;    //可充电量
	short soc_total_col;       //电站总SOC
	short soc_up_col;          //电站SOC上限
	short soc_low_col;         //电站SOC下限
	short status_col;     //电站投运状态     
public:
	int read_station_rdb();
};
//并网点表
class Cpoint_info
{
public:
	Cpoint_info(Cdata_access *_data_obj);
	~Cpoint_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char name[128];
	float p;   //有功功率
	float q;	//无功功率
	float cos; //功率因素
	float p_kwh;   //正向有功电能
	float pn_kwh;	//反向有功电能
	float q_kvarh;	//正向无功电能
	float qn_kvarh;//反向无功电能
	short display_id_col;
	short name_col;
	short p_col;   //有功功率
	short q_col;	//无功功率
	short cos_col; //功率因素
	short p_kwh_col;   //正向有功电能
	short pn_kwh_col;	//反向有功电能
	short q_kvarh_col;	//正向无功电能
	short qn_kvarh_col;//反向无功电能
public:
	int read_point_rdb();
};
//AGVC表
class Cagvc_info
{
public:
	Cagvc_info(Cdata_access *_data_obj);
	~Cagvc_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char name[64];
	int total_fault;    //全站事故总
	int char_done;      //充电完成
	int dischar_done;	//放电完成
	int if_agc_ctrl;	//AGC允许控制信号
	int agc_remote_mode; //AGC远方就地信号
	int char_lock;      //充电闭锁
	int dischar_lock;   //放电闭锁
	int ask_agc_on;     //调度请求远方投入
	int ask_agc_status; //请求保持信号
	float agc_p_dest;  //有功目标值
	float agc_p_return;//有功目标反馈值
	float p_max_char;  //最大充电功率允许值
	float p_max_dischar;       //最大放电功率允许值
	float p_max_dischar_time;  //最大功率放电可用时间
	float p_max_char_time;     //最大功率充电可用时间
	float dead_value;   //agvc控制死区
	int ctrl_interval;    //agvc控制周期(秒)
	int agc_chanel_stat;    //agc通道状态
	int avc_chanel_stat;    //avc通道状态
	on_time_t agc_last_modify_time; //agc最后一次请求时间
	on_time_t avc_last_modify_time; //avc最后一次请求时间
	//avc
	int if_agv_ctrl;	//AVC允许控制信号
	int ask_avc_on;     //AVC投入/退出
	int ask_avc_status; //AVC请求保持信号
	int avc_remote_mode; //AVC子站远方/就地
	int add_q_lock;     //AVC子站增无功闭锁
	int reduce_q_lock;  //AVC子站减无功闭锁
	int avc_u_q_ctrl_mode;  //AVC子站电压/无功控制模式
	float avc_add_q;    //AVC子站可增无功
	float avc_reduce_q; //AVC子站可减无功
	float avc_u_dest;   //高压侧母线电压目标值
	float avc_u_ref;    //高压侧母线电压参考值
	float avc_q_dest;   //AVC 无功目标值
	float avc_q_ref;    //AVC 无功参考值

	short display_id_col;
	short name_col;
	short total_fault_col;    //全站事故总
	short char_done_col;      //充电完成
	short dischar_done_col;	//放电完成
	short if_agc_ctrl_col;	//AGC允许控制信号
	short agc_remote_mode_col; //AGC远方就地信号
	short char_lock_col;      //充电闭锁
	short dischar_lock_col;   //放电闭锁
	short ask_agc_on_col;     //调度请求远方投入
	short ask_agc_status_col; //请求保持信号
	short agc_p_dest_col;  //有功目标值
	short agc_p_return_col;//有功目标反馈值
	short p_max_char_col;  //最大充电功率允许值
	short p_max_dischar_col;       //最大放电功率允许值
	short p_max_dischar_time_col;  //最大功率放电可用时间
	short p_max_char_time_col;     //最大功率充电可用时间
	short dead_value_col;   //agvc控制死区
	short ctrl_interval_col;    //agvc控制周期(秒)    
	short if_agv_ctrl_col;	//AVC允许控制信号
	short ask_avc_on_col;     //AVC投入/退出
	short ask_avc_status_col; //AVC请求保持信号
	short avc_remote_mode_col; //AVC子站远方/就地
	short add_q_lock_col;     //AVC子站增无功闭锁
	short reduce_q_lock_col;  //AVC子站减无功闭锁
	short avc_u_q_ctrl_mode_col;  //AVC子站电压/无功控制模式
	short avc_add_q_col;    //AVC子站可增无功
	short avc_reduce_q_col; //AVC子站可减无功
	short avc_u_dest_col;   //高压侧母线电压目标值
	short avc_u_ref_col;    //高压侧母线电压参考值
	short avc_q_dest_col;   //AVC 无功目标值
	short avc_q_ref_col;    //AVC 无功参考值
	short agc_chanel_stat_col;    //agc通道状态
	short avc_chanel_stat_col;    //avc通道状态
	short agc_last_modify_time_col; //agc最后一次请求时间
	short avc_last_modify_time_col; //avc最后一次请求时间
public:
	int read_agvc_rdb();
};


//关口电量信息表
class Cgatepower_info
{
public:
	Cgatepower_info(Cdata_access *_data_obj);
	~Cgatepower_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char name[64];

	on_time_t  month;    //月份
    double thismonth_uppower;      //当月上网电量
    double last1_uppower;      //上1月上网电量
    double last2_uppower;      //
    double last3_uppower;      //
    double last4_uppower;      //
    double last5_uppower;      //
    double last6_uppower;      //
    double last3_uppower;      //
    double last7_uppower;      //
    double last8_uppower;      //
    double last9_uppower;      //
    double last10_uppower;      //
    double last11_uppower;      //
    double last12_uppower;      //

    double thismonth_downpower;      //当月下网电量
    double last1_downpower;      //上1月下网电量
    double last2_downpower;      //
    double last3_downpower;      //
    double last4_downpower;      //
    double last5_downpower;      //
    double last6_downpower;      //
    double last3_downpower;      //
    double last7_downpower;      //
    double last8_downpower;      //
    double last9_downpower;      //
    double last10_downpower;      //
    double last11_downpower;      //
    double last12_downpower;      //

    double today_uppower;      //当日上网电量
    double day1_uppower;      //昨日上网电量
    double day2_uppower;      //前日上网电量
    double day3_uppower;      //上3日上网电量
    double day4_uppower;      //
    double day5_uppower;      //
    double day6_uppower;      //
    double day3_uppower;      //
    double day7_uppower;      //
    double day8_uppower;      //
    double day9_uppower;      //
    double day10_uppower;      //
    double day11_uppower;      //
    double day12_uppower;      //
    double day13_uppower;      //
    double day14_uppower;      //
    double day15_uppower;      //
    double day16_uppower;      //
    double day17_uppower;      //
    double day18_uppower;      //
    double day19_uppower;      //
    double day20_uppower;      //
    double day21_uppower;      //
    double day22_uppower;      //
    double day23_uppower;      //
    double day24_uppower;      //
    double day25_uppower;      //
    double day26_uppower;      //
    double day27_uppower;      //
    double day28_uppower;      //
    double day29_uppower;      //
    double day30_uppower;      //
    double day31_uppower;      //


   double today_downpower;      //当日下网电量
    double day1_downpower;      //昨日下网电量
    double day2_downpower;      //前日下网电量
    double day3_downpower;      //上3日下网电量
    double day4_downpower;      //
    double day5_downpower;      //
    double day6_downpower;      //
    double day3_downpower;      //
    double day7_downpower;      //
    double day8_downpower;      //
    double day9_downpower;      //
    double day10_downpower;      //
    double day11_downpower;      //
    double day12_downpower;      //
    double day13_downpower;      //
    double day14_downpower;      //
    double day15_downpower;      //
    double day16_downpower;      //
    double day17_downpower;      //
    double day18_downpower;      //
    double day19_downpower;      //
    double day20_downpower;      //
    double day21_downpower;      //
    double day22_downpower;      //
    double day23_downpower;      //
    double day24_downpower;      //
    double day25_downpower;      //
    double day26_downpower;      //
    double day27_downpower;      //
    double day28_downpower;      //
    double day29_downpower;      //
    double day30_downpower;      //
    double day31_downpower;      //

	//关口电量
	short display_id_col;
	short name_col;
    on_time_t  month_col;    //月份
    short thismonth_uppower_col;      //当月上网电量
    short last1_uppower_col;      //上1月上网电量
    short last2_uppower_col;      //
    short last3_uppower_col;      //
    short last4_uppower_col;      //
    short last5_uppower_col;      //
    short last6_uppower_col;      //
    short last3_uppower_col;      //
    short last7_uppower_col;      //
    short last8_uppower_col;      //
    short last9_uppower_col;      //
    short last10_uppower_col;      //
    short last11_uppower_col;      //
    short last12_uppower_col;      //
    short thismonth_downpower_col;      //当月下网电量
    short last1_downpower_col;      //上1月下网电量
    short last2_downpower_col;      //
    short last3_downpower_col;      //
    short last4_downpower_col;      //
    short last5_downpower_col;      //
    short last6_downpower_col;      //
    short last3_downpower_col;      //
    short last7_downpower_col;      //
    short last8_downpower_col;      //
    short last9_downpower_col;      //
    short last10_downpower_col;      //
    short last11_downpower_col;      //
    short last12_downpower_col;      //
    short today_uppower_col;      //当日上网电量
    short day1_uppower_col;      //昨日上网电量
    short day2_uppower_col;      //前日上网电量
    short day3_uppower_col;      //上3日上网电量
    short day4_uppower_col;      //
    short day5_uppower_col;      //
    short day6_uppower_col;      //
    short day3_uppower_col;      //
    short day7_uppower_col;      //
    short day8_uppower_col;      //
    short day9_uppower_col;      //
    short day10_uppower_col;      //
    short day11_uppower_col;      //
    short day12_uppower_col;      //
    short day13_uppower_col;      //
    short day14_uppower_col;      //
    short day15_uppower_col;      //
    short day16_uppower_col;      //
    short day17_uppower_col;      //
    short day18_uppower_col;      //
    short day19_uppower_col;      //
    short day20_uppower_col;      //
    short day21_uppower_col;      //
    short day22_uppower_col;      //
    short day23_uppower_col;      //
    short day24_uppower_col;      //
    short day25_uppower_col;      //
    short day26_uppower_col;      //
    short day27_uppower_col;      //
    short day28_uppower_col;      //
    short day29_uppower_col;      //
    short day30_uppower_col;      //
    short day31_uppower_col;      //
   short today_downpower_col;      //当日下网电量
    short day1_downpower_col;      //昨日下网电量
    short day2_downpower_col;      //前日下网电量
    short day3_downpower_col;      //上3日下网电量
    short day4_downpower_col;      //
    short day5_downpower_col;      //
    short day6_downpower_col;      //
    short day3_downpower_col;      //
    short day7_downpower_col;      //
    short day8_downpower_col;      //
    short day9_downpower_col;      //
    short day10_downpower_col;      //
    short day11_downpower_col;      //
    short day12_downpower_col;      //
    short day13_downpower_col;      //
    short day14_downpower_col;      //
    short day15_downpower_col;      //
    short day16_downpower_col;      //
    short day17_downpower_col;      //
    short day18_downpower_col;      //
    short day19_downpower_col;      //
    short day20_downpower_col;      //
    short day21_downpower_col;      //
    short day22_downpower_col;      //
    short day23_downpower_col;      //
    short day24_downpower_col;      //
    short day25_downpower_col;      //
    short day26_downpower_col;      //
    short day27_downpower_col;      //
    short day28_downpower_col;      //
    short day29_downpower_col;      //
    short day30_downpower_col;      //
    short day31_downpower_col;      //

	
public:
	int read_gatepower_rdb();
};



//整站月电量分析表
class Cstation_monthpower_info
{
public:
	Cstation_monthpower_info(Cdata_access *_data_obj);
	~Cstation_monthpower_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char name[64];

	on_time_t  month;    //月份
	double year_uppower;      //整站年上网电量
	double year_downpower;      //整站年下网电量

	double month1_uppower;      //1月上网电量
	double month2_uppower;      //
	double month3_uppower;      //
	double month4_uppower;      //
	double month5_uppower;      //
	double month6_uppower;      //
	double month3_uppower;      //
	double month7_uppower;      //
	double month8_uppower;      //
	double month9_uppower;      //
	double month10_uppower;      //
	double month11_uppower;      //
	double month12_uppower;      //
	double season1_uppower;      //1季度上网电量
	double season2_uppower;      //
	double season3_uppower;      //
	double season4_uppower;      //


	double month1_downpower;      //1月下网电量
	double month2_downpower;      //
	double month3_downpower;      //
	double month4_downpower;      //
	double month5_downpower;      //
	double month6_downpower;      //
	double month3_downpower;      //
	double month7_downpower;      //
	double month8_downpower;      //
	double month9_downpower;      //
	double month10_downpower;      //
	double month11_downpower;      //
	double month12_downpower;      //
	double season1_downpower;      //1季度下网电量
	double season2_downpower;      //
	double season3_downpower;      //
	double season4_downpower;      //	


	//
	short display_id_col;
	short name_col;
	on_time_t  month_col;    //月份
	short year_uppower_col;      //当月上网电量
	short year_downpower_col;      //当月上网电量


	short month1_uppower_col;      //1月上网电量
	short month2_uppower_col;      //
	short month3_uppower_col;      //
	short month4_uppower_col;      //
	short month5_uppower_col;      //
	short month6_uppower_col;      //
	short month3_uppower_col;      //
	short month7_uppower_col;      //
	short month8_uppower_col;      //
	short month9_uppower_col;      //
	short month10_uppower_col;      //
	short month11_uppower_col;      //
	short month12_uppower_col;      //
	short season1_uppower_col;      //
	short season2_uppower_col;      //
	short season3_uppower_col;      //
	short season4_uppower_col;      //

	short month1_downpower_col;      //1月上网电量
	short month2_downpower_col;      //
	short month3_downpower_col;      //
	short month4_downpower_col;      //
	short month5_downpower_col;      //
	short month6_downpower_col;      //
	short month3_downpower_col;      //
	short month7_downpower_col;      //
	short month8_downpower_col;      //
	short month9_downpower_col;      //
	short month10_downpower_col;      //
	short month11_downpower_col;      //
	short month12_downpower_col;      //
	short season1_downpower_col;      //
	short season2_downpower_col;      //
	short season3_downpower_col;      //
	short season4_downpower_col;      //


public:
	int read_station_monthpower_rdb();
};



//整站日电量分析表
class Cstation_daypower_info
{
public:
	Cstation_daypower_info(Cdata_access *_data_obj);
	~Cstation_daypower_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID
	char name[64];

	on_time_t  day;    //月份
	double day1_uppower;      //1号上网电量
	double day2_uppower;      //2号上网电量
	double day3_uppower;      //3号上网电量
	double day4_uppower;      //
	double day5_uppower;      //
	double day6_uppower;      //
	double day3_uppower;      //
	double day7_uppower;      //
	double day8_uppower;      //
	double day9_uppower;      //
	double day10_uppower;      //
	double day11_uppower;      //
	double day12_uppower;      //
	double day13_uppower;      //
	double day14_uppower;      //
	double day15_uppower;      //
	double day16_uppower;      //
	double day17_uppower;      //
	double day18_uppower;      //
	double day19_uppower;      //
	double day20_uppower;      //
	double day21_uppower;      //
	double day22_uppower;      //
	double day23_uppower;      //
	double day24_uppower;      //
	double day25_uppower;      //
	double day26_uppower;      //
	double day27_uppower;      //
	double day28_uppower;      //
	double day29_uppower;      //
	double day30_uppower;      //
	double day31_uppower;      //


	double day1_downpower;      //
	double day2_downpower;      //
	double day3_downpower;      //
	double day4_downpower;      //
	double day5_downpower;      //
	double day6_downpower;      //
	double day3_downpower;      //
	double day7_downpower;      //
	double day8_downpower;      //
	double day9_downpower;      //
	double day10_downpower;      //
	double day11_downpower;      //
	double day12_downpower;      //
	double day13_downpower;      //
	double day14_downpower;      //
	double day15_downpower;      //
	double day16_downpower;      //
	double day17_downpower;      //
	double day18_downpower;      //
	double day19_downpower;      //
	double day20_downpower;      //
	double day21_downpower;      //
	double day22_downpower;      //
	double day23_downpower;      //
	double day24_downpower;      //
	double day25_downpower;      //
	double day26_downpower;      //
	double day27_downpower;      //
	double day28_downpower;      //
	double day29_downpower;      //
	double day30_downpower;      //
	double day31_downpower;      //

	//整站日电量
	short display_id_col;
	short name_col;
	on_time_t  day_col;    //月份

	short day1_uppower_col;      //1号上网电量
	short day2_uppower_col;      //2号上网电量
	short day3_uppower_col;      //3号上网电量
	short day4_uppower_col;      //
	short day5_uppower_col;      //
	short day6_uppower_col;      //
	short day3_uppower_col;      //
	short day7_uppower_col;      //
	short day8_uppower_col;      //
	short day9_uppower_col;      //
	short day10_uppower_col;      //
	short day11_uppower_col;      //
	short day12_uppower_col;      //
	short day13_uppower_col;      //
	short day14_uppower_col;      //
	short day15_uppower_col;      //
	short day16_uppower_col;      //
	short day17_uppower_col;      //
	short day18_uppower_col;      //
	short day19_uppower_col;      //
	short day20_uppower_col;      //
	short day21_uppower_col;      //
	short day22_uppower_col;      //
	short day23_uppower_col;      //
	short day24_uppower_col;      //
	short day25_uppower_col;      //
	short day26_uppower_col;      //
	short day27_uppower_col;      //
	short day28_uppower_col;      //
	short day29_uppower_col;      //
	short day30_uppower_col;      //
	short day31_uppower_col;      //

	short day1_downpower_col;      //
	short day2_downpower_col;      //
	short day3_downpower_col;      //
	short day4_downpower_col;      //
	short day5_downpower_col;      //
	short day6_downpower_col;      //
	short day3_downpower_col;      //
	short day7_downpower_col;      //
	short day8_downpower_col;      //
	short day9_downpower_col;      //
	short day10_downpower_col;      //
	short day11_downpower_col;      //
	short day12_downpower_col;      //
	short day13_downpower_col;      //
	short day14_downpower_col;      //
	short day15_downpower_col;      //
	short day16_downpower_col;      //
	short day17_downpower_col;      //
	short day18_downpower_col;      //
	short day19_downpower_col;      //
	short day20_downpower_col;      //
	short day21_downpower_col;      //
	short day22_downpower_col;      //
	short day23_downpower_col;      //
	short day24_downpower_col;      //
	short day25_downpower_col;      //
	short day26_downpower_col;      //
	short day27_downpower_col;      //
	short day28_downpower_col;      //
	short day29_downpower_col;      //
	short day30_downpower_col;      //
	short day31_downpower_col;      //



public:
	int read_station_daypower_rdb();
};











//厂站表
class Cfac_info
{
public:
	Cfac_info(Cdata_access *_data_obj);//将厂站序号当成自己的显示序号来处理
	~Cfac_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID //在厂站表中使用厂站序号
	char name[128];
	int fac_id;
	short fac_no;
	char if_allow_yk; //是否允许遥控
	int fore_group; //前置组别
	short display_id_col;
	short name_col;
	short if_allow_yk_col;
	short fore_group_col;
public:
	int read_fac_rdb();
};










class Cmeas_info
{
public:
	Cmeas_info(Cdata_access *_data_obj);//将厂站序号当成自己的显示序号来处理
	~Cmeas_info();
	Cdata_access *data_obj;
	RDB_NET      *rdb_obj;       //实时库对象
	system_net_info *dnet_obj;   //网络操作对象
public:
	int table_id;
	int record_id;      //在本设备表中的记录号
	int display_id;     //在本设备表中的显示ID //在厂站表中使用厂站序号
	char name[128];
	int meas_id;
	int meas_value;
	short meas_id_col;
	short display_id_col;
	short name_col;
	short meas_value_col;
public:
	int read_meas_rdb();
};
#endif // DEVICE_H
