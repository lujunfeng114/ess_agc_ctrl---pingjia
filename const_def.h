#ifndef CONST_DEF_H
#define CONST_DEF_H
#include <signal.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <list>

#include "dnet_all.h"
#include "rdb_all.h"
//#include "rdb_common_func_head.h"

#include "../../com/graph/include/rdb_common_func_head.h"
#include "security_all.h"
#include "config_all.h"
//#include "scada_report_manager.h"
#include "../../scada/scada_normal/scada_report_manager.h"
using namespace std;
#define PCS_CTRL_MODE_REMOTE    1   //远程模式
#define PCS_CTRL_MODE_LOCAL     0   //就地模式


#define DEV_LIST_RELOAD_INTERVAL 60*10  //10分钟重读配置

//调度交互约定定义
//定义调度下发的消息类型 0 无效类型 1 agvc 2 转发
#define DISPATCH_MSG_TYPE_INVALID   0
#define DISPATCH_MSG_TYPE_AGVC      1
#define DISPATCH_MSG_TYPE_FORWOD    2
//定义调度动作类型
#define ESS_AGVC_STATUS_ENTER   1
#define ESS_AGVC_STATUS_QUIT    0
//远方就地信号类型
#define ESS_AGVC_STATUS_REMOTE  1
#define ESS_AGVC_STATUS_LOCAL   0
//是否允许调度agvc控制
#define ESS_AGC_ALLOWED 1
#define ESS_AGC_DENIED  0
#define ESS_AVC_ALLOWED 1
#define ESS_AVC_DENIED  0

//定义设备记录个数,该个数必须在现实序号中连续
#define PCS_LIST_NUM 5
#define BMS_LIST_NUM 5
#define FAC_LIST_NUM 20
#define STATION_LIST_NUM 1
#define POINT_LIST_NUM 1
#define AGVC_LIST_NUM 1
#define MICRO_CTROL_NUM 100 ///!!!需根据现场实际情况调整


//转发给调度约定的电站运行状态
#define PLANT_RUN_STATUS_STOP   1 //电站停机
#define PLANT_RUN_STATUS_STANDBY    2   //待机
#define PLANT_RUN_STATUS_CHARGE 3
#define PLANT_RUN_STATUS_DISCHARGE  4

//定义储能电站信息表中的电站投运状态status字段类型
#define STATION_STATUS_NONE         0
#define STATION_STATUS_PLAN_CURVE   2
#define STATION_STATUS_SET_VAL      3
#define sTATION_STATUS_STOP         4

//前置规约定义
//控制命令公共描述头长度
#define CTRL_CMD_FIRM_HEAD_LEN		(sizeof(int)+sizeof(short)*3+sizeof(int))
#define CTRL_CMD_DEFINE_INFO_LEN    (sizeof(char)*3 +sizeof(int)+sizeof(short)*2+sizeof(char))
//命令类型定义：
#define FORE_CMD_TYPE_YK 1   //1——遥控
#define FORE_CMD_TYPE_YT 2   //2——遥调（暂时没有用）
#define FORE_CMD_TYPE_INT    3   //3——整形设定
#define FORE_CMD_TYPE_FLOAT  4   //4——浮点设定

//阶段定义：
#define FORE_CMD_STEP_SELECT 1   //1——选择
#define FORE_CMD_STEP_CANCLE 2   //2——取消
#define FORE_CMD_STEP_EXEC   3   //3——执行
#define FORE_CMD_STEP_DERECT 4   //4——直接控制

//控合 控分
#define FORE_CMD_YK_ON 2    //控合
#define FORE_CMD_YK_OFF 1   //控分

//源网荷dnet消息类型
#define DNET_SCADA_YUAN_WANG_HE  20001

//微电网控制定义表结构
typedef struct
{
	ON_KEY_INT64 yk_id;    //遥控ID号
	char yk_alias[128];  //别名
	int fac_id; //转发场
	short send_no;  //转发号
	char allow_ctrl_dest;   //允许控制状态
	short select_wait_time;   //预置超时时间
	short select_keep_time; //预置保持时间

	//time_t cmd_gen_time;    //在控制命令缓存中的时长,用于判定命令超时用

}yk_send_struct;
typedef struct
{
	ON_KEY_INT64 yc_id;
	char yc_alias[128];  //别名
	int fac_id; //转发场
	short send_no;  //转发号
	float max_eng_val;   //归一化系数
	float eng_factor;   //工程值系数
	short select_wait_time;   //预置超时时间
	short select_keep_time; //预置保持时间

	//time_t cmd_gen_time;    //在控制命令缓存中的时长,用于判定命令超时用
}yt_send_struct;
typedef struct
{
	char cmd_type;	//命令类型
	char asdu_type;	//Asdu类型
	char cmd_step;	//阶段
	int fac_id; 	//厂站ID
	short addr_no;	//公共地址
	short send_no;	//转发点号
	char dest_val_char;	//控制目标
	short dest_val_short;
	float dest_val_float;
	char gen_host[128];
	char gen_proc[128];
	ON_KEY_INT64 ctrl_obj_id;//遥控或遥调转发表中的key
	int table_id;
	int record_id;
	time_t cmd_gen_time; //在控制命令缓存中的时长
}ctrl_cmd_struct;

typedef struct
{
	int sw_id;
	char yk_alias[128];
	int fac_id;
	short yk_public_addr;
	int yk_no;
	short wf_sw_num;
	ON_KEY_INT64 swx_id[8];
	char close_swx_yx_value[8];
	char open_swx_yx_value[8];
	char if_allow_oper;
	char if_wf_lock;


}yk_define_struct;

typedef struct
{
	int sw_id;
	int yx_value;
}sw_info_struct;


#endif // CONST_DEF_H
