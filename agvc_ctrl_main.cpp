#include "agvc_ctrl_main.h"
//#include <windows.h>
#include<time.h>
#include "../../scada/scada_normal/scada_report_manager.h"



//PCS设值模式
const int HengGongLv_charge = 1;
const int HengGongLV_out = 2;
const int HengDianYa_charge = 3;
const int WuGongSet = 4;
const int VF_DianYa =5;
const int VF_PinLv = 6;

//PCS使能模式
const int Charge_command =1;
const int Out_command =2;
const int VF_command =3;
const int DianYa_command= 4;


//控制指令
const int GRID_STOP = 1 ; //停机
const int GRID_ON = 2;  //并网
const int GRID_OFF = 3;  //离网


//常量
const int PV_DC = 400;
const int PCS_DC_LOW=500;
const int PCS_DC_High=700;
const int socStop = 20;  //SOC低于多少时让离网

const int NO_BUSBAR_VOLTAGE = 20;  //电网电压小于多少判断有问题
const int GRID_VOLTAGE_MIN = 180;
const int GRID_VOLTAGE_MAX = 250;


//并网功率平衡
const int SOC_MAX = 90 ;  
const int SOC_MIN = 20 ;

//选择那一块电量表
const int TOTAL_POWER_METER = 1 ;  //总关口表
const int STATION_TRANSFORMER_METER = 2 ;  //站用变表

int  timecount=0;

int Mode = 0;


Cagvc_ctrl_mgr::Cagvc_ctrl_mgr()
{
	dnet_instance.set_system_net_info("ess_agvc_ctrl", DNET_NO);
	rdb_instance.set_dnet_object(dnet_instance);

	dnet_obj = &dnet_instance;
	rdb_obj = &rdb_instance;

	scada_report = new scada_report_manager;
	scada_report->init_modify_rdb_and_alarm_report_manager(dnet_obj,rdb_obj);


	data_obj = new Cdata_access(rdb_obj, dnet_obj, scada_report);
	dnet_obj->set_write_log_level(-1, 0);
	dnet_obj->set_write_log_interval(-1, 0);
	//dnet_obj->write_log_at_once(0, 1000, "Cagvc_ctrl_mgr:dnet_obj=%x, rdb_obj=%x, data_obj=%x",
	//                          dnet_obj, rdb_obj, data_obj);

	dnet_recv_buf = (char*)malloc(128);
}

Cagvc_ctrl_mgr::~Cagvc_ctrl_mgr()
{
	device_list_clear();
	delete data_obj;
	free(dnet_recv_buf);
	delete scada_report;
}

int Cagvc_ctrl_mgr::read_fac_info_table()
{
	for(int i=0; i<fac_list.size(); i++)
	{
		delete fac_list.at(i);
	}
	fac_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 5;
	buffer = (char *)MALLOC(1000);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);

	char  English_names[5][DB_ENG_TABLE_NAME_LEN] = {
		"fac_no",
		"fac_id",
		"fac_name",
		"if_allow_yk",
		"fore_group"
	};

	//根据设备表名称读取设备表ID
	int table_id = -1;
	retcode = rdb_obj->get_table_id_by_table_name("fac_info",table_id);    
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取厂站信息表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取厂站信息表表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "厂站信息表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cfac_info *fac = new Cfac_info(data_obj);        
		fac->table_id = table_id;

		memcpy((char *)&fac->fac_no, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		// dnet_obj->write_log_at_once(0, 1000, "厂站信息表%d", fac->fac_no);
		memcpy((char *)&fac->fac_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		// dnet_obj->write_log_at_once(0, 1000, "厂站信息表%d", fac->fac_id);
		memcpy((char *)&fac->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		//dnet_obj->write_log_at_once(0, 1000, "厂站信息表%d", fac->fac_id);
		memcpy((char *)&fac->if_allow_yk, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		//dnet_obj->write_log_at_once(0, 1000, "厂站信息表%d", fac->if_allow_yk);
		memcpy((char *)&fac->fore_group, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		//dnet_obj->write_log_at_once(0, 1000, "厂站信息表%d", fac->fore_group);
		fac_list.push_back(fac);
	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);

	return 1;
}

int Cagvc_ctrl_mgr::read_meas_info_table()
{
	for(int i=0; i<meas_list.size(); i++)
	{
		delete meas_list.at(i);
	}
	meas_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 3;
	buffer = (char *)MALLOC(1000);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);

	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"meas_id",
		"meas_name",
		"meas_value"
	};

	//根据设备表名称读取设备表ID
	int table_id = -1;
	retcode = rdb_obj->get_table_id_by_table_name("meas_info",table_id);    
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取量测信息表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取量测信息表表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "量测信息表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cmeas_info *meas = new Cmeas_info(data_obj);   
		meas->table_id = table_id;

		memcpy((char *)&meas->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;

		memcpy((char *)&meas->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;

		memcpy((char *)&meas->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;

		memcpy((char *)&meas->meas_value, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;


		meas->display_id_col = fields_info[0].rdb_field_no;
		meas->name_col = fields_info[2].rdb_field_no;
		meas->meas_value_col = fields_info[3].rdb_field_no;

		meas_list.push_back(meas);
	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);

	return 1;
}


int Cagvc_ctrl_mgr::read_bms_info_table()
{
	for(int i=0; i<bms_list.size(); i++)
	{
		delete bms_list.at(i);
	}
	bms_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 9;
	buffer = (char *)MALLOC(1000);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);


	char  English_names[9][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"soc_min_disc",
		"soc_max_char",
		"soc",
		"cap_r",
		"ava_char",
		"ava_disc"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("d_plant_stack", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取储能电池堆表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取储能电池堆表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "'储能电池堆信息表没有记录！");
		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;
		//dnet_obj->write_log_at_once(11, 1000, "read_bms_info_table: data_obj1=%x", data_obj);
		Cbms_info *bms = new Cbms_info(data_obj);

		//        dnet_obj->write_log_at_once(11, 1000, "read_bms_info_table: read_bms_rdb:dnet_obj=%x, rdb_obj=%x, data_obj=%x, bms_new=%x",
		//                                    bms->dnet_obj, bms->rdb_obj, bms->data_obj, bms);
		bms->table_id = table_id;

		memcpy((char *)&(bms->display_id), buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&bms->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&bms->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d name=%s", i, bms->name);
		memcpy((char *)&bms->soc_min, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d soc_min=%f", i, bms->soc_min);
		memcpy((char *)&bms->soc_max, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d soc_max=%f", i, bms->soc_max);
		memcpy((char *)&bms->soc_now, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d soc_now=%f", i, bms->soc_now);
		memcpy((char *)&bms->kwh, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d kwh=%f", i, bms->kwh);
		memcpy((char *)&bms->ava_char, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d ava_char=%f", i, bms->ava_char);
		memcpy((char *)&bms->ava_disc, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		//dnet_obj->write_log(0,5199,"read_bms_info_table: i=%d ava_disc=%f", i, bms->ava_disc);
		bms->display_id_col = fields_info[0].rdb_field_no;
		bms->name_col = fields_info[2].rdb_field_no;
		bms->soc_min_col = fields_info[3].rdb_field_no;
		bms->soc_max_col = fields_info[4].rdb_field_no;
		bms->soc_now_col = fields_info[5].rdb_field_no;
		bms->kwh_col = fields_info[6].rdb_field_no;
		bms->ava_char_col = fields_info[7].rdb_field_no;
		bms->ava_dischar_col = fields_info[8].rdb_field_no;

		bms_list.push_back(bms);
	}
	//    dnet_obj->write_log_at_once(11, 1000,
	//                                "read_bms_info_table:++++size= %d", bms_list.size());
	//    for(int i=0;i<bms_list.size();i++)
	//    {
	//        Cbms_info *bms1 = bms_list.at(i);
	//        dnet_obj->write_log_at_once(11, 1000, "read_bms_info_table:---> dnet_obj=%x, rdb_obj=%x, data_obj=%x, bms_new=%x",
	//                                    bms1->dnet_obj, bms1->rdb_obj, bms1->data_obj, bms1);
	//    }

	FREE((char *&)buffer);
	FREE((char *&)fields_info);

	return 1;
}

int Cagvc_ctrl_mgr::read_pcs_info_table()
{
	for(int i=0; i<pcs_list.size(); i++)
	{
		delete pcs_list.at(i);
	}
	pcs_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 27;

	buffer = (char *)MALLOC(5000);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);

	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"ac_p",
		"ac_q",
		"p_max_char",
		"p_max_disc",
		"if_use",
		"if_fault",
		"ctrl_mode",
		"run_mode",
		"ctrl_condition",
		"on_off_stat",
		"on_off_grid_stat",
		"standby_stat",
		"q_max_char",
		"q_max_disc",
		"capacity",
		"alarm_level_1",
		"alarm_level_2",
		"alarm_level_3",
		"alarm_level_1_act",
		"alarm_level_2_act",
		"alarm_level_3_act",
		"ac_ua",
		"ac_ub",
		"ac_uc"

	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("d_plant_pcs", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取变流器表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		rdb_obj->write_log_at_once(0, 1000, "读取变流器表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "变流器表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;
		Cpcs_info *pcs = new Cpcs_info(data_obj);
		pcs->table_id = table_id;

		memcpy((char *)&pcs->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&pcs->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&pcs->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&pcs->p, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&pcs->q, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&pcs->p_max_char, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&pcs->p_max_disc, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&pcs->if_use, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&pcs->fault_total, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&pcs->ctrl_mode, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&pcs->run_mode, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&pcs->ctrl_condition, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&pcs->on_off_stat, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&pcs->on_off_grid_stat, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&pcs->standby_stat, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&pcs->q_max_char, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&pcs->q_max_disc, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&pcs->capacity, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;

		memcpy((char *)&pcs->alarm_level_1, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;
		memcpy((char *)&pcs->alarm_level_2, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&pcs->alarm_level_3, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&pcs->alarm_level_1_act, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&pcs->alarm_level_2_act, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&pcs->alarm_level_3_act, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&pcs->ac_ua, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&pcs->ac_ub, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&pcs->ac_uc, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;

		pcs->display_id_col = fields_info[0].rdb_field_no;
		pcs->name_col = fields_info[2].rdb_field_no;
		pcs->p_col = fields_info[3].rdb_field_no;
		pcs->q_col = fields_info[4].rdb_field_no;
		pcs->p_max_char_col = fields_info[5].rdb_field_no;
		pcs->p_max_disc_col = fields_info[6].rdb_field_no;
		pcs->if_use_col = fields_info[7].rdb_field_no;
		pcs->fault_total_col = fields_info[8].rdb_field_no;
		pcs->ctrl_mode_col = fields_info[9].rdb_field_no;
		pcs->run_mode_col = fields_info[10].rdb_field_no;
		pcs->ctrl_condition_col = fields_info[11].rdb_field_no;
		pcs->on_off_stat_col = fields_info[12].rdb_field_no;
		pcs->on_off_grid_stat_col = fields_info[13].rdb_field_no;
		pcs->standby_stat_col = fields_info[14].rdb_field_no;
		pcs->q_max_char_col = fields_info[15].rdb_field_no;
		pcs->q_max_disc_col = fields_info[16].rdb_field_no;
		pcs->capacity_col = fields_info[17].rdb_field_no;

		pcs->alarm_level_1_col = fields_info[18].rdb_field_no;
		pcs->alarm_level_2_col = fields_info[19].rdb_field_no;
		pcs->alarm_level_3_col = fields_info[20].rdb_field_no;
		pcs->alarm_level_1_act_col = fields_info[21].rdb_field_no;
		pcs->alarm_level_2_act_col = fields_info[22].rdb_field_no;
		pcs->alarm_level_3_act_col = fields_info[23].rdb_field_no;
		pcs->ac_ua_col = fields_info[24].rdb_field_no;
		pcs->ac_ub_col = fields_info[25].rdb_field_no;
		pcs->ac_uc_col = fields_info[26].rdb_field_no;

		pcs->bms = NULL;
		pcs->yt_p_chong_micro_ctrl = NULL;
		pcs->yk_standby_micro_ctrl = NULL;
		pcs->yt_q_micro_ctrl = NULL;
		pcs->yk_poweron_micro_ctrl = NULL;
		pcs->emergency_reset_micro_ctrl = NULL;

		pcs->yt_p_fang_micro_ctrl= NULL;  //pcs有功调节对应的微电网控制定义记录
		pcs->yt_voltage_micro_ctrl= NULL;  //pcs恒压浮充  
		pcs->yt_vf_ac_micro_ctrl= NULL;  //pcs离网电压
		pcs->yt_vf_hz_micro_ctrl= NULL;  //pcs离网频率

		pcs->yt_dc_kailu_micro_ctrl= NULL;  //开路电压  
		pcs->yt_a_kailu_micro_ctrl= NULL;  //开路电流
		pcs->yt_1_micro_ctrl= NULL;  //系数1
		pcs->yt_2_micro_ctrl= NULL;  //系数2
		pcs->yt_3_micro_ctrl= NULL;  //系数3
		pcs->yt_4_micro_ctrl= NULL;  //系数4

		//卢俊峰 20190307 青禾 新增
		pcs->yt_pz_micro_ctrl= NULL;  // 作为PZ的遥调控制开关  4096合 8192分
		pcs->yt_aemfen_micro_ctrl= NULL;  //作为AEM表的遥调控制开关 分
		pcs->yt_aemhe_micro_ctrl= NULL;  //作为AEM表的遥调控制开关 合
		pcs->yt_sunon_micro_ctrl= NULL;  //作为sun光伏逆变器的开机
		pcs->yt_sunoff_micro_ctrl= NULL;  //作为sun光伏逆变器的关机


		pcs->yk_p_chong_micro_ctrl= NULL;  //pcs有功调节对应的微电网控制定义记录
		pcs->yk_p_fang_micro_ctrl= NULL;  //pcs有功调节对应的微电网控制定义记录
		pcs->yk_poweroff_micro_ctrl= NULL;	//pcs停机对应的微电网控制定义记录
		pcs->yk_vf_micro_ctrl= NULL;		//pcs vf使能
		pcs->yk_voltage_micro_ctrl= NULL;	//pcs 恒压浮充使能
		pcs->yk_failure_micro_ctrl= NULL;	//pcs 故障复归


		//卢俊峰 20190307 青禾 新增
		pcs->yk_offnet_micro_ctrl= NULL;	// 主动离网   //用于斯菲尔表
		pcs->yk_tongqi_micro_ctrl= NULL;	// 主动离网   //用于斯菲尔表


		pcs_list.push_back(pcs);
	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);

	return 1;
}

int Cagvc_ctrl_mgr::read_station_info_table()
{
	for(int i=0; i<station_list.size(); i++)
	{
		delete station_list.at(i);
	}
	station_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 23;
	buffer = (char *)MALLOC(2400);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);

	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"cap_r",
		"p_r",
		"run_bay_num",
		"stop_bay_num",
		"plt_run_status",
		"ava_kwh_disc",
		"ava_kwh_char",
		"soc_total",
		"soc_up",
		"soc_low",
		"plt_run_mod",
		"latitude",
		"longtitude",
		"status",
		"bay_num",
		"bats_num",
		"pcs_num",
		"bms_num",
		"day_ongrid_energy",
		"day_downgrid_energy"	

	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("d_ess_plant", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "储能电站表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "储能电站表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "储能电站表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;
		Cstation_info *station = new Cstation_info(data_obj);
		station->table_id = table_id;

		memcpy((char *)&station->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&station->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&station->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&station->cap_r, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&station->p_r, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&station->run_bay_num, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&station->stop_bay_num, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&station->plt_run_status, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&station->ava_kwh_disc, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&station->ava_kwh_char, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&station->soc_total, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&station->soc_up, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&station->soc_low, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;

		memcpy((char *)&station->plt_run_mod, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&station->latitude, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&station->longtitude, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&station->status, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;

		memcpy((char *)&station->bay_num, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;
		memcpy((char *)&station->bats_num, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;
		memcpy((char *)&station->pcs_num, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&station->bms_num, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;

		memcpy((char *)&station->day_ongrid_energy, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&station->day_downgrid_energy, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;




		station->display_id_col = fields_info[0].rdb_field_no;
		station->name_col = fields_info[2].rdb_field_no;
		station->cap_r_col = fields_info[3].rdb_field_no;
		station->p_r_col = fields_info[4].rdb_field_no;
		station->run_bay_num_col = fields_info[5].rdb_field_no;
		station->stop_bay_num_col = fields_info[6].rdb_field_no;
		station->plt_run_status_col = fields_info[7].rdb_field_no;
		station->ava_kwh_disc_col = fields_info[8].rdb_field_no;
		station->ava_kwh_char_col = fields_info[9].rdb_field_no;
		station->soc_total_col = fields_info[10].rdb_field_no;
		station->soc_up_col = fields_info[11].rdb_field_no;
		station->soc_low_col = fields_info[12].rdb_field_no;

		station->plt_run_mod_col = fields_info[13].rdb_field_no;
		station->latitude_col = fields_info[14].rdb_field_no;
		station->longtitude_col = fields_info[15].rdb_field_no;
		station->status_col = fields_info[16].rdb_field_no;
		station->bay_num_col = fields_info[17].rdb_field_no;
		station->bats_num_col = fields_info[18].rdb_field_no;
		station->pcs_num_col = fields_info[19].rdb_field_no;
		station->bms_num_col = fields_info[20].rdb_field_no;

		station->day_ongrid_energy_col = fields_info[21].rdb_field_no;
		station->day_downgrid_energy_col = fields_info[22].rdb_field_no;

		station_list.push_back(station);

	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}

int Cagvc_ctrl_mgr::read_point_info_table()
{
	for(int i=0; i<point_list.size(); i++)
	{
		delete point_list.at(i);
	}
	point_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;


	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;
	const int field_num = 10;
	buffer = (char *)MALLOC(1000);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);

	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"power",
		"re_power",
		"cos1",
		"p_kwh",
		"pn_kwh",
		"q_kvarh",
		"qn_kwarh"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("d_plant_point", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取并网点表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取并网点表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "并网点表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}



	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;
		Cpoint_info *point = new Cpoint_info(data_obj);
		point->table_id = table_id;

		memcpy((char *)&point->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&point->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&point->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&point->p, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&point->q, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&point->cos, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&point->p_kwh, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&point->pn_kwh, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&point->q_kvarh, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&point->qn_kvarh, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;

		point->display_id_col = fields_info[0].rdb_field_no;
		point->name_col = fields_info[2].rdb_field_no;
		point->p_col = fields_info[3].rdb_field_no;
		point->q_col = fields_info[4].rdb_field_no;
		point->cos_col = fields_info[5].rdb_field_no;
		point->p_kwh_col = fields_info[6].rdb_field_no;
		point->pn_kwh_col = fields_info[7].rdb_field_no;
		point->q_kvarh_col = fields_info[8].rdb_field_no;
		point->qn_kvarh_col = fields_info[9].rdb_field_no;

		//        for(int i=0;i<field_num;i++)
		//        {
		//            dnet_obj->write_log(0,5199,"read_point_info_table:%i field_no=%d", i, fields_info[i].rdb_field_no);
		//        }
		point_list.push_back(point);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}

int Cagvc_ctrl_mgr::read_agvc_info_table()
{
	for(int i=0; i<agvc_list.size(); i++)
	{
		delete agvc_list.at(i);
	}
	agvc_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 37;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"total_fault",
		"char_done",
		"dischar_done",
		"if_agc_ctrl",
		"agc_remote_mode",
		"char_lock",
		"dischar_lock",
		"ask_agc_on",
		"ask_agc_status",
		"agc_p_dest",
		"agc_p_return",
		"p_max_char",
		"p_max_dischar",
		"p_max_dischar_time",
		"p_max_char_time",
		"dead_value",
		"ctrl_interval",
		"if_agv_ctrl",
		"ask_avc_on",
		"ask_avc_status",
		"avc_remote_mode",
		"add_q_lock",
		"reduce_q_lock",
		"avc_u_q_ctrl_mode",
		"avc_add_q",
		"avc_reduce_q",
		"avc_u_dest",
		"avc_u_ref",
		"avc_q_dest",
		"avc_q_ref",
		"agc_chanel_stat",
		"avc_chanel_stat",
		"agc_last_modify_time",
		"avc_last_modify_time"
	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("d_plant_agvc", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取电站AGVC表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取电站AGVC表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "电站AGVC表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cagvc_info *agvc = new Cagvc_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->total_fault, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->char_done, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->dischar_done, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->if_agc_ctrl, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->agc_remote_mode, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;

		memcpy((char *)&agvc->char_lock, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->dischar_lock, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->ask_agc_on, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->ask_agc_status, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->agc_p_dest, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;

		memcpy((char *)&agvc->agc_p_return, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->p_max_char, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->p_max_dischar, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->p_max_dischar_time, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&agvc->p_max_char_time, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;
		memcpy((char *)&agvc->dead_value, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->ctrl_interval, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;

		memcpy((char *)&agvc->if_agv_ctrl, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->ask_avc_on, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&agvc->ask_avc_status, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->avc_remote_mode, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->add_q_lock, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->reduce_q_lock, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->avc_u_q_ctrl_mode, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->avc_add_q, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->avc_reduce_q, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->avc_u_dest, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;
		memcpy((char *)&agvc->avc_u_ref, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;
		memcpy((char *)&agvc->avc_q_dest, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;
		memcpy((char *)&agvc->avc_q_ref, buffer+record_pos+offset, fields_info[32].field_len);
		offset += fields_info[32].field_len;

		memcpy((char *)&agvc->agc_chanel_stat, buffer+record_pos+offset, fields_info[33].field_len);
		offset += fields_info[33].field_len;
		memcpy((char *)&agvc->avc_chanel_stat, buffer+record_pos+offset, fields_info[34].field_len);
		offset += fields_info[34].field_len;
		memcpy((char *)&agvc->agc_last_modify_time, buffer+record_pos+offset, fields_info[35].field_len);
		offset += fields_info[35].field_len;
		memcpy((char *)&agvc->avc_last_modify_time, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;

		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->total_fault_col = fields_info[3].rdb_field_no;
		agvc->char_done_col = fields_info[4].rdb_field_no;
		agvc->dischar_done_col = fields_info[5].rdb_field_no;
		agvc->if_agc_ctrl_col = fields_info[6].rdb_field_no;
		agvc->agc_remote_mode_col = fields_info[7].rdb_field_no;
		agvc->char_lock_col = fields_info[8].rdb_field_no;
		agvc->dischar_lock_col = fields_info[9].rdb_field_no;
		agvc->ask_agc_on_col = fields_info[10].rdb_field_no;
		agvc->ask_agc_status_col = fields_info[11].rdb_field_no;
		agvc->agc_p_dest_col = fields_info[12].rdb_field_no;
		agvc->agc_p_return_col = fields_info[13].rdb_field_no;
		agvc->p_max_char_col = fields_info[14].rdb_field_no;
		agvc->p_max_dischar_col = fields_info[15].rdb_field_no;
		agvc->p_max_dischar_time_col = fields_info[16].rdb_field_no;
		agvc->p_max_char_time_col = fields_info[17].rdb_field_no;
		agvc->dead_value_col = fields_info[18].rdb_field_no;
		agvc->ctrl_interval_col = fields_info[19].rdb_field_no;


		agvc->if_agv_ctrl_col = fields_info[20].rdb_field_no;
		agvc->ask_avc_on_col = fields_info[21].rdb_field_no;
		agvc->ask_avc_status_col = fields_info[22].rdb_field_no;
		agvc->avc_remote_mode_col = fields_info[23].rdb_field_no;
		agvc->add_q_lock_col = fields_info[24].rdb_field_no;
		agvc->reduce_q_lock_col = fields_info[25].rdb_field_no;
		agvc->avc_u_q_ctrl_mode_col = fields_info[26].rdb_field_no;
		agvc->avc_add_q_col = fields_info[27].rdb_field_no;
		agvc->avc_reduce_q_col = fields_info[28].rdb_field_no;
		agvc->avc_u_dest_col = fields_info[29].rdb_field_no;
		agvc->avc_u_ref_col = fields_info[30].rdb_field_no;
		agvc->avc_q_dest_col = fields_info[31].rdb_field_no;
		agvc->avc_q_ref_col = fields_info[32].rdb_field_no;

		agvc->agc_chanel_stat_col = fields_info[33].rdb_field_no;
		agvc->avc_chanel_stat_col = fields_info[34].rdb_field_no;
		agvc->agc_last_modify_time_col = fields_info[35].rdb_field_no;
		agvc->avc_last_modify_time_col = fields_info[36].rdb_field_no;
		agvc_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}


int Cagvc_ctrl_mgr::read_gatepower_info_table()
{
	for(int i=0; i<gatepower_list.size(); i++)
	{
		delete gatepower_list.at(i);
	}
	gatepower_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 94;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"month",
		"thismonth_uppower",
		"last1_uppower",
		"last2_uppower",
		"last3_uppower",
		"last4_uppower",
		"last5_uppower",
		"last6_uppower",
		"last7_uppower",
		"last8_uppower",
		"last9_uppower",
		"last10_uppower",
		"last11_uppower",
		"last12_uppower",
		"thismonth_downpower",
		"last1_downpower",
		"last2_downpower",
		"last3_downpower",
		"last4_downpower",
		"last5_downpower",
		"last6_downpower",
		"last7_downpower",
		"last8_downpower",
		"last9_downpower",
		"last10_downpower",
		"last11_downpower",
		"last12_downpower",
		"today_uppower",   //当日上网电量
		"day1_uppower",    //昨日上网电量
		"day2_uppower",    //前日上网电量
		"day3_uppower",   //上3日上网电量
		"day4_uppower",    //
		"day5_uppower",    //
		"day6_uppower",    //
		"day7_uppower",    //
		"day8_uppower",     //
		"day9_uppower",    //
		"day10_uppower",    //
		"day11_uppower",     //
		"day12_uppower",     //
		"day13_uppower",     //
		"day14_uppower",     //
		"day15_uppower",     //
		"day16_uppower",     //
		"day17_uppower",     //
		"day18_uppower",     //
		"day19_uppower",     //
		"day20_uppower",     //
		"day21_uppower",     //
		"day22_uppower",     //
		"day23_uppower",     //
		"day24_uppower",     //
		"day25_uppower",     //
		"day26_uppower",     //
		"day27_uppower",     //
		"day28_uppower",     //
		"day29_uppower",     //
		"day30_uppower",     //
		"day31_uppower",     //
		"today_downpower",   //
		"day1_downpower",    //
		"day2_downpower",    //
		"day3_downpower",   //
		"day4_downpower",    //
		"day5_downpower",    //
		"day6_downpower",    //
		"day7_downpower",    //
		"day8_downpower",     //
		"day9_downpower",    //
		"day10_downpower",    //
		"day11_downpower",     //
		"day12_downpower",     //
		"day13_downpower",     //
		"day14_downpower",     //
		"day15_downpower",     //
		"day16_downpower",     //
		"day17_downpower",     //
		"day18_downpower",     //
		"day19_downpower",     //
		"day20_downpower",     //
		"day21_downpower",     //
		"day22_downpower",     //
		"day23_downpower",     //
		"day24_downpower",     //
		"day25_downpower",     //
		"day26_downpower",     //
		"day27_downpower",     //
		"day28_downpower",     //
		"day29_downpower",     //
		"day30_downpower",     //
		"day31_downpower"   //

	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("pj_gatepower_info", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取关口电量表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取关口电量表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "关口电量表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cgatepower_info *agvc = new Cgatepower_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->month, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->thismonth_uppower, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->last1_uppower, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->last2_uppower, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->last3_uppower, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&agvc->last4_uppower, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->last5_uppower, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->last6_uppower, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->last7_uppower, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->last8_uppower, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&agvc->last9_uppower, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->last10_uppower, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->last11_uppower, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->last12_uppower, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;

		memcpy((char *)&agvc->thismonth_downpower, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;
		memcpy((char *)&agvc->last1_downpower, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->last2_downpower, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&agvc->last3_downpower, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->last4_downpower, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&agvc->last5_downpower, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->last6_downpower, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->last7_downpower, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->last8_downpower, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->last9_downpower, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->last10_downpower, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->last11_downpower, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->last12_downpower, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;

		memcpy((char *)&agvc->today_uppower, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;
		memcpy((char *)&agvc->day1_uppower, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;
		memcpy((char *)&agvc->day2_uppower, buffer+record_pos+offset, fields_info[32].field_len);
		offset += fields_info[32].field_len;
		memcpy((char *)&agvc->day3_uppower, buffer+record_pos+offset, fields_info[33].field_len);
		offset += fields_info[33].field_len;
		memcpy((char *)&agvc->day4_uppower, buffer+record_pos+offset, fields_info[34].field_len);
		offset += fields_info[34].field_len;
		memcpy((char *)&agvc->day5_uppower, buffer+record_pos+offset, fields_info[35].field_len);
		offset += fields_info[35].field_len;
		memcpy((char *)&agvc->day6_uppower, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;
		memcpy((char *)&agvc->day7_uppower, buffer+record_pos+offset, fields_info[37].field_len);
		offset += fields_info[37].field_len;
		memcpy((char *)&agvc->day8_uppower, buffer+record_pos+offset, fields_info[38].field_len);
		offset += fields_info[38].field_len;
		memcpy((char *)&agvc->day9_uppower, buffer+record_pos+offset, fields_info[39].field_len);
		offset += fields_info[39].field_len;
		memcpy((char *)&agvc->day10_uppower, buffer+record_pos+offset, fields_info[40].field_len);
		offset += fields_info[40].field_len;
		memcpy((char *)&agvc->day11_uppower, buffer+record_pos+offset, fields_info[41].field_len);
		offset += fields_info[41].field_len;
		memcpy((char *)&agvc->day12_uppower, buffer+record_pos+offset, fields_info[42].field_len);
		offset += fields_info[42].field_len;
		memcpy((char *)&agvc->day13_uppower, buffer+record_pos+offset, fields_info[43].field_len);
		offset += fields_info[43].field_len;
		memcpy((char *)&agvc->day14_uppower, buffer+record_pos+offset, fields_info[44].field_len);
		offset += fields_info[44].field_len;
		memcpy((char *)&agvc->day15_uppower, buffer+record_pos+offset, fields_info[45].field_len);
		offset += fields_info[45].field_len;
		memcpy((char *)&agvc->day16_uppower, buffer+record_pos+offset, fields_info[46].field_len);
		offset += fields_info[46].field_len;
		memcpy((char *)&agvc->day17_uppower, buffer+record_pos+offset, fields_info[47].field_len);
		offset += fields_info[47].field_len;
		memcpy((char *)&agvc->day18_uppower, buffer+record_pos+offset, fields_info[48].field_len);
		offset += fields_info[48].field_len;
		memcpy((char *)&agvc->day19_uppower, buffer+record_pos+offset, fields_info[49].field_len);
		offset += fields_info[49].field_len;
		memcpy((char *)&agvc->day20_uppower, buffer+record_pos+offset, fields_info[50].field_len);
		offset += fields_info[50].field_len;
		memcpy((char *)&agvc->day21_uppower, buffer+record_pos+offset, fields_info[51].field_len);
		offset += fields_info[51].field_len;
		memcpy((char *)&agvc->day22_uppower, buffer+record_pos+offset, fields_info[52].field_len);
		offset += fields_info[52].field_len;
		memcpy((char *)&agvc->day23_uppower, buffer+record_pos+offset, fields_info[53].field_len);
		offset += fields_info[53].field_len;
		memcpy((char *)&agvc->day24_uppower, buffer+record_pos+offset, fields_info[54].field_len);
		offset += fields_info[54].field_len;
		memcpy((char *)&agvc->day25_uppower, buffer+record_pos+offset, fields_info[55].field_len);
		offset += fields_info[55].field_len;
		memcpy((char *)&agvc->day26_uppower, buffer+record_pos+offset, fields_info[56].field_len);
		offset += fields_info[56].field_len;
		memcpy((char *)&agvc->day27_uppower, buffer+record_pos+offset, fields_info[57].field_len);
		offset += fields_info[57].field_len;
		memcpy((char *)&agvc->day28_uppower, buffer+record_pos+offset, fields_info[58].field_len);
		offset += fields_info[58].field_len;
		memcpy((char *)&agvc->day29_uppower, buffer+record_pos+offset, fields_info[59].field_len);
		offset += fields_info[59].field_len;
		memcpy((char *)&agvc->day30_uppower, buffer+record_pos+offset, fields_info[60].field_len);
		offset += fields_info[60].field_len;
		memcpy((char *)&agvc->day31_uppower, buffer+record_pos+offset, fields_info[61].field_len);
		offset += fields_info[61].field_len;

		memcpy((char *)&agvc->today_downpower, buffer+record_pos+offset, fields_info[62].field_len);
		offset += fields_info[62].field_len;
		memcpy((char *)&agvc->day1_downpower, buffer+record_pos+offset, fields_info[63].field_len);
		offset += fields_info[63].field_len;
		memcpy((char *)&agvc->day2_downpower, buffer+record_pos+offset, fields_info[64].field_len);
		offset += fields_info[64].field_len;
		memcpy((char *)&agvc->day3_downpower, buffer+record_pos+offset, fields_info[65].field_len);
		offset += fields_info[65].field_len;
		memcpy((char *)&agvc->day4_downpower, buffer+record_pos+offset, fields_info[66].field_len);
		offset += fields_info[66].field_len;
		memcpy((char *)&agvc->day5_downpower, buffer+record_pos+offset, fields_info[67].field_len);
		offset += fields_info[67].field_len;
		memcpy((char *)&agvc->day6_downpower, buffer+record_pos+offset, fields_info[68].field_len);
		offset += fields_info[68].field_len;
		memcpy((char *)&agvc->day7_downpower, buffer+record_pos+offset, fields_info[69].field_len);
		offset += fields_info[69].field_len;
		memcpy((char *)&agvc->day8_downpower, buffer+record_pos+offset, fields_info[70].field_len);
		offset += fields_info[70].field_len;
		memcpy((char *)&agvc->day9_downpower, buffer+record_pos+offset, fields_info[71].field_len);
		offset += fields_info[71].field_len;
		memcpy((char *)&agvc->day10_downpower, buffer+record_pos+offset, fields_info[72].field_len);
		offset += fields_info[72].field_len;
		memcpy((char *)&agvc->day11_downpower, buffer+record_pos+offset, fields_info[73].field_len);
		offset += fields_info[73].field_len;
		memcpy((char *)&agvc->day12_downpower, buffer+record_pos+offset, fields_info[74].field_len);
		offset += fields_info[74].field_len;
		memcpy((char *)&agvc->day13_downpower, buffer+record_pos+offset, fields_info[75].field_len);
		offset += fields_info[75].field_len;
		memcpy((char *)&agvc->day14_downpower, buffer+record_pos+offset, fields_info[76].field_len);
		offset += fields_info[76].field_len;
		memcpy((char *)&agvc->day15_downpower, buffer+record_pos+offset, fields_info[77].field_len);
		offset += fields_info[77].field_len;
		memcpy((char *)&agvc->day16_downpower, buffer+record_pos+offset, fields_info[78].field_len);
		offset += fields_info[78].field_len;
		memcpy((char *)&agvc->day17_downpower, buffer+record_pos+offset, fields_info[79].field_len);
		offset += fields_info[79].field_len;
		memcpy((char *)&agvc->day18_downpower, buffer+record_pos+offset, fields_info[80].field_len);
		offset += fields_info[80].field_len;
		memcpy((char *)&agvc->day19_downpower, buffer+record_pos+offset, fields_info[81].field_len);
		offset += fields_info[81].field_len;
		memcpy((char *)&agvc->day20_downpower, buffer+record_pos+offset, fields_info[82].field_len);
		offset += fields_info[82].field_len;
		memcpy((char *)&agvc->day21_downpower, buffer+record_pos+offset, fields_info[83].field_len);
		offset += fields_info[83].field_len;
		memcpy((char *)&agvc->day22_downpower, buffer+record_pos+offset, fields_info[84].field_len);
		offset += fields_info[84].field_len;
		memcpy((char *)&agvc->day23_downpower, buffer+record_pos+offset, fields_info[85].field_len);
		offset += fields_info[85].field_len;
		memcpy((char *)&agvc->day24_downpower, buffer+record_pos+offset, fields_info[86].field_len);
		offset += fields_info[86].field_len;
		memcpy((char *)&agvc->day25_downpower, buffer+record_pos+offset, fields_info[87].field_len);
		offset += fields_info[87].field_len;
		memcpy((char *)&agvc->day26_downpower, buffer+record_pos+offset, fields_info[88].field_len);
		offset += fields_info[88].field_len;
		memcpy((char *)&agvc->day27_downpower, buffer+record_pos+offset, fields_info[89].field_len);
		offset += fields_info[89].field_len;
		memcpy((char *)&agvc->day28_downpower, buffer+record_pos+offset, fields_info[90].field_len);
		offset += fields_info[90].field_len;
		memcpy((char *)&agvc->day29_downpower, buffer+record_pos+offset, fields_info[91].field_len);
		offset += fields_info[91].field_len;
		memcpy((char *)&agvc->day30_downpower, buffer+record_pos+offset, fields_info[92].field_len);
		offset += fields_info[92].field_len;
		memcpy((char *)&agvc->day31_downpower, buffer+record_pos+offset, fields_info[93].field_len);		
		offset += fields_info[93].field_len;




		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->month_col = fields_info[3].rdb_field_no;
		agvc->thismonth_uppower_col = fields_info[4].rdb_field_no;
		agvc->last1_uppower_col = fields_info[5].rdb_field_no;
		agvc->last2_uppower_col = fields_info[6].rdb_field_no;
		agvc->last3_uppower_col = fields_info[7].rdb_field_no;
		agvc->last4_uppower_col = fields_info[8].rdb_field_no;
		agvc->last5_uppower_col = fields_info[9].rdb_field_no;
		agvc->last6_uppower_col = fields_info[10].rdb_field_no;
		agvc->last7_uppower_col = fields_info[11].rdb_field_no;
		agvc->last8_uppower_col = fields_info[12].rdb_field_no;
		agvc->last9_uppower_col = fields_info[13].rdb_field_no;
		agvc->last10_uppower_col = fields_info[14].rdb_field_no;
		agvc->last11_uppower_col = fields_info[15].rdb_field_no;
		agvc->last12_uppower_col = fields_info[16].rdb_field_no;

		agvc->thismonth_downpower_col = fields_info[17].rdb_field_no;
		agvc->last1_downpower_col = fields_info[18].rdb_field_no;
		agvc->last2_downpower_col = fields_info[19].rdb_field_no;
		agvc->last3_downpower_col = fields_info[20].rdb_field_no;
		agvc->last4_downpower_col = fields_info[21].rdb_field_no;
		agvc->last5_downpower_col = fields_info[22].rdb_field_no;
		agvc->last6_downpower_col = fields_info[23].rdb_field_no;
		agvc->last7_downpower_col = fields_info[24].rdb_field_no;
		agvc->last8_downpower_col = fields_info[25].rdb_field_no;
		agvc->last9_downpower_col = fields_info[26].rdb_field_no;
		agvc->last10_downpower_col = fields_info[27].rdb_field_no;
		agvc->last11_downpower_col = fields_info[28].rdb_field_no;
		agvc->last12_downpower_col = fields_info[29].rdb_field_no;

		agvc->today_uppower_col = fields_info[30].rdb_field_no;
		agvc->day1_uppower_col = fields_info[31].rdb_field_no;
		agvc->day2_uppower_col = fields_info[32].rdb_field_no;
		agvc->day3_uppower_col = fields_info[33].rdb_field_no;
		agvc->day4_uppower_col = fields_info[34].rdb_field_no;
		agvc->day5_uppower_col = fields_info[35].rdb_field_no;
		agvc->day6_uppower_col = fields_info[36].rdb_field_no;
		agvc->day7_uppower_col = fields_info[37].rdb_field_no;
		agvc->day8_uppower_col = fields_info[38].rdb_field_no;
		agvc->day9_uppower_col = fields_info[39].rdb_field_no;
		agvc->day10_uppower_col = fields_info[40].rdb_field_no;
		agvc->day11_uppower_col = fields_info[41].rdb_field_no;
		agvc->day12_uppower_col = fields_info[42].rdb_field_no;
		agvc->day13_uppower_col = fields_info[43].rdb_field_no;
		agvc->day14_uppower_col = fields_info[44].rdb_field_no;
		agvc->day15_uppower_col = fields_info[45].rdb_field_no;
		agvc->day16_uppower_col = fields_info[46].rdb_field_no;
		agvc->day17_uppower_col = fields_info[47].rdb_field_no;
		agvc->day18_uppower_col = fields_info[48].rdb_field_no;
		agvc->day19_uppower_col = fields_info[49].rdb_field_no;
		agvc->day20_uppower_col = fields_info[50].rdb_field_no;
		agvc->day21_uppower_col = fields_info[51].rdb_field_no;
		agvc->day22_uppower_col = fields_info[52].rdb_field_no;
		agvc->day23_uppower_col = fields_info[53].rdb_field_no;
		agvc->day24_uppower_col = fields_info[54].rdb_field_no;
		agvc->day25_uppower_col = fields_info[55].rdb_field_no;
		agvc->day26_uppower_col = fields_info[56].rdb_field_no;
		agvc->day27_uppower_col = fields_info[57].rdb_field_no;
		agvc->day28_uppower_col = fields_info[58].rdb_field_no;
		agvc->day29_uppower_col = fields_info[59].rdb_field_no;
		agvc->day30_uppower_col = fields_info[60].rdb_field_no;
	    agvc->day31_uppower_col = fields_info[61].rdb_field_no;


		agvc->today_downpower_col = fields_info[62].rdb_field_no;
		agvc->day1_downpower_col = fields_info[63].rdb_field_no;
		agvc->day2_downpower_col = fields_info[64].rdb_field_no;
		agvc->day3_downpower_col = fields_info[65].rdb_field_no;
		agvc->day4_downpower_col = fields_info[66].rdb_field_no;
		agvc->day5_downpower_col = fields_info[67].rdb_field_no;
		agvc->day6_downpower_col = fields_info[68].rdb_field_no;
		agvc->day7_downpower_col = fields_info[69].rdb_field_no;
		agvc->day8_downpower_col = fields_info[70].rdb_field_no;
		agvc->day9_downpower_col = fields_info[71].rdb_field_no;
		agvc->day10_downpower_col = fields_info[72].rdb_field_no;
		agvc->day11_downpower_col = fields_info[73].rdb_field_no;
		agvc->day12_downpower_col = fields_info[74].rdb_field_no;
		agvc->day13_downpower_col = fields_info[75].rdb_field_no;
		agvc->day14_downpower_col = fields_info[76].rdb_field_no;
		agvc->day15_downpower_col = fields_info[77].rdb_field_no;
		agvc->day16_downpower_col = fields_info[78].rdb_field_no;
		agvc->day17_downpower_col = fields_info[79].rdb_field_no;
		agvc->day18_downpower_col = fields_info[80].rdb_field_no;
		agvc->day19_downpower_col = fields_info[81].rdb_field_no;
		agvc->day20_downpower_col = fields_info[82].rdb_field_no;
		agvc->day21_downpower_col = fields_info[83].rdb_field_no;
		agvc->day22_downpower_col = fields_info[84].rdb_field_no;
		agvc->day23_downpower_col = fields_info[85].rdb_field_no;
		agvc->day24_downpower_col = fields_info[86].rdb_field_no;
		agvc->day25_downpower_col = fields_info[87].rdb_field_no;
		agvc->day26_downpower_col = fields_info[88].rdb_field_no;
		agvc->day27_downpower_col = fields_info[89].rdb_field_no;
		agvc->day28_downpower_col = fields_info[90].rdb_field_no;
		agvc->day29_downpower_col = fields_info[91].rdb_field_no;
		agvc->day30_downpower_col = fields_info[92].rdb_field_no;
		agvc->day31_downpower_col = fields_info[93].rdb_field_no;


		gatepower_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}











//整站电量月分析表

int Cagvc_ctrl_mgr::read_station_monthpower_info_table()
{
	for(int i=0; i<station_monthpower_list.size(); i++)
	{
		delete station_monthpower_list.at(i);
	}
	station_monthpower_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 38;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"year",
		"year_uppower",
		"year_downpower",
		"month1_uppower",
		"month2_uppower",
		"month3_uppower",
		"month4_uppower",
		"month5_uppower",
		"month6_uppower",
		"month7_uppower",
		"month8_uppower",
		"month9_uppower",
		"month10_uppower",
		"month11_uppower",
		"month12_uppower",
		"season1_uppower",
		"season2_uppower",
		"season3_uppower",
		"season4_uppower",
		"month1_downpower",
		"month2_downpower",
		"month3_downpower",
		"month4_downpower",
		"month5_downpower",
		"month6_downpower",
		"month7_downpower",
		"month8_downpower",
		"month9_downpower",
		"month10_downpower",
		"month11_downpower",
		"month12_downpower",
		"season1_downpower",
		"season2_downpower",
		"season3_downpower",
		"season4_downpower",

	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("pj_station_monthpower_info", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读整站月电量表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读整站月电量表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "整站月电量表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cstation_monthpower_info *agvc = new Cstation_monthpower_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->year, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->year_uppower, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->year_downpower, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->month1_uppower, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->month2_uppower, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&agvc->month3_uppower, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->month4_uppower, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->month5_uppower, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->month6_uppower, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->month7_uppower, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&agvc->month8_uppower, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->month9_uppower, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->month10_uppower, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->month11_uppower, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&agvc->month12_uppower, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;

		memcpy((char *)&agvc->season1_uppower, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->season2_uppower, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&agvc->season3_uppower, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->season4_uppower, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;

		memcpy((char *)&agvc->month1_downpower, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->month2_downpower, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->month3_downpower, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->month4_downpower, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->month5_downpower, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->month6_downpower, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->month7_downpower, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->month8_downpower, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;
		memcpy((char *)&agvc->month9_downpower, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;
		memcpy((char *)&agvc->month10_downpower, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;
		memcpy((char *)&agvc->month11_downpower, buffer+record_pos+offset, fields_info[32].field_len);
		offset += fields_info[32].field_len;
		memcpy((char *)&agvc->month12_downpower, buffer+record_pos+offset, fields_info[33].field_len);
		offset += fields_info[33].field_len;	

		memcpy((char *)&agvc->season1_downpower, buffer+record_pos+offset, fields_info[34].field_len);
		offset += fields_info[34].field_len;
		memcpy((char *)&agvc->season2_downpower, buffer+record_pos+offset, fields_info[35].field_len);
		offset += fields_info[35].field_len;
		memcpy((char *)&agvc->season3_downpower, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;
		memcpy((char *)&agvc->season4_downpower, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;

		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->year_col = fields_info[3].rdb_field_no;
		agvc->year_uppower_col = fields_info[4].rdb_field_no;
		agvc->year_downpower_col = fields_info[5].rdb_field_no;
		agvc->month1_uppower_col = fields_info[6].rdb_field_no;
		agvc->month2_uppower_col = fields_info[7].rdb_field_no;
		agvc->month3_uppower_col = fields_info[8].rdb_field_no;
		agvc->month4_uppower_col = fields_info[9].rdb_field_no;
		agvc->month5_uppower_col = fields_info[10].rdb_field_no;
		agvc->month6_uppower_col = fields_info[11].rdb_field_no;
		agvc->month7_uppower_col = fields_info[12].rdb_field_no;
		agvc->month8_uppower_col = fields_info[13].rdb_field_no;
		agvc->month9_uppower_col = fields_info[14].rdb_field_no;
		agvc->month10_uppower_col = fields_info[15].rdb_field_no;
		agvc->month11_uppower_col = fields_info[16].rdb_field_no;
		agvc->month12_uppower_col = fields_info[17].rdb_field_no;

		agvc->season1_uppower_col = fields_info[18].rdb_field_no;
		agvc->season2_uppower_col = fields_info[19].rdb_field_no;
		agvc->season3_uppower_col = fields_info[20].rdb_field_no;
		agvc->season4_uppower_col = fields_info[21].rdb_field_no;


		agvc->month1_downpower_col = fields_info[22].rdb_field_no;
		agvc->month2_downpower_col = fields_info[23].rdb_field_no;
		agvc->month3_downpower_col = fields_info[24].rdb_field_no;
		agvc->month4_downpower_col = fields_info[25].rdb_field_no;
		agvc->month5_downpower_col = fields_info[26].rdb_field_no;
		agvc->month6_downpower_col = fields_info[27].rdb_field_no;
		agvc->month7_downpower_col = fields_info[28].rdb_field_no;
		agvc->month8_downpower_col = fields_info[29].rdb_field_no;
		agvc->month9_downpower_col = fields_info[30].rdb_field_no;
		agvc->month10_downpower_col = fields_info[31].rdb_field_no;
		agvc->month11_downpower_col = fields_info[32].rdb_field_no;
		agvc->month12_downpower_col = fields_info[33].rdb_field_no;

		agvc->season1_downpower_col = fields_info[34].rdb_field_no;
		agvc->season2_downpower_col = fields_info[35].rdb_field_no;
		agvc->season3_downpower_col = fields_info[36].rdb_field_no;
		agvc->season4_downpower_col = fields_info[37].rdb_field_no;

		station_monthpower_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}





//整站电量日分析表

int Cagvc_ctrl_mgr::read_station_daypower_info_table()
{
	for(int i=0; i<station_daypower_list.size(); i++)
	{
		delete station_daypower_list.at(i);
	}
	station_daypower_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 66;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"month",
		"day1_uppower",
		"day2_uppower",
		"day3_uppower",
		"day4_uppower",
		"day5_uppower",
		"day6_uppower",
		"day7_uppower",
		"day8_uppower",
		"day9_uppower",
		"day10_uppower",
		"day11_uppower",
		"day12_uppower",
		"day13_uppower",
		"day14_uppower",
		"day15_uppower",
		"day16_uppower",
		"day17_uppower",
		"day18_uppower",
		"day19_uppower",
		"day20_uppower",
		"day21_uppower",
		"day22_uppower",
		"day23_uppower",
		"day24_uppower",
		"day25_uppower",
		"day26_uppower",
		"day27_uppower",
		"day28_uppower",
		"day29_uppower",
		"day30_uppower",
		"day31_uppower",
		"day1_downpower",
		"day2_downpower",
		"day3_downpower",
		"day4_downpower",
		"day5_downpower",
		"day6_downpower",
		"day7_downpower",
		"day8_downpower",
		"day9_downpower",
		"day10_downpower",
		"day11_downpower",
		"day12_downpower",
		"day13_downpower",
		"day14_downpower",
		"day15_downpower",
		"day16_downpower",
		"day17_downpower",
		"day18_downpower",
		"day19_downpower",
		"day20_downpower",
		"day21_downpower",
		"day22_downpower",
		"day23_downpower",
		"day24_downpower",
		"day25_downpower",
		"day26_downpower",
		"day27_downpower",
		"day28_downpower",
		"day29_downpower",
		"day30_downpower",
		"day31_downpower",

	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("pj_station_daypower_info", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读整站日电量表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读整站日电量表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "整站日电量表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cstation_daypower_info *agvc = new Cstation_daypower_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->month, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->day1_uppower, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->day2_uppower, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->day3_uppower, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->day4_uppower, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&agvc->day5_uppower, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->day6_uppower, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->day7_uppower, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->day8_uppower, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->day9_uppower, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&agvc->day10_uppower, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->day11_uppower, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->day12_uppower, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->day13_uppower, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&agvc->day14_uppower, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;
		memcpy((char *)&agvc->day15_uppower, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->day16_uppower, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&agvc->day17_uppower, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->day18_uppower, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&agvc->day19_uppower, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->day20_uppower, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->day21_uppower, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->day22_uppower, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->day23_uppower, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->day24_uppower, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->day25_uppower, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->day26_uppower, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;
		memcpy((char *)&agvc->day27_uppower, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;
		memcpy((char *)&agvc->day28_uppower, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;
		memcpy((char *)&agvc->day29_uppower, buffer+record_pos+offset, fields_info[32].field_len);
		offset += fields_info[32].field_len;
		memcpy((char *)&agvc->day30_uppower, buffer+record_pos+offset, fields_info[33].field_len);
		offset += fields_info[33].field_len;		
		memcpy((char *)&agvc->day31_uppower, buffer+record_pos+offset, fields_info[34].field_len);
		offset += fields_info[4].field_len;

		memcpy((char *)&agvc->day1_downpower, buffer+record_pos+offset, fields_info[35].field_len);
		offset += fields_info[35].field_len;
		memcpy((char *)&agvc->day2_downpower, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;
		memcpy((char *)&agvc->day3_downpower, buffer+record_pos+offset, fields_info[37].field_len);
		offset += fields_info[37].field_len;
		memcpy((char *)&agvc->day4_downpower, buffer+record_pos+offset, fields_info[38].field_len);
		offset += fields_info[38].field_len;
		memcpy((char *)&agvc->day5_downpower, buffer+record_pos+offset, fields_info[39].field_len);
		offset += fields_info[39].field_len;
		memcpy((char *)&agvc->day6_downpower, buffer+record_pos+offset, fields_info[40].field_len);
		offset += fields_info[40].field_len;
		memcpy((char *)&agvc->day7_downpower, buffer+record_pos+offset, fields_info[41].field_len);
		offset += fields_info[41].field_len;
		memcpy((char *)&agvc->day8_downpower, buffer+record_pos+offset, fields_info[42].field_len);
		offset += fields_info[42].field_len;
		memcpy((char *)&agvc->day9_downpower, buffer+record_pos+offset, fields_info[43].field_len);
		offset += fields_info[43].field_len;
		memcpy((char *)&agvc->day10_downpower, buffer+record_pos+offset, fields_info[44].field_len);
		offset += fields_info[44].field_len;
		memcpy((char *)&agvc->day11_downpower, buffer+record_pos+offset, fields_info[45].field_len);
		offset += fields_info[45].field_len;
		memcpy((char *)&agvc->day12_downpower, buffer+record_pos+offset, fields_info[46].field_len);
		offset += fields_info[46].field_len;
		memcpy((char *)&agvc->day13_downpower, buffer+record_pos+offset, fields_info[47].field_len);
		offset += fields_info[47].field_len;
		memcpy((char *)&agvc->day14_downpower, buffer+record_pos+offset, fields_info[48].field_len);
		offset += fields_info[48].field_len;
		memcpy((char *)&agvc->day15_downpower, buffer+record_pos+offset, fields_info[49].field_len);
		offset += fields_info[49].field_len;
		memcpy((char *)&agvc->day16_downpower, buffer+record_pos+offset, fields_info[50].field_len);
		offset += fields_info[50].field_len;
		memcpy((char *)&agvc->day17_downpower, buffer+record_pos+offset, fields_info[51].field_len);
		offset += fields_info[51].field_len;
		memcpy((char *)&agvc->day18_downpower, buffer+record_pos+offset, fields_info[52].field_len);
		offset += fields_info[52].field_len;
		memcpy((char *)&agvc->day19_downpower, buffer+record_pos+offset, fields_info[53].field_len);
		offset += fields_info[53].field_len;
		memcpy((char *)&agvc->day20_downpower, buffer+record_pos+offset, fields_info[54].field_len);
		offset += fields_info[54].field_len;
		memcpy((char *)&agvc->day21_downpower, buffer+record_pos+offset, fields_info[55].field_len);
		offset += fields_info[55].field_len;
		memcpy((char *)&agvc->day22_downpower, buffer+record_pos+offset, fields_info[56].field_len);
		offset += fields_info[56].field_len;
		memcpy((char *)&agvc->day23_downpower, buffer+record_pos+offset, fields_info[57].field_len);
		offset += fields_info[57].field_len;
		memcpy((char *)&agvc->day24_downpower, buffer+record_pos+offset, fields_info[58].field_len);
		offset += fields_info[58].field_len;
		memcpy((char *)&agvc->day25_downpower, buffer+record_pos+offset, fields_info[59].field_len);
		offset += fields_info[59].field_len;
		memcpy((char *)&agvc->day26_downpower, buffer+record_pos+offset, fields_info[60].field_len);
		offset += fields_info[60].field_len;
		memcpy((char *)&agvc->day27_downpower, buffer+record_pos+offset, fields_info[61].field_len);
		offset += fields_info[61].field_len;

		memcpy((char *)&agvc->day28_downpower, buffer+record_pos+offset, fields_info[62].field_len);
		offset += fields_info[62].field_len;
		memcpy((char *)&agvc->day29_downpower, buffer+record_pos+offset, fields_info[63].field_len);
		offset += fields_info[63].field_len;
		memcpy((char *)&agvc->day30_downpower, buffer+record_pos+offset, fields_info[64].field_len);
		offset += fields_info[64].field_len;
		memcpy((char *)&agvc->day31_downpower, buffer+record_pos+offset, fields_info[65].field_len);
		offset += fields_info[65].field_len;


		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->month_col = fields_info[3].rdb_field_no;
		agvc->day1_uppower_col = fields_info[4].rdb_field_no;
		agvc->day2_uppower_col = fields_info[5].rdb_field_no;
		agvc->day3_uppower_col = fields_info[6].rdb_field_no;
		agvc->day4_uppower_col = fields_info[7].rdb_field_no;
		agvc->day5_uppower_col = fields_info[8].rdb_field_no;
		agvc->day6_uppower_col = fields_info[9].rdb_field_no;
		agvc->day7_uppower_col = fields_info[10].rdb_field_no;
		agvc->day8_uppower_col = fields_info[11].rdb_field_no;
		agvc->day9_uppower_col = fields_info[12].rdb_field_no;
		agvc->day10_uppower_col = fields_info[13].rdb_field_no;
		agvc->day11_uppower_col = fields_info[14].rdb_field_no;
		agvc->day12_uppower_col = fields_info[15].rdb_field_no;
		agvc->day13_uppower_col = fields_info[16].rdb_field_no;
		agvc->day14_uppower_col = fields_info[17].rdb_field_no;
		agvc->day15_uppower_col = fields_info[18].rdb_field_no;
		agvc->day16_uppower_col = fields_info[19].rdb_field_no;
		agvc->day17_uppower_col = fields_info[20].rdb_field_no;
		agvc->day18_uppower_col = fields_info[21].rdb_field_no;
		agvc->day19_uppower_col = fields_info[22].rdb_field_no;
		agvc->day20_uppower_col = fields_info[23].rdb_field_no;
		agvc->day21_uppower_col = fields_info[24].rdb_field_no;
		agvc->day22_uppower_col = fields_info[25].rdb_field_no;
		agvc->day23_uppower_col = fields_info[26].rdb_field_no;
		agvc->day24_uppower_col = fields_info[27].rdb_field_no;
		agvc->day25_uppower_col = fields_info[28].rdb_field_no;
		agvc->day26_uppower_col = fields_info[29].rdb_field_no;
		agvc->day27_uppower_col = fields_info[30].rdb_field_no;
		agvc->day28_uppower_col = fields_info[31].rdb_field_no;
		agvc->day29_uppower_col = fields_info[32].rdb_field_no;
		agvc->day30_uppower_col = fields_info[33].rdb_field_no;
		agvc->day31_uppower_col = fields_info[34].rdb_field_no;

		agvc->day1_downpower_col = fields_info[35].rdb_field_no;
		agvc->day2_downpower_col = fields_info[36].rdb_field_no;
		agvc->day3_downpower_col = fields_info[37].rdb_field_no;
		agvc->day4_downpower_col = fields_info[38].rdb_field_no;
		agvc->day5_downpower_col = fields_info[39].rdb_field_no;
		agvc->day6_downpower_col = fields_info[40].rdb_field_no;
		agvc->day7_downpower_col = fields_info[41].rdb_field_no;
		agvc->day8_downpower_col = fields_info[42].rdb_field_no;
		agvc->day9_downpower_col = fields_info[43].rdb_field_no;
		agvc->day10_downpower_col = fields_info[44].rdb_field_no;
		agvc->day11_downpower_col = fields_info[45].rdb_field_no;
		agvc->day12_downpower_col = fields_info[46].rdb_field_no;
		agvc->day13_downpower_col = fields_info[47].rdb_field_no;
		agvc->day14_downpower_col = fields_info[48].rdb_field_no;
		agvc->day15_downpower_col = fields_info[49].rdb_field_no;
		agvc->day16_downpower_col = fields_info[50].rdb_field_no;
		agvc->day17_downpower_col = fields_info[51].rdb_field_no;
		agvc->day18_downpower_col = fields_info[52].rdb_field_no;
		agvc->day19_downpower_col = fields_info[53].rdb_field_no;
		agvc->day20_downpower_col = fields_info[54].rdb_field_no;
		agvc->day21_downpower_col = fields_info[55].rdb_field_no;
		agvc->day22_downpower_col = fields_info[56].rdb_field_no;
		agvc->day23_downpower_col = fields_info[57].rdb_field_no;
		agvc->day24_downpower_col = fields_info[58].rdb_field_no;
		agvc->day25_downpower_col = fields_info[59].rdb_field_no;
		agvc->day26_downpower_col = fields_info[60].rdb_field_no;
		agvc->day27_downpower_col = fields_info[61].rdb_field_no;
		agvc->day28_downpower_col = fields_info[62].rdb_field_no;
		agvc->day29_downpower_col = fields_info[63].rdb_field_no;
		agvc->day30_downpower_col = fields_info[64].rdb_field_no;
		agvc->day31_downpower_col = fields_info[65].rdb_field_no;

		station_daypower_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}






/*函数名：read_unit_monthpower_info_table()
 *输入:
 *输出：
 *功能简介：读取储能单元月电量分析表
 *时间：[8/2/2021 LJF]
 */
int Cagvc_ctrl_mgr::read_unit_monthpower_info_table()
{


	for(int i=0; i<unit_monthpower_list.size(); i++)
	{
		delete unit_monthpower_list.at(i);
	}
	unit_monthpower_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 39;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"year",
		"cal_mode",
		"year_uppower",
		"year_downpower",
		"month1_uppower",
		"month2_uppower",
		"month3_uppower",
		"month4_uppower",
		"month5_uppower",
		"month6_uppower",
		"month7_uppower",
		"month8_uppower",
		"month9_uppower",
		"month10_uppower",
		"month11_uppower",
		"month12_uppower",
		"season1_uppower",
		"season2_uppower",
		"season3_uppower",
		"season4_uppower",
		"month1_downpower",
		"month2_downpower",
		"month3_downpower",
		"month4_downpower",
		"month5_downpower",
		"month6_downpower",
		"month7_downpower",
		"month8_downpower",
		"month9_downpower",
		"month10_downpower",
		"month11_downpower",
		"month12_downpower",
		"season1_downpower",
		"season2_downpower",
		"season3_downpower",
		"season4_downpower",

	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("pj_unit_monthpower_info", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元月电量表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元月电量表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元月电量表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cunit_monthpower_info *agvc = new Cunit_monthpower_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->year, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->year_uppower, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->year_downpower, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->month1_uppower, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->month2_uppower, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&agvc->month3_uppower, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->month4_uppower, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->month5_uppower, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->month6_uppower, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->month7_uppower, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&agvc->month8_uppower, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->month9_uppower, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->month10_uppower, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->month11_uppower, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&agvc->month12_uppower, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;

		memcpy((char *)&agvc->season1_uppower, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->season2_uppower, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&agvc->season3_uppower, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->season4_uppower, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;

		memcpy((char *)&agvc->month1_downpower, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->month2_downpower, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->month3_downpower, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->month4_downpower, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->month5_downpower, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->month6_downpower, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->month7_downpower, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->month8_downpower, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;
		memcpy((char *)&agvc->month9_downpower, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;
		memcpy((char *)&agvc->month10_downpower, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;
		memcpy((char *)&agvc->month11_downpower, buffer+record_pos+offset, fields_info[32].field_len);
		offset += fields_info[32].field_len;
		memcpy((char *)&agvc->month12_downpower, buffer+record_pos+offset, fields_info[33].field_len);
		offset += fields_info[33].field_len;	

		memcpy((char *)&agvc->season1_downpower, buffer+record_pos+offset, fields_info[34].field_len);
		offset += fields_info[34].field_len;
		memcpy((char *)&agvc->season2_downpower, buffer+record_pos+offset, fields_info[35].field_len);
		offset += fields_info[35].field_len;
		memcpy((char *)&agvc->season3_downpower, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;
		memcpy((char *)&agvc->season4_downpower, buffer+record_pos+offset, fields_info[37].field_len);
		offset += fields_info[37].field_len;
		memcpy((char *)&agvc->cal_mode_col, buffer+record_pos+offset, fields_info[38].field_len);
		offset += fields_info[38].field_len;


		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->year_col = fields_info[3].rdb_field_no;
		agvc->year_uppower_col = fields_info[4].rdb_field_no;
		agvc->year_downpower_col = fields_info[5].rdb_field_no;
		agvc->month1_uppower_col = fields_info[6].rdb_field_no;
		agvc->month2_uppower_col = fields_info[7].rdb_field_no;
		agvc->month3_uppower_col = fields_info[8].rdb_field_no;
		agvc->month4_uppower_col = fields_info[9].rdb_field_no;
		agvc->month5_uppower_col = fields_info[10].rdb_field_no;
		agvc->month6_uppower_col = fields_info[11].rdb_field_no;
		agvc->month7_uppower_col = fields_info[12].rdb_field_no;
		agvc->month8_uppower_col = fields_info[13].rdb_field_no;
		agvc->month9_uppower_col = fields_info[14].rdb_field_no;
		agvc->month10_uppower_col = fields_info[15].rdb_field_no;
		agvc->month11_uppower_col = fields_info[16].rdb_field_no;
		agvc->month12_uppower_col = fields_info[17].rdb_field_no;

		agvc->season1_uppower_col = fields_info[18].rdb_field_no;
		agvc->season2_uppower_col = fields_info[19].rdb_field_no;
		agvc->season3_uppower_col = fields_info[20].rdb_field_no;
		agvc->season4_uppower_col = fields_info[21].rdb_field_no;

		agvc->month1_downpower_col = fields_info[22].rdb_field_no;
		agvc->month2_downpower_col = fields_info[23].rdb_field_no;
		agvc->month3_downpower_col = fields_info[24].rdb_field_no;
		agvc->month4_downpower_col = fields_info[25].rdb_field_no;
		agvc->month5_downpower_col = fields_info[26].rdb_field_no;
		agvc->month6_downpower_col = fields_info[27].rdb_field_no;
		agvc->month7_downpower_col = fields_info[28].rdb_field_no;
		agvc->month8_downpower_col = fields_info[29].rdb_field_no;
		agvc->month9_downpower_col = fields_info[30].rdb_field_no;
		agvc->month10_downpower_col = fields_info[31].rdb_field_no;
		agvc->month11_downpower_col = fields_info[32].rdb_field_no;
		agvc->month12_downpower_col = fields_info[33].rdb_field_no;

		agvc->season1_downpower_col = fields_info[34].rdb_field_no;
		agvc->season2_downpower_col = fields_info[35].rdb_field_no;
		agvc->season3_downpower_col = fields_info[36].rdb_field_no;
		agvc->season4_downpower_col = fields_info[37].rdb_field_no;
		agvc->cal_mode_col = fields_info[38].rdb_field_no;
		unit_monthpower_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;

}






int Cagvc_ctrl_mgr::read_unit_daypower_info_table()
{

	for(int i=0; i<unit_daypower_list.size(); i++)
	{
		delete unit_daypower_list.at(i);
	}
	unit_daypower_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 67;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"month",
		"cal_mode",
		"day1_uppower",
		"day2_uppower",
		"day3_uppower",
		"day4_uppower",
		"day5_uppower",
		"day6_uppower",
		"day7_uppower",
		"day8_uppower",
		"day9_uppower",
		"day10_uppower",
		"day11_uppower",
		"day12_uppower",
		"day13_uppower",
		"day14_uppower",
		"day15_uppower",
		"day16_uppower",
		"day17_uppower",
		"day18_uppower",
		"day19_uppower",
		"day20_uppower",
		"day21_uppower",
		"day22_uppower",
		"day23_uppower",
		"day24_uppower",
		"day25_uppower",
		"day26_uppower",
		"day27_uppower",
		"day28_uppower",
		"day29_uppower",
		"day30_uppower",
		"day31_uppower",
		"day1_downpower",
		"day2_downpower",
		"day3_downpower",
		"day4_downpower",
		"day5_downpower",
		"day6_downpower",
		"day7_downpower",
		"day8_downpower",
		"day9_downpower",
		"day10_downpower",
		"day11_downpower",
		"day12_downpower",
		"day13_downpower",
		"day14_downpower",
		"day15_downpower",
		"day16_downpower",
		"day17_downpower",
		"day18_downpower",
		"day19_downpower",
		"day20_downpower",
		"day21_downpower",
		"day22_downpower",
		"day23_downpower",
		"day24_downpower",
		"day25_downpower",
		"day26_downpower",
		"day27_downpower",
		"day28_downpower",
		"day29_downpower",
		"day30_downpower",
		"day31_downpower",

	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("pj_unit_daypower_info", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元日电量表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元日电量表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "储能单元日电量表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cunit_daypower_info *agvc = new Cunit_daypower_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->month, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->day1_uppower, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->day2_uppower, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->day3_uppower, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->day4_uppower, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&agvc->day5_uppower, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->day6_uppower, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->day7_uppower, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->day8_uppower, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->day9_uppower, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&agvc->day10_uppower, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->day11_uppower, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->day12_uppower, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->day13_uppower, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&agvc->day14_uppower, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;
		memcpy((char *)&agvc->day15_uppower, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->day16_uppower, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&agvc->day17_uppower, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->day18_uppower, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&agvc->day19_uppower, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->day20_uppower, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->day21_uppower, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->day22_uppower, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->day23_uppower, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->day24_uppower, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->day25_uppower, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->day26_uppower, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;
		memcpy((char *)&agvc->day27_uppower, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;
		memcpy((char *)&agvc->day28_uppower, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;
		memcpy((char *)&agvc->day29_uppower, buffer+record_pos+offset, fields_info[32].field_len);
		offset += fields_info[32].field_len;
		memcpy((char *)&agvc->day30_uppower, buffer+record_pos+offset, fields_info[33].field_len);
		offset += fields_info[33].field_len;		
		memcpy((char *)&agvc->day31_uppower, buffer+record_pos+offset, fields_info[34].field_len);
		offset += fields_info[4].field_len;

		memcpy((char *)&agvc->day1_downpower, buffer+record_pos+offset, fields_info[35].field_len);
		offset += fields_info[35].field_len;
		memcpy((char *)&agvc->day2_downpower, buffer+record_pos+offset, fields_info[36].field_len);
		offset += fields_info[36].field_len;
		memcpy((char *)&agvc->day3_downpower, buffer+record_pos+offset, fields_info[37].field_len);
		offset += fields_info[37].field_len;
		memcpy((char *)&agvc->day4_downpower, buffer+record_pos+offset, fields_info[38].field_len);
		offset += fields_info[38].field_len;
		memcpy((char *)&agvc->day5_downpower, buffer+record_pos+offset, fields_info[39].field_len);
		offset += fields_info[39].field_len;
		memcpy((char *)&agvc->day6_downpower, buffer+record_pos+offset, fields_info[40].field_len);
		offset += fields_info[40].field_len;
		memcpy((char *)&agvc->day7_downpower, buffer+record_pos+offset, fields_info[41].field_len);
		offset += fields_info[41].field_len;
		memcpy((char *)&agvc->day8_downpower, buffer+record_pos+offset, fields_info[42].field_len);
		offset += fields_info[42].field_len;
		memcpy((char *)&agvc->day9_downpower, buffer+record_pos+offset, fields_info[43].field_len);
		offset += fields_info[43].field_len;
		memcpy((char *)&agvc->day10_downpower, buffer+record_pos+offset, fields_info[44].field_len);
		offset += fields_info[44].field_len;
		memcpy((char *)&agvc->day11_downpower, buffer+record_pos+offset, fields_info[45].field_len);
		offset += fields_info[45].field_len;
		memcpy((char *)&agvc->day12_downpower, buffer+record_pos+offset, fields_info[46].field_len);
		offset += fields_info[46].field_len;
		memcpy((char *)&agvc->day13_downpower, buffer+record_pos+offset, fields_info[47].field_len);
		offset += fields_info[47].field_len;
		memcpy((char *)&agvc->day14_downpower, buffer+record_pos+offset, fields_info[48].field_len);
		offset += fields_info[48].field_len;
		memcpy((char *)&agvc->day15_downpower, buffer+record_pos+offset, fields_info[49].field_len);
		offset += fields_info[49].field_len;
		memcpy((char *)&agvc->day16_downpower, buffer+record_pos+offset, fields_info[50].field_len);
		offset += fields_info[50].field_len;
		memcpy((char *)&agvc->day17_downpower, buffer+record_pos+offset, fields_info[51].field_len);
		offset += fields_info[51].field_len;
		memcpy((char *)&agvc->day18_downpower, buffer+record_pos+offset, fields_info[52].field_len);
		offset += fields_info[52].field_len;
		memcpy((char *)&agvc->day19_downpower, buffer+record_pos+offset, fields_info[53].field_len);
		offset += fields_info[53].field_len;
		memcpy((char *)&agvc->day20_downpower, buffer+record_pos+offset, fields_info[54].field_len);
		offset += fields_info[54].field_len;
		memcpy((char *)&agvc->day21_downpower, buffer+record_pos+offset, fields_info[55].field_len);
		offset += fields_info[55].field_len;
		memcpy((char *)&agvc->day22_downpower, buffer+record_pos+offset, fields_info[56].field_len);
		offset += fields_info[56].field_len;
		memcpy((char *)&agvc->day23_downpower, buffer+record_pos+offset, fields_info[57].field_len);
		offset += fields_info[57].field_len;
		memcpy((char *)&agvc->day24_downpower, buffer+record_pos+offset, fields_info[58].field_len);
		offset += fields_info[58].field_len;
		memcpy((char *)&agvc->day25_downpower, buffer+record_pos+offset, fields_info[59].field_len);
		offset += fields_info[59].field_len;
		memcpy((char *)&agvc->day26_downpower, buffer+record_pos+offset, fields_info[60].field_len);
		offset += fields_info[60].field_len;
		memcpy((char *)&agvc->day27_downpower, buffer+record_pos+offset, fields_info[61].field_len);
		offset += fields_info[61].field_len;

		memcpy((char *)&agvc->day28_downpower, buffer+record_pos+offset, fields_info[62].field_len);
		offset += fields_info[62].field_len;
		memcpy((char *)&agvc->day29_downpower, buffer+record_pos+offset, fields_info[63].field_len);
		offset += fields_info[63].field_len;
		memcpy((char *)&agvc->day30_downpower, buffer+record_pos+offset, fields_info[64].field_len);
		offset += fields_info[64].field_len;
		memcpy((char *)&agvc->day31_downpower, buffer+record_pos+offset, fields_info[65].field_len);
		offset += fields_info[65].field_len;
		memcpy((char *)&agvc->cal_mode, buffer+record_pos+offset, fields_info[66].field_len);
		offset += fields_info[66].field_len;

		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->month_col = fields_info[3].rdb_field_no;
		agvc->day1_uppower_col = fields_info[4].rdb_field_no;
		agvc->day2_uppower_col = fields_info[5].rdb_field_no;
		agvc->day3_uppower_col = fields_info[6].rdb_field_no;
		agvc->day4_uppower_col = fields_info[7].rdb_field_no;
		agvc->day5_uppower_col = fields_info[8].rdb_field_no;
		agvc->day6_uppower_col = fields_info[9].rdb_field_no;
		agvc->day7_uppower_col = fields_info[10].rdb_field_no;
		agvc->day8_uppower_col = fields_info[11].rdb_field_no;
		agvc->day9_uppower_col = fields_info[12].rdb_field_no;
		agvc->day10_uppower_col = fields_info[13].rdb_field_no;
		agvc->day11_uppower_col = fields_info[14].rdb_field_no;
		agvc->day12_uppower_col = fields_info[15].rdb_field_no;
		agvc->day13_uppower_col = fields_info[16].rdb_field_no;
		agvc->day14_uppower_col = fields_info[17].rdb_field_no;
		agvc->day15_uppower_col = fields_info[18].rdb_field_no;
		agvc->day16_uppower_col = fields_info[19].rdb_field_no;
		agvc->day17_uppower_col = fields_info[20].rdb_field_no;
		agvc->day18_uppower_col = fields_info[21].rdb_field_no;
		agvc->day19_uppower_col = fields_info[22].rdb_field_no;
		agvc->day20_uppower_col = fields_info[23].rdb_field_no;
		agvc->day21_uppower_col = fields_info[24].rdb_field_no;
		agvc->day22_uppower_col = fields_info[25].rdb_field_no;
		agvc->day23_uppower_col = fields_info[26].rdb_field_no;
		agvc->day24_uppower_col = fields_info[27].rdb_field_no;
		agvc->day25_uppower_col = fields_info[28].rdb_field_no;
		agvc->day26_uppower_col = fields_info[29].rdb_field_no;
		agvc->day27_uppower_col = fields_info[30].rdb_field_no;
		agvc->day28_uppower_col = fields_info[31].rdb_field_no;
		agvc->day29_uppower_col = fields_info[32].rdb_field_no;
		agvc->day30_uppower_col = fields_info[33].rdb_field_no;
		agvc->day31_uppower_col = fields_info[34].rdb_field_no;

		agvc->day1_downpower_col = fields_info[35].rdb_field_no;
		agvc->day2_downpower_col = fields_info[36].rdb_field_no;
		agvc->day3_downpower_col = fields_info[37].rdb_field_no;
		agvc->day4_downpower_col = fields_info[38].rdb_field_no;
		agvc->day5_downpower_col = fields_info[39].rdb_field_no;
		agvc->day6_downpower_col = fields_info[40].rdb_field_no;
		agvc->day7_downpower_col = fields_info[41].rdb_field_no;
		agvc->day8_downpower_col = fields_info[42].rdb_field_no;
		agvc->day9_downpower_col = fields_info[43].rdb_field_no;
		agvc->day10_downpower_col = fields_info[44].rdb_field_no;
		agvc->day11_downpower_col = fields_info[45].rdb_field_no;
		agvc->day12_downpower_col = fields_info[46].rdb_field_no;
		agvc->day13_downpower_col = fields_info[47].rdb_field_no;
		agvc->day14_downpower_col = fields_info[48].rdb_field_no;
		agvc->day15_downpower_col = fields_info[49].rdb_field_no;
		agvc->day16_downpower_col = fields_info[50].rdb_field_no;
		agvc->day17_downpower_col = fields_info[51].rdb_field_no;
		agvc->day18_downpower_col = fields_info[52].rdb_field_no;
		agvc->day19_downpower_col = fields_info[53].rdb_field_no;
		agvc->day20_downpower_col = fields_info[54].rdb_field_no;
		agvc->day21_downpower_col = fields_info[55].rdb_field_no;
		agvc->day22_downpower_col = fields_info[56].rdb_field_no;
		agvc->day23_downpower_col = fields_info[57].rdb_field_no;
		agvc->day24_downpower_col = fields_info[58].rdb_field_no;
		agvc->day25_downpower_col = fields_info[59].rdb_field_no;
		agvc->day26_downpower_col = fields_info[60].rdb_field_no;
		agvc->day27_downpower_col = fields_info[61].rdb_field_no;
		agvc->day28_downpower_col = fields_info[62].rdb_field_no;
		agvc->day29_downpower_col = fields_info[63].rdb_field_no;
		agvc->day30_downpower_col = fields_info[64].rdb_field_no;
		agvc->day31_downpower_col = fields_info[65].rdb_field_no;
		agvc->cal_mode_col = fields_info[66].rdb_field_no;

		unit_daypower_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;

}



int Cagvc_ctrl_mgr::read_unit_runstate_info_table()
{

	for(int i=0; i<unit_runstate_list.size(); i++)
	{
		delete unit_runstate_list.at(i);
	}
	unit_runstate_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	const int field_num = 31;
	buffer = (char *)MALLOC(6000);

	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);
	char  English_names[field_num][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"id",
		"name",
		"month",
		"pcs_name",
		"unit_name",
		"unit_state",
		"control_state",
		"pcs_state",
		"pcs_power",
		"unit_soc",
		"unit_apacity",
		"total_uppower",
		"year_uppower",
		"month_uppower",
		"today_uppower",
		"time_uppower",
		"total_downpower",
		"year_downpower",
		"month_downpower",
		"today_downpower",
		"time_downpower",
		"total_runtime",
		"total_chartime",
		"total_distime",
		"year_chartime",	
		"year_distime",
		"month_chartime",
		"month_distime",
		"day_chartime",
		"day_distime",		

	};
	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("pj_unit_runstate_info", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元运行信息表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读储能单元运行信息表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "储能单元运行信息表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}


	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cunit_runstate_info *agvc = new Cunit_runstate_info(data_obj);
		agvc->table_id = table_id;

		memcpy((char *)&agvc->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&agvc->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&agvc->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&agvc->month, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&agvc->pcs_name, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;
		memcpy((char *)&agvc->unit_name, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&agvc->unit_state, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&agvc->control_state, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&agvc->pcs_state, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&agvc->pcs_power, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&agvc->unit_soc, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;
		memcpy((char *)&agvc->unit_apacity, buffer+record_pos+offset, fields_info[11].field_len);
		offset += fields_info[11].field_len;
		memcpy((char *)&agvc->total_uppower, buffer+record_pos+offset, fields_info[12].field_len);
		offset += fields_info[12].field_len;
		memcpy((char *)&agvc->year_uppower, buffer+record_pos+offset, fields_info[13].field_len);
		offset += fields_info[13].field_len;
		memcpy((char *)&agvc->month_uppower, buffer+record_pos+offset, fields_info[14].field_len);
		offset += fields_info[14].field_len;
		memcpy((char *)&agvc->today_uppower, buffer+record_pos+offset, fields_info[15].field_len);
		offset += fields_info[15].field_len;
		memcpy((char *)&agvc->time_uppower, buffer+record_pos+offset, fields_info[16].field_len);
		offset += fields_info[16].field_len;
		memcpy((char *)&agvc->total_downpower, buffer+record_pos+offset, fields_info[17].field_len);
		offset += fields_info[17].field_len;
		memcpy((char *)&agvc->year_downpower, buffer+record_pos+offset, fields_info[18].field_len);
		offset += fields_info[18].field_len;       
		memcpy((char *)&agvc->month_downpower, buffer+record_pos+offset, fields_info[19].field_len);
		offset += fields_info[19].field_len;
		memcpy((char *)&agvc->today_downpower, buffer+record_pos+offset, fields_info[20].field_len);
		offset += fields_info[20].field_len;
		memcpy((char *)&agvc->time_downpower, buffer+record_pos+offset, fields_info[21].field_len);
		offset += fields_info[21].field_len;
		memcpy((char *)&agvc->total_runtime, buffer+record_pos+offset, fields_info[22].field_len);
		offset += fields_info[22].field_len;
		memcpy((char *)&agvc->total_chartime, buffer+record_pos+offset, fields_info[23].field_len);
		offset += fields_info[23].field_len;
		memcpy((char *)&agvc->total_distime, buffer+record_pos+offset, fields_info[24].field_len);
		offset += fields_info[24].field_len;
		memcpy((char *)&agvc->year_chartime, buffer+record_pos+offset, fields_info[25].field_len);
		offset += fields_info[25].field_len;
		memcpy((char *)&agvc->year_distime, buffer+record_pos+offset, fields_info[26].field_len);
		offset += fields_info[26].field_len;
		memcpy((char *)&agvc->month_chartime, buffer+record_pos+offset, fields_info[27].field_len);
		offset += fields_info[27].field_len;
		memcpy((char *)&agvc->month_distime, buffer+record_pos+offset, fields_info[28].field_len);
		offset += fields_info[28].field_len;
		memcpy((char *)&agvc->day_chartime, buffer+record_pos+offset, fields_info[29].field_len);
		offset += fields_info[29].field_len;
		memcpy((char *)&agvc->day_distime, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;


		agvc->display_id_col = fields_info[0].rdb_field_no;
		agvc->name_col = fields_info[2].rdb_field_no;
		agvc->month_col = fields_info[3].rdb_field_no;
		agvc->pcs_name_col = fields_info[4].rdb_field_no;
		agvc->unit_name_col = fields_info[5].rdb_field_no;
		agvc->unit_state_col = fields_info[6].rdb_field_no;
		agvc->control_state_col = fields_info[7].rdb_field_no;
		agvc->pcs_state_col = fields_info[8].rdb_field_no;
		agvc->pcs_power_col = fields_info[9].rdb_field_no;
		agvc->unit_soc_col = fields_info[10].rdb_field_no;
		agvc->unit_apacity_col = fields_info[11].rdb_field_no;
		agvc->total_uppower_col = fields_info[12].rdb_field_no;
		agvc->year_uppower_col = fields_info[13].rdb_field_no;
		agvc->month_uppower_col= fields_info[14].rdb_field_no;
		agvc->today_uppower_col = fields_info[15].rdb_field_no;
		agvc->time_uppower_col = fields_info[16].rdb_field_no;
		agvc->total_downpower_col = fields_info[17].rdb_field_no;
		agvc->year_downpower_col = fields_info[18].rdb_field_no;
		agvc->month_downpower_col = fields_info[19].rdb_field_no;
		agvc->today_downpower_col= fields_info[20].rdb_field_no;
		agvc->time_downpower_col = fields_info[21].rdb_field_no;
		agvc->total_runtime_col = fields_info[22].rdb_field_no;
		agvc->total_chartime_col = fields_info[23].rdb_field_no;
		agvc->total_distime_col= fields_info[24].rdb_field_no;
		agvc->year_chartime_col = fields_info[25].rdb_field_no;
		agvc->year_distime_col = fields_info[26].rdb_field_no;
		agvc->month_chartime_col = fields_info[27].rdb_field_no;
		agvc->month_distime_col = fields_info[28].rdb_field_no;
		agvc->day_chartime_col = fields_info[29].rdb_field_no;
		agvc->day_distime_col = fields_info[30].rdb_field_no;

	

		unit_runstate_list.push_back(agvc);

	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;


}






int Cagvc_ctrl_mgr::read_micro_ctrl_info_table()
{
	for(int i=0; i<micro_ctrl_list.size(); i++)
	{
		delete micro_ctrl_list.at(i);
	}
	micro_ctrl_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;

	int offset = 0;
	int record_pos = 0;
	int record_len = 0;

	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;

	buffer = (char *)MALLOC(50*500);
	const int field_num = 11;
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*field_num);

	char  English_names[11][DB_ENG_TABLE_NAME_LEN] = {
		"display_idx",
		"obj_key_id",
		"ao_alias",
		"plc_id",
		"ctrl_data",
		"ctrl_format",
		"ctrl_type",
		"ctrl_mode",
		"yk_public_addr",
		"dot_no",
		"control_sq"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("micro_ctrl", table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取微电网控制定义表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		field_num,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取微电网控制定义表表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "微电网控制定义表没有记录！");

		return -1;
	}

	for(int i=0; i<field_num; i++)
	{
		record_len += fields_info[i].field_len;
	}

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;

		Cmicro_ctrl_info *micro_ctrl = new Cmicro_ctrl_info(data_obj);
		micro_ctrl->table_id = table_id;

		memcpy((char *)&micro_ctrl->display_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;
		memcpy((char *)&micro_ctrl->record_id, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;
		memcpy((char *)&micro_ctrl->name, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;
		memcpy((char *)&micro_ctrl->fac_id, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;
		memcpy((char *)&micro_ctrl->ctrl_data, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;	
		memcpy((char *)&micro_ctrl->ctrl_format, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		memcpy((char *)&micro_ctrl->ctrl_type, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;
		memcpy((char *)&micro_ctrl->ctrl_mode, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;
		memcpy((char *)&micro_ctrl->add_no, buffer+record_pos+offset, fields_info[8].field_len);
		offset += fields_info[8].field_len;
		memcpy((char *)&micro_ctrl->dot_no, buffer+record_pos+offset, fields_info[9].field_len);
		offset += fields_info[9].field_len;
		memcpy((char *)&micro_ctrl->control_sq, buffer+record_pos+offset, fields_info[10].field_len);
		offset += fields_info[10].field_len;

		micro_ctrl->display_id_col = fields_info[0].rdb_field_no;
		micro_ctrl->name_col = fields_info[2].rdb_field_no;
		micro_ctrl->fac_id_col = fields_info[3].rdb_field_no;
		micro_ctrl->ctrl_data_col = fields_info[4].rdb_field_no;
		micro_ctrl->ctrl_format_col = fields_info[5].rdb_field_no;
		micro_ctrl->ctrl_type_col = fields_info[6].rdb_field_no;
		micro_ctrl->ctrl_mode_col = fields_info[7].rdb_field_no;
		micro_ctrl->add_no_col = fields_info[8].rdb_field_no;
		micro_ctrl->dot_no_col = fields_info[9].rdb_field_no;
		micro_ctrl->control_sq_col = fields_info[10].rdb_field_no;

		micro_ctrl_list.push_back(micro_ctrl);
	}

	FREE((char *&)buffer);
	FREE((char *&)fields_info);

	return 1;
}

Cfac_info *Cagvc_ctrl_mgr::find_fac_from_list(int fac_id)
{
	Cfac_info *fac;
	for(int i=0; i< fac_list.size(); i++)
	{
		dnet_obj->write_log(0, 999, "i = %d, fac_group= %d", i, fac->fore_group);
		fac = fac_list.at(i);
		if(fac->fac_id == fac_id)
			return fac;

	}
	return NULL;
}

Cmeas_info *Cagvc_ctrl_mgr::find_meas_from_list(int meas_id)
{
	Cmeas_info *meas;
	for(int i=0; i< meas_list.size(); i++)
	{
		// dnet_obj->write_log(0, 999, "i = %d, fac_group= %d", i, fac->fore_group);
		meas = meas_list.at(i);
		if(meas->meas_id == meas_id)
			return meas;

	}
	return NULL;
}

Cbms_info *Cagvc_ctrl_mgr::find_bms_from_list(int display_id)
{
	Cbms_info *bms;
	for(int i=0; i< bms_list.size(); i++)
	{
		bms = bms_list.at(i);
		if(bms->display_id == display_id)
			return bms;

	}
	return NULL;
}

Cpcs_info *Cagvc_ctrl_mgr::find_pcs_from_list(int display_id)
{
	Cpcs_info *pcs;
	for(int i=0; i< pcs_list.size(); i++)
	{
		pcs = pcs_list.at(i);
		if(pcs->display_id == display_id)
			return pcs;

	}
	return NULL;

}
Cstation_info *Cagvc_ctrl_mgr::find_station_from_list(int display_id)
{
	Cstation_info *station;
	for(int i=0; i< station_list.size(); i++)
	{
		station = station_list.at(i);
		if(station->display_id == display_id)
			return station;
	}
	return NULL;
}

Cpoint_info *Cagvc_ctrl_mgr::find_point_from_list(int display_id)
{
	Cpoint_info *point;
	for(int i=0; i< point_list.size(); i++)
	{
		point = point_list.at(i);
		if(point->display_id == display_id)
			return point;

	}
	return NULL;
}

Cagvc_info* Cagvc_ctrl_mgr::find_agvc_from_list(int display_id)
{
	Cagvc_info *agvc=NULL;
	for(int i=0; i< agvc_list.size(); i++)
	{
		agvc = agvc_list.at(i);
		if(agvc->display_id == display_id)
			return agvc;

	}
	return NULL;

}

Cmicro_ctrl_info *Cagvc_ctrl_mgr::find_micro_ctrl_from_list(int display_id)
{
	Cmicro_ctrl_info *mctrl=NULL;
	for(int i=0; i< micro_ctrl_list.size(); i++)
	{
		mctrl = micro_ctrl_list.at(i);
		if(mctrl->display_id == display_id)
			return mctrl;

	}
	return NULL;
}


//查关口电量表
Cgatepower_info *Cagvc_ctrl_mgr::find_gatepower_from_list(int display_id)
{
	Cgatepower_info *gatepower=NULL;
	for(int i=0; i<gatepower_list.size(); i++)
	{
		gatepower = gatepower_list.at(i);
		if(gatepower->display_id == display_id)
			return gatepower;

	}
	return NULL;
}


//查找站月电量记录

Cstation_monthpower_info *Cagvc_ctrl_mgr::find_station_monthpower_from_list(int display_id)
{
	Cstation_monthpower_info *station_monthpower=NULL;
	for(int i=0; i<station_monthpower_list.size(); i++)
	{
		station_monthpower = station_monthpower_list.at(i);
		if(station_monthpower->display_id == display_id)
			return station_monthpower;

	}
	return NULL;
}

//查找站日电量记录

Cstation_daypower_info *Cagvc_ctrl_mgr::find_station_daypower_from_list(int display_id)
{
	Cstation_daypower_info *station_daypower=NULL;
	for(int i=0; i<station_daypower_list.size(); i++)
	{
		station_daypower = station_daypower_list.at(i);
		if(station_daypower->display_id == display_id)
			return station_daypower;

	}
	return NULL;
}



//查找储能单元月电量记录
Cunit_monthpower_info *Cagvc_ctrl_mgr::find_unit_monthpower_from_list(int display_id)
{
	Cunit_monthpower_info *unit_monthpower=NULL;
	for(int i=0; i<unit_monthpower_list.size(); i++)
	{
		unit_monthpower = unit_monthpower_list.at(i);
		if(unit_monthpower->display_id == display_id)
			return unit_monthpower;

	}
	return NULL;
}

//查找储能单元日电量记录
Cunit_daypower_info* Cagvc_ctrl_mgr::find_unit_daypower_from_list(int display_id)
{
	Cunit_daypower_info *unit_daypower=NULL;
	for(int i=0; i<unit_daypower_list.size(); i++)
	{
		unit_daypower = unit_daypower_list.at(i);
		if(unit_daypower->display_id == display_id)
			return unit_daypower;
	}
	return NULL;
}

//查找储能单元运行状态信息表
Cunit_runstate_info* Cagvc_ctrl_mgr::find_unit_runstate_from_list(int display_id)
{

	Cunit_runstate_info *unit_runstate=NULL;
	for(int i=0; i<unit_runstate_list.size(); i++)
	{
		unit_runstate= unit_runstate_list.at(i);
		if(unit_runstate->display_id == display_id)
			return unit_runstate;
	}
	return NULL;



}


//计算储能单元运行信息表中的记录总数


int Cagvc_ctrl_mgr::read_yk_send_table()
{
	yk_send_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;
	int offset = 0;
	int record_pos = 0;
	int record_len = 0;
	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;
	buffer = (char *)MALLOC(1000);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*20);

	char  English_names[7][DB_ENG_TABLE_NAME_LEN] = {
		"yk_id",
		"yk_alias",
		"fac_id",
		"send_no",
		"allow_ctrl_dest",
		"select_wait_time",
		"select_keep_time"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("yk_send",table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取遥控转发表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		7,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取遥控转发表表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "遥控转发表没有记录！");

		return -1;
	}

	record_len = fields_info[0].field_len +
		fields_info[1].field_len +
		fields_info[2].field_len +
		fields_info[3].field_len +
		fields_info[4].field_len +
		fields_info[5].field_len +
		fields_info[6].field_len;

	yk_send_struct one_yk;

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;


		memcpy((char *)&one_yk.yk_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;

		memcpy((char *)&one_yk.yk_alias, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;

		memcpy((char *)&one_yk.fac_id, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;

		memcpy((char *)&one_yk.send_no, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;

		memcpy((char *)&one_yk.allow_ctrl_dest, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;

		memcpy((char *)&one_yk.select_wait_time, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;

		memcpy((char *)&one_yk.select_keep_time, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;

		yk_send_list.push_back(one_yk);

	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}
int Cagvc_ctrl_mgr::read_yt_send_table()
{
	yt_send_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;
	int offset = 0;
	int record_pos = 0;
	int record_len = 0;
	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;
	buffer = (char *)MALLOC(1024*5);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*20);

	char  English_names[8][DB_ENG_TABLE_NAME_LEN] = {
		"yc_id",
		"yc_alias",
		"fac_id",
		"send_no",
		"max_eng_val",
		"eng_factor",
		"select_wait_time",
		"select_keep_time"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("yt_send",table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取遥调转发表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		8,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取遥调转发表表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "遥调转发表没有记录！");

		return -1;
	}

	record_len = fields_info[0].field_len +
		fields_info[1].field_len +
		fields_info[2].field_len +
		fields_info[3].field_len +
		fields_info[4].field_len +
		fields_info[5].field_len +
		fields_info[6].field_len +
		fields_info[7].field_len;

	yt_send_struct one_yt;

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;


		memcpy((char *)&one_yt.yc_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;

		memcpy((char *)&one_yt.yc_alias, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;

		memcpy((char *)&one_yt.fac_id, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;

		memcpy((char *)&one_yt.send_no, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;

		memcpy((char *)&one_yt.max_eng_val, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;

		memcpy((char *)&one_yt.eng_factor, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;

		memcpy((char *)&one_yt.select_wait_time, buffer+record_pos+offset, fields_info[6].field_len);
		offset += fields_info[6].field_len;

		memcpy((char *)&one_yt.select_keep_time, buffer+record_pos+offset, fields_info[7].field_len);
		offset += fields_info[7].field_len;

		yt_send_list.push_back(one_yt);

	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}

int Cagvc_ctrl_mgr::read_yk_define_table()
{
	yk_define_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;
	int offset = 0;
	int record_pos = 0;
	int record_len = 0;
	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;
	buffer = (char *)MALLOC(1024*5);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*35);

	char  English_names[32][DB_ENG_TABLE_NAME_LEN] = {
		"sw_id",
		"yk_alias",
		"fac_id",
		"yk_public_addr",
		"yk_no",
		"wf_sw_num",
		"sw1_id",
		"close_sw1_yx_value",
		"open_sw1_yx_value",
		"sw2_id",
		"close_sw2_yx_value",
		"open_sw2_yx_value",
		"sw3_id",
		"close_sw3_yx_value",
		"open_sw3_yx_value",
		"sw4_id",
		"close_sw4_yx_value",
		"open_sw4_yx_value",
		"sw5_id",
		"close_sw5_yx_value",
		"open_sw5_yx_value",
		"sw6_id",
		"close_sw6_yx_value",
		"open_sw6_yx_value",
		"sw7_id",
		"close_sw7_yx_value",
		"open_sw7_yx_value",
		"sw8_id",
		"close_sw8_yx_value",
		"open_sw8_yx_value",
		"if_allow_oper",
		"if_wf_lock"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("yk_define",table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取遥控定义表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		32,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取遥控定义表表数据失败");
		return -1;
	}

	if(record_num <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "遥控定义表没有记录！");

		return -1;
	}

	for(int i=0; i< 32; i++)
	{
		record_len += fields_info[i].field_len;
	}


	yk_define_struct one_yk_define;

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;


		memcpy((char *)&one_yk_define.sw_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;

		memcpy((char *)&one_yk_define.yk_alias, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;

		memcpy((char *)&one_yk_define.fac_id, buffer+record_pos+offset, fields_info[2].field_len);
		offset += fields_info[2].field_len;

		memcpy((char *)&one_yk_define.yk_public_addr, buffer+record_pos+offset, fields_info[3].field_len);
		offset += fields_info[3].field_len;

		memcpy((char *)&one_yk_define.yk_no, buffer+record_pos+offset, fields_info[4].field_len);
		offset += fields_info[4].field_len;

		memcpy((char *)&one_yk_define.wf_sw_num, buffer+record_pos+offset, fields_info[5].field_len);
		offset += fields_info[5].field_len;
		int idx = 6;
		for(int j=0; j<8; j++)
		{
			memcpy((char *)&one_yk_define.swx_id[j], buffer+record_pos+offset, fields_info[idx].field_len);
			offset += fields_info[idx].field_len;
			idx++;
			memcpy((char *)&one_yk_define.close_swx_yx_value[j], buffer+record_pos+offset, fields_info[idx].field_len);
			offset += fields_info[idx].field_len;
			idx++;
			memcpy((char *)&one_yk_define.open_swx_yx_value[j], buffer+record_pos+offset, fields_info[idx].field_len);
			offset += fields_info[idx].field_len;
			idx++;
		}
		memcpy((char *)&one_yk_define.if_allow_oper, buffer+record_pos+offset, fields_info[30].field_len);
		offset += fields_info[30].field_len;

		memcpy((char *)&one_yk_define.if_wf_lock, buffer+record_pos+offset, fields_info[31].field_len);
		offset += fields_info[31].field_len;

		yk_define_list.push_back(one_yk_define);

	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);
	return 1;
}

int Cagvc_ctrl_mgr::read_sw_info_table()
{
	sw_list.clear();

	int retcode;
	int record_num;
	int result_len;
	char *buffer = NULL;
	int offset = 0;
	int record_pos = 0;
	int record_len = 0;
	struct TABLE_HEAD_FIELDS_INFO* fields_info = NULL;
	buffer = (char *)MALLOC(1024*5);
	fields_info = (struct TABLE_HEAD_FIELDS_INFO *)MALLOC(sizeof(struct TABLE_HEAD_FIELDS_INFO)*20);

	char  English_names[2][DB_ENG_TABLE_NAME_LEN] = {
		"sw_id",
		"yx_value"
	};

	//根据设备表名称读取设备表ID
	int table_id;
	retcode = rdb_obj->get_table_id_by_table_name("sw_info",table_id);
	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取开关表表号失败");
		return -1;
	}

	retcode = rdb_obj->read_table_data_by_english_names(
		table_id,
		DNET_APP_TYPE_SCADA,
		(char(*)[DB_ENG_TABLE_NAME_LEN])English_names,
		2,
		fields_info,
		buffer,
		record_num,
		result_len);

	if(retcode <= 0)
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "读取开关表表数据失败");
		return -1;
	}

	if(record_num <= 0)  
	{
		FREE((char *&)buffer);
		FREE((char *&)fields_info);
		dnet_obj->write_log_at_once(0, 1000, "开关表没有记录！");

		return -1;
	}

	record_len = fields_info[0].field_len +
		fields_info[1].field_len;

	sw_info_struct one_sw_info;

	for(int i=0; i<record_num; i++)
	{
		record_pos = i * record_len;
		offset = 0;


		memcpy((char *)&one_sw_info.sw_id, buffer+record_pos+offset, fields_info[0].field_len);
		offset += fields_info[0].field_len;

		memcpy((char *)&one_sw_info.yx_value, buffer+record_pos+offset, fields_info[1].field_len);
		offset += fields_info[1].field_len;

		sw_list.push_back(one_sw_info);

	}
	FREE((char *&)buffer);
	FREE((char *&)fields_info);

}

int Cagvc_ctrl_mgr::find_yk_send_record(int fac_id, short send_no, yk_send_struct &yk_send)
{
	int flag = -1;
	for(int i=0; i<yk_send_list.size(); i++)
	{
		if(yk_send_list.at(i).fac_id == fac_id &&
			yk_send_list.at(i).send_no == send_no)
		{
			flag = 1;
			yk_send = yk_send_list.at(i);
			return flag;
		}
	}
	return flag;
}
int Cagvc_ctrl_mgr::find_yt_send_record(int fac_id, short send_no, yt_send_struct &yt_send)
{
	int flag = -1;
	for(int i=0; i<yt_send_list.size(); i++)
	{
		if(yt_send_list.at(i).fac_id == fac_id &&
			yt_send_list.at(i).send_no == send_no)
		{
			flag = 1;
			yt_send = yt_send_list.at(i);
			return flag;
		}
	}
	return flag;
}

int Cagvc_ctrl_mgr::find_yk_define_record(int sw_id, yk_define_struct &yk_define)
{
	int flag = -1;
	for(int i=0; i<yk_define_list.size(); i++)
	{
		if(sw_id == yk_define_list.at(i).sw_id)
		{
			flag = 1;
			yk_define = yk_define_list.at(i);
			return flag;

		}
	}
	return flag;

}



int Cagvc_ctrl_mgr::find_sw_info_record(int sw_id, sw_info_struct &sw_info)
{
	int find_flag = -1;
	for(int i=0; i<sw_list.size(); i++)
	{
		if(sw_id == sw_list.at(i).sw_id)
		{
			find_flag = 1;
			sw_info = sw_list.at(i);
			return find_flag;
		}
	}
	return find_flag;
}

int Cagvc_ctrl_mgr::recv_and_proc_dnet_report()
{
	dnet_obj->write_log(0,5005,
		"进入到接收和分析控制命令报文函数");
	int	ret_code;
	app_buf_head	dnet_app_head;


	// ret_code=dnet_obj->dnet_receive_message_one_type(dnet_app_head, dnet_recv_buf, DNET_SCADA_OPERATION_YK);

	ret_code=dnet_obj->dnet_receive_message(dnet_app_head, dnet_recv_buf);

	if(ret_code<0)
	{
		//没有接收到网络报文
		dnet_obj->write_log(0,5055,
			"本次循环,没有接收到任何网络报文");
		return	0;
	}

	dnet_obj->write_log(0,5099,
		"---------------------------------------->接收到%s机器%s进程长度%d的报文,报文类型:%d",
		dnet_app_head.rhead.sour_host,
		dnet_app_head.rhead.sour_process_name,
		dnet_app_head.rhead.length,
		dnet_app_head.rhead.report_type);
	if(dnet_app_head.rhead.length < 0)
	{
		dnet_obj->write_log(0,5105,
			"接收到%s机器%s进程的报文,长度%d非法",
			dnet_app_head.rhead.sour_host,
			dnet_app_head.rhead.sour_process_name,
			dnet_app_head.rhead.length);
		return	-1;
	}

	if(dnet_app_head.rhead.report_type == DNET_SCADA_YUAN_WANG_HE)
	{
		dnet_obj->write_log(0,5111,
			"->源网荷:接收到%s机器%s进程长度%d的报文",
			dnet_app_head.rhead.sour_host,
			dnet_app_head.rhead.sour_process_name,
			dnet_app_head.rhead.length);

		unpacket_yuan_wang_he_dnet_report(dnet_app_head);
		return 1;

	}
	else if(dnet_app_head.rhead.report_type == DNET_SCADA_OPERATION_YK)
	{
		dnet_obj->write_log(0,5111,
			"接收到%s机器%s进程长度%d的控制命令报文",
			dnet_app_head.rhead.sour_host,
			dnet_app_head.rhead.sour_process_name,
			dnet_app_head.rhead.length);

		if(strcmp(dnet_app_head.rhead.sour_process_name, "fore_cmd_manager") == 0)
		{
			//处理原始通道装置返回的反较报文
			dnet_obj->write_log(0,5111,"暂不处理装置反较信息");
		}
		else
		{


			//处理104转发通道上送的报文
			ret_code = unpacket_ctrl_cmd_dnet_report(dnet_app_head);

			if (ret_code>0 )
			{
				dnet_obj->write_log(0,5111,
					"执行反较确认,type=%d,step=%d,result=%d",cmd_define.cmd_type, cmd_define.cmd_step,0);
				packet_ctrl_cmd_dnet_report_ack(dnet_app_head, cmd_define, 0);
				return 1;
			}
			else if(ret_code >-2)
			{
				dnet_obj->write_log(0,5111,
					"执行反较确认,type=%d,step=%d,result=%d",cmd_define.cmd_type, cmd_define.cmd_step,-1);
				packet_ctrl_cmd_dnet_report_ack(dnet_app_head, cmd_define, -1);
				return 1;

			}

		}
		return 1;
	}
	dnet_obj->write_log(0,5199,
		"接收到%s机器%s进程的长度%d报文,报文类型:%d %s",
		dnet_app_head.rhead.sour_host,
		dnet_app_head.rhead.sour_process_name,
		dnet_app_head.rhead.length,
		dnet_app_head.rhead.report_type,
		"该类型报文非法,或当前程序无法识别");

	return	-1;
}

int Cagvc_ctrl_mgr::packet_ctrl_cmd_dnet_report_ack(app_buf_head dnet_app_head,ctrl_cmd_struct cmd_define, int return_stat)
{
	//组装控制确认报文
	char	send_buf[512];
	int		buf_len = 0;
	send_buf[buf_len++] = cmd_define.cmd_type;	//命令类型	1 代表是遥控
	send_buf[buf_len++] = cmd_define.asdu_type;	//Asdu类型	不需要处理，处理结果带上
	send_buf[buf_len++] = cmd_define.cmd_step;	//阶段

	//厂站ID	代表的是转发厂，也就是省调
	put_one_int_to_buffer(send_buf,cmd_define.fac_id,buf_len);
	//公共地址	一般无效 -1
	put_one_short_to_buffer(send_buf,cmd_define.addr_no,buf_len);
	//转发点号
	put_one_short_to_buffer(send_buf,cmd_define.send_no,buf_len);

	//控制目标
	if(cmd_define.cmd_type == FORE_CMD_TYPE_YK)
	{
		put_one_char_to_buffer(send_buf,cmd_define.dest_val_char,buf_len);
		dnet_obj->write_log(0,5656,
			"-->遥控目标值:%d", cmd_define.dest_val_char);
	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_INT)
	{
		put_one_short_to_buffer(send_buf,cmd_define.dest_val_short,buf_len);
		dnet_obj->write_log(0,5656,
			"-->整形设定目标值:%d", cmd_define.dest_val_short);
	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_FLOAT)
	{
		put_one_float_to_buffer(send_buf,cmd_define.dest_val_float,buf_len);
		dnet_obj->write_log(0,5656,
			"-->浮点设定目标值:%d", cmd_define.dest_val_float);
	}
	put_one_int_to_buffer(send_buf,return_stat,buf_len);

	int ret_code = dnet_obj->dnet_send_message(
		DNET_REPORT_POINT_TO_POINT,
		DNET_SCADA_OPERATION_YK,
		dnet_app_head.rhead.sour_host,
		dnet_app_head.rhead.sour_process_name,
		send_buf,
		buf_len);


	dnet_obj->write_log(0,33333,
		"-->发送控制返回报文.控制结果:%d,状态:%d",return_stat,ret_code);
	send_msg_to_scada_normal(cmd_define, 2);

	return 0;
}

int Cagvc_ctrl_mgr::send_msg_to_scada_normal(ctrl_cmd_struct cmd, char derection)
{
	//组装遥控操作记录报文消息体
	char	send_buf[512];
	int		buf_len = 0;
	on_time_t cur_time;
	on_time(&cur_time);
	put_one_time_t_to_buffer(send_buf, cur_time, buf_len);//操作时间
	put_one_int64_to_buffer(send_buf, cmd_define.ctrl_obj_id, buf_len);//对象名111
	put_one_char_to_buffer(send_buf, cmd_define.cmd_type, buf_len);//控制类型
	put_one_char_to_buffer(send_buf, cmd_define.asdu_type, buf_len);//asdu类型
	put_one_char_to_buffer(send_buf, cmd_define.cmd_step, buf_len);//控制步骤
	put_one_int_to_buffer(send_buf, cmd_define.fac_id, buf_len);//厂站id
	put_one_short_to_buffer(send_buf, cmd_define.addr_no, buf_len);//地址
	put_one_short_to_buffer(send_buf, cmd_define.send_no, buf_len);//点号
	put_one_char_to_buffer(send_buf, (char)derection, buf_len);//方向 调度下发为1，回给调度为2
	put_one_char_to_buffer(send_buf, cmd_define.cmd_type, buf_len);//告警状态  定义同控制类型
	int machine_id;
	get_local_machine_id((*rdb_obj), machine_id);
	put_one_int_to_buffer(send_buf, machine_id, buf_len);//机器id

	if(cmd_define.cmd_type == FORE_CMD_TYPE_YK)
	{
		put_one_float_to_buffer(send_buf, (float)cmd_define.dest_val_char, buf_len);//所有值用float表示
	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_INT)
	{
		put_one_float_to_buffer(send_buf, (float)cmd_define.dest_val_short, buf_len);//所有值用float表示
	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_FLOAT)
	{
		put_one_float_to_buffer(send_buf, (float)cmd_define.dest_val_float, buf_len);//所有值用float表示
	}

	int ret_code = dnet_obj->dnet_send_message(
		DNET_REPORT_POINT_TO_POINT,
		6530,
		DNET_APP_TYPE_SCADA,
		"scada_normal",
		send_buf,buf_len);

	dnet_obj->write_log(0,44444,
		"-->发送控制操作事件.状态:%d",ret_code);
	return 0;
}
int Cagvc_ctrl_mgr::unpacket_ctrl_cmd_dnet_report(app_buf_head dnet_app_head)
{
	if(dnet_app_head.rhead.length < CTRL_CMD_DEFINE_INFO_LEN)
	{
		dnet_obj->write_log(0,5511,
			"接收到%s机器%s进程的长度%d操作命令报文,%s",
			dnet_app_head.rhead.sour_host,
			dnet_app_head.rhead.sour_process_name,
			dnet_app_head.rhead.length,
			"但是报文内容的长度不足以描述报文头,非法");
		return	-2;
	}
	Cagvc_info* agvc = find_agvc_from_list(1);
	int		cur_ptr = 0;

	yk_send_struct yk_send_recv;
	yt_send_struct yt_send_recv;

	SPLIT_ON_KEY_INT64_TYPE split_key;
	int table_id;
	int record_id;
	short field_id;
	strcpy(cmd_define.gen_host, dnet_app_head.rhead.sour_host);
	strcpy(cmd_define.gen_proc, dnet_app_head.rhead.sour_process_name);


	//得到当前命令的定义信息
	get_one_char_from_buffer(dnet_recv_buf,cmd_define.cmd_type,cur_ptr);
	get_one_char_from_buffer(dnet_recv_buf,cmd_define.asdu_type,cur_ptr);
	get_one_char_from_buffer(dnet_recv_buf,cmd_define.cmd_step,cur_ptr);
	get_one_int_from_buffer(dnet_recv_buf,cmd_define.fac_id,cur_ptr);
	get_one_short_from_buffer(dnet_recv_buf,cmd_define.addr_no,cur_ptr);
	get_one_short_from_buffer(dnet_recv_buf,cmd_define.send_no,cur_ptr);

	dnet_obj->write_log(0,5656,
		"当前命令定义信息:cmd_type=%d, asdu_type=%d, cmd_step=%d, fac_id=%d, addr_no=%d, send_no=%d",
		cmd_define.cmd_type,
		cmd_define.asdu_type,
		cmd_define.cmd_step,
		cmd_define.fac_id,
		cmd_define.addr_no,
		cmd_define.send_no);
	if(cmd_define.cmd_type == FORE_CMD_TYPE_YK)
	{
		get_one_char_from_buffer(dnet_recv_buf,cmd_define.dest_val_char,cur_ptr);
		dnet_obj->write_log(0,5656,
			"遥控目标值:%d", cmd_define.dest_val_char);
	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_INT)
	{
		get_one_short_from_buffer(dnet_recv_buf,cmd_define.dest_val_short,cur_ptr);
		dnet_obj->write_log(0,5656,
			"整形设定目标值:%d", cmd_define.dest_val_short);
	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_FLOAT)
	{
		get_one_float_from_buffer(dnet_recv_buf,cmd_define.dest_val_float,cur_ptr);
		dnet_obj->write_log(0,5656,
			"浮点设定目标值:%d", cmd_define.dest_val_float);
	}

	time(&cmd_define.cmd_gen_time);
	int msg_is_ok = 0;
	if((cmd_define.cmd_type==FORE_CMD_TYPE_YK ||
		cmd_define.cmd_type==FORE_CMD_TYPE_INT ||
		cmd_define.cmd_type==FORE_CMD_TYPE_FLOAT) &&
		(cmd_define.cmd_step==FORE_CMD_STEP_SELECT ||
		cmd_define.cmd_step==FORE_CMD_STEP_CANCLE ||
		cmd_define.cmd_step==FORE_CMD_STEP_EXEC ||
		cmd_define.cmd_step==FORE_CMD_STEP_DERECT))
	{
		msg_is_ok = 1;

	}
	if(msg_is_ok !=1)
	{
		dnet_obj->write_log(0,5657,"当前报文控制命令无效");
		return -1;
	}

	//查找遥控和遥调转发表 有无此命令定义
	if(cmd_define.cmd_type == FORE_CMD_TYPE_YK)
	{
		if(find_yk_send_record(cmd_define.fac_id, cmd_define.send_no, yk_send_recv)>0)
		{
			//处理该遥控命令是EMS执行 ?转发给其他通道?
			generate_tno_key_fno_from_int64(split_key, yk_send_recv.yk_id);
			record_id = split_key.record_id;
			table_id = split_key.table_no;
			cmd_define.ctrl_obj_id = yk_send_recv.yk_id;

			dnet_obj->write_log(0,5658,"查找到该遥控命令报文的定义信息, table_id=%d,record_id=%d, fac_id=%d", table_id, record_id,cmd_define.fac_id);
			if(table_id == 1102 || table_id == 4570)  //1102-计算值表 4570-AGVC表
			{

				if(cmd_define.cmd_step == FORE_CMD_STEP_EXEC)
				{
					this->yk_send = yk_send_recv;
					if(yk_send.fac_id ==42 || yk_send.fac_id ==55)
					{
						dnet_obj->write_log(0,5657,"接收到省调通道AGC:agvc,通道%d", yk_send.fac_id);
						if(agvc->if_agc_ctrl == ESS_AGC_DENIED)
						{
							dnet_obj->write_log(0,5657,"AGC允许控制信号 为0,退出");
							return -1;
						}
						ess_dispatcher_interact_process(1);
					}else if(yk_send.fac_id ==43 || yk_send.fac_id ==56)       //地调
					{
						dnet_obj->write_log(0,5657,"接收到地调通道AVC:agvc,通道%d", yk_send.fac_id);
						if(agvc->if_agv_ctrl == ESS_AVC_DENIED)
						{
							dnet_obj->write_log(0,5657,"AVC允许控制信号 为0,退出");
							return -1;
						}
						ess_dispatcher_interact_process(2);
					}

				}
				send_msg_to_scada_normal(cmd_define, 1);
				return 1;

			}
			else if(table_id = 309) //309-开关表
			{
				dnet_obj->write_log(0,5657,"接收到调度开关表遥控");

				//                if(cmd_define.cmd_step == FORE_CMD_STEP_EXEC)
				//                {
				this->yk_send = yk_send_recv;

				forward_cmd_process();
				dnet_obj->write_log(0,5657,"接收到调度遥控:转发");
				//                }
				send_msg_to_scada_normal(cmd_define, 1);
				return 1;
			}

			return 1;
		}
		else
		{
			dnet_obj->write_log(0,5658,"接收到未在遥控转发表定义的控制命令,不处理");
			return -1;
		}

	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_INT)
	{

		if(find_yt_send_record(cmd_define.fac_id, cmd_define.send_no, yt_send_recv)>0)
		{
			//处理该遥控命令是EMS执行 ?转发给其他通道?

			generate_tno_key_fno_from_int64(split_key, yt_send_recv.yc_id);
			table_id = split_key.table_no;
			record_id = split_key.record_id;
			field_id = split_key.field_no;
			cmd_define.ctrl_obj_id = yt_send_recv.yc_id;

			dnet_obj->write_log(0,5658,"查找到参数设定报文的定义信息, table_id=%d,record_id=%d,max_eng_val=%f, yc_id:%ld",
				table_id, record_id,yt_send_recv.max_eng_val, yt_send_recv.eng_factor,
				yt_send_recv.yc_id);
			if(table_id == 4570)  //1102-计算值表 4570-AGVC表
			{
				this->yt_send = yt_send_recv;
				if(yt_send.fac_id ==42 ||yt_send.fac_id == 55)//agc
				{
					if(agvc->if_agc_ctrl == ESS_AGC_DENIED)
					{
						dnet_obj->write_log(0,5657,"AGC允许控制信号 为0,退出");
						return -1;
					}
					ess_dispatcher_interact_process(1);
				}else if(yt_send.fac_id ==43 || yt_send.fac_id == 56)//avc
				{
					if(agvc->if_agv_ctrl == ESS_AVC_DENIED)
					{
						dnet_obj->write_log(0,5657,"AVC允许控制信号 为0,退出");
						return -1;
					}
					ess_dispatcher_interact_process(2);
				}

				dnet_obj->write_log(0,5657,"接收到调度遥调:agvc,通道%d", yt_send.fac_id);

				send_msg_to_scada_normal(cmd_define, 1);
				return 1;
			}

			if(table_id == 1102)  //1102-计算值表 4570-AGVC表
			{
				float dest_val = 0;
				this->yt_send = yt_send_recv;
				if(cmd_define.asdu_type == 48)//设定值，归一化值
				{
					dest_val = cmd_define.dest_val_short*this->yt_send.max_eng_val;//归一化值转换为实际值
				}else if(cmd_define.asdu_type == 49)//设定值，工程值
				{
					dest_val = cmd_define.dest_val_float*this->yt_send.eng_factor;//设定值*工程系数
				}//设定值，标度化值

				data_obj->set_rdb_value(table_id, record_id, field_id-1, dest_val);
				dnet_obj->write_log(0,5657,"接收到设定值,写入计算值表对应字段");
				send_msg_to_scada_normal(cmd_define, 1);
				return 1;
			}
			else
			{
				dnet_obj->write_log(0,5657,"参数设定模式未定义转发哦");
				return -1;
			}
		}
		else
		{
			dnet_obj->write_log(0,5658,"接收到未在遥控转发表定义的控制命令");
			return -1;
		}

	}else if(cmd_define.cmd_type == FORE_CMD_TYPE_FLOAT)
	{
		float dest_val = 0;
		if(find_yt_send_record(cmd_define.fac_id, cmd_define.send_no, yt_send_recv)>0)
		{
			cmd_define.ctrl_obj_id = yt_send_recv.yc_id;
			this->yt_send = yt_send_recv;
			if(cmd_define.asdu_type == 48)//设定值，归一化值
			{
				dest_val = cmd_define.dest_val_short*this->yt_send.max_eng_val;//归一化值转换为实际值
			}else if(cmd_define.asdu_type == 49)//设定值，工程值
			{
				dest_val = cmd_define.dest_val_float*this->yt_send.eng_factor;//设定值*工程系数
			}//设定值，标度化值

			data_obj->set_rdb_value(table_id, record_id, field_id-1, dest_val);
			dnet_obj->write_log(0,5657,"接收到浮点设定值,写入计算值表对应字段");
			send_msg_to_scada_normal(cmd_define, 1);
		}


		return -1;
	}

	return	1;
}

int Cagvc_ctrl_mgr::unpacket_yuan_wang_he_dnet_report(app_buf_head dnet_app_head)
{
	on_time_t cmd_time;
	int cur_ptr = 0;
	int yuanwanghe_action = -1;

	Cagvc_info *yuanwanghe = find_agvc_from_list(2);
	if(yuanwanghe == NULL)
	{
		dnet_obj->write_log(0,5657,"未定义显示需要为2的agvc记录(源网荷)");
		return -1;
	}

	//得到当前命令的定义信息
	get_one_time_t_from_buffer(dnet_recv_buf,cmd_time,cur_ptr);
	get_one_int_from_buffer(dnet_recv_buf,yuanwanghe_action,cur_ptr);

	dnet_obj->write_log(0,5657,"读取报文中源网荷指令,指令值%d", yuanwanghe_action);

	if(yuanwanghe_action == 1)
	{
		//源网荷动作
		yuanwanghe->char_done = 1;
		yuanwanghe->dischar_done = 0;
		data_obj->set_rdb_value(yuanwanghe->table_id, yuanwanghe->record_id, yuanwanghe->char_done_col, yuanwanghe->char_done);
		data_obj->set_rdb_value(yuanwanghe->table_id, yuanwanghe->record_id, yuanwanghe->dischar_done_col, yuanwanghe->dischar_done);
		dnet_obj->write_log(0,5657,"源网荷动作:写入实时库char_done=%d, dischar_done = %d",  yuanwanghe->char_done, yuanwanghe->dischar_done);

	}
	else if(yuanwanghe_action == 0)
	{
		//源网荷动作
		yuanwanghe->char_done = 0;
		yuanwanghe->dischar_done = 1;
		data_obj->set_rdb_value(yuanwanghe->table_id, yuanwanghe->record_id, yuanwanghe->char_done_col, yuanwanghe->char_done);
		data_obj->set_rdb_value(yuanwanghe->table_id, yuanwanghe->record_id, yuanwanghe->dischar_done_col, yuanwanghe->dischar_done);
		dnet_obj->write_log(0,5657,"源网荷动作:写入实时库char_done=%d, dischar_done = %d",  yuanwanghe->char_done, yuanwanghe->dischar_done);
	}
	else
	{
		dnet_obj->write_log(0,5657,"接收到源网荷指令,指令值%d,非法", yuanwanghe_action);

	}

	return 1;


}

void Cagvc_ctrl_mgr::agvc_ctrl_init()
{
	Cpcs_info *pcs;
	Cmicro_ctrl_info *micro_ctrl;
	char micro_ctrl_name[64];

	int ret;

	ret = read_fac_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_fac_info_table");

	// ret = read_meas_info_table();
	// if(ret<0) return;
	//dnet_obj->write_log(0,5657,"read_meas_info_table");

	//ret = read_micro_ctrl_info_table();
	//if(ret<0) return;
	//dnet_obj->write_log(0,5657,"read_micro_ctrl_info_table");

	/* ret = read_bms_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_bms_info_table");*/


	//ret = read_pcs_info_table();   //变流器表
	//if(ret<0) return;
	//dnet_obj->write_log(0,5657,"read_pcs_info_table");
	//查找pcs有功设定对应的微电网控制定义表记录 注意以微电网控制名称作匹配项
	//查找pcs待机设定对应的微电网控制定义表记录 注意以微电网控制名称作匹配项


	ret = read_station_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_station_info_table");


	ret = read_gatepower_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_gate_power_info_table");


	ret = read_station_monthpower_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_station_monthpower_info_table");


	ret = read_station_daypower_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_station_daypower_info_table");

	ret = read_unit_runstate_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_unit_runstate_info_table");

	ret = read_unit_daypower_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_unit_daypower_info_table");


	ret = read_point_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_point_info_table");


	ret = read_agvc_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_agvc_info_table");

	ret = read_yk_send_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_yk_send_table");

	ret = read_yt_send_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_yt_send_table");

	ret = read_sw_info_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_sw_info_table");

	ret = read_yk_define_table();
	if(ret<0) return;
	dnet_obj->write_log(0,5657,"read_yk_define_table");

	//dnet_obj->write_log(0,5657,"read_sw_info_table");
}

void Cagvc_ctrl_mgr::device_list_clear()
{
	int i;

	for(i=0; i<fac_list.size(); i++)
	{
		delete fac_list.at(i);
	}

	for(i=0; i<bms_list.size(); i++)
	{
		delete bms_list.at(i);
	}

	for(i=0; i<pcs_list.size(); i++)
	{
		delete pcs_list.at(i);
	}

	for(i=0; i<station_list.size(); i++)
	{
		delete station_list.at(i);
	}

	for(i=0; i<point_list.size(); i++)
	{
		delete point_list.at(i);
	}

	for(i=0; i<agvc_list.size(); i++)
	{
		delete agvc_list.at(i);
	}

	for(i=0; i<micro_ctrl_list.size(); i++)
	{
		delete micro_ctrl_list.at(i);
	}
}

void Cagvc_ctrl_mgr::device_rdb_read()
{
	Cfac_info *fac;
	Cmeas_info *meas;
	Cbms_info *bms;
	Cpcs_info *pcs;
	Cstation_info *station;
	Cpoint_info *point;
	Cagvc_info *agvc;
	Cmicro_ctrl_info *micro_ctrl;
	int i;

	for(i=0; i<bms_list.size(); i++)
	{
		bms = bms_list.at(i);
		bms->read_bms_rdb();
	}

	for(i=0; i<meas_list.size(); i++)
	{
		meas = meas_list.at(i);
		meas->read_meas_rdb();
	}


	for(i=0; i<pcs_list.size(); i++)
	{
		pcs = pcs_list.at(i);
		pcs->read_pcs_rdb();
	}

	for(i=0; i<station_list.size(); i++)
	{
		station = station_list.at(i);
		station->read_station_rdb();
	}

	for(i=0; i<point_list.size(); i++)
	{
		point = point_list.at(i);
		point->read_point_rdb();
	}

	for(i=0; i<agvc_list.size(); i++)
	{
		agvc = agvc_list.at(i);
		agvc->read_agvc_rdb();
	}

	for(i=0; i<micro_ctrl_list.size(); i++)
	{
		micro_ctrl = micro_ctrl_list.at(i);
		micro_ctrl->read_micro_ctrl_rdb();
	}

}

void Cagvc_ctrl_mgr::distribute_power(float p_ess)  //暂时未考虑无功分配,如有需求再另行开发
{

}

void Cagvc_ctrl_mgr::ess_dispatcher_interact_process(int agvc_type)
{


}










#if 0  //测试使用
void Cagvc_ctrl_mgr::agc_ctrl_process()
{
	float p_plan;
	static float p_ess = 0;//储能实际分配功率
	float p_grid;
	float delta_p;

	//!!!直接写死,对应显示序号1的设备类型.
	Cagvc_info *agvc = this->find_agvc_from_list(1);
	Cpoint_info *point = this->find_point_from_list(1);
	Cstation_info *station = this->find_station_from_list(1);

	p_plan = agvc->agc_p_dest;

	//输入合法性检查
	if(fabsf(p_plan) > station->p_r*1000*1.2)
	{
		dnet_obj->write_log(0, 999, "有功传入参数非法");
		return;
	}
	dnet_obj->write_log(0, 999, "执行AGC调节,有功目标值:%.2f", p_plan);


	//读取并网点功率
	p_grid = point->p;


	//计算当前调节差值
	//delta_p = p_plan - p_grid; //大于0表示储能放电，小于0表示储能充电
	//    if(fabsf(delta_p) <= agvc->dead_value)
	//    {
	//        dnet_obj->write_log(0, 999, "有功目标值：%.2f, 并网点功率：%.2f,处于控制死区无需调节", p_plan, p_grid);
	//        return;
	//    }

	//    p_ess += delta_p;
	p_ess = agvc->agc_p_dest;
	p_ess = p_ess>station->p_r?station->p_r:p_ess;
	p_ess = p_ess<-station->p_r?-station->p_r:p_ess;

	distribute_power(p_ess);
}
#endif

#if 1 //现场使用
void Cagvc_ctrl_mgr::agc_ctrl_process()
{
	
}
#endif

void Cagvc_ctrl_mgr::set_all_pcs_standby()
{
	Cpcs_info *pcs;
	for(int i=0; i< pcs_list.size(); i++)
	{
		pcs = pcs_list.at(i);       
		pcs->set_standby();

	}
	dnet_obj->write_log(0, 999, "执行全站PCS待机");
}

void Cagvc_ctrl_mgr::set_all_pcs_stop()
{
	Cpcs_info *pcs;
	for(int i=0; i< pcs_list.size(); i++)
	{
		pcs = pcs_list.at(i);
		pcs->set_poweroff();

	}
	dnet_obj->write_log(0, 999, "执行全站PCS停机");
}

void Cagvc_ctrl_mgr::avc_ctrl_process()
{


}

void Cagvc_ctrl_mgr::forward_cmd_process()
{
	


}

int Cagvc_ctrl_mgr::yuanwanghe_ctrl()
{

	return 1;
}


int Cagvc_ctrl_mgr::get_duty_status()
{
	//从网络上读取本机是否值班标志
	int		retcode;

	//判断本机是否是某一应用的值班情况
	retcode = dnet_obj->judge_if_local_machine_on_duty(DNET_APP_TYPE_FORE_COMP);//必需为SCADA服务器的应用号

	switch(retcode)
	{
	case DNET_NET_FAULT:
		//本机失去与所有机器的网络联系
		return 0;
	case DNET_THIS_MACHINE_RUN_THIS_APP:
		//本机值班
		return 1;
	case DNET_OTHER_MACHINE_RUN_THIS_APP:
		//另外的机器是该应用的值班机
		return 0;
	case DNET_THIS_MACHINE_IS_THE_FISRT_RUN_THIS_APP:
		//本机是运行该应用的第一台机器
		return 1;
	case DNET_NO_MACHINE_RUN_THIS_APP:
		//未找到
		return 0;
	default:
		return 0;
	}

	return 0;
}

void Cagvc_ctrl_mgr::pcs_alarm_level_ctrl()
{

}

void Cagvc_ctrl_mgr::agvc_link_stat_check()
{

}






//函数：save_gatepower_to_dayandmonthpower_value(int num )
//输入：关口电量表编号，每一行记录，代表一个表计。
//输出：无
//作用：将关口电量表的当日上下网电量存储到整站日电量表的相关位置
//时间：[7/30/2021 LJF]

void Cagvc_ctrl_mgr::save_gatepower_to_dayanmonthpower_value(int num )
{

	on_time_t cur_time;  
	on_time(&cur_time);    //获取当前日期
	struct tm *p;
	//p=gmtime(&cur_time);
	p = on_localtime(&cur_time); //分解时间
	int  Year= p->tm_year+1900;
	int  Month =p->tm_mon+1;
	int  Day=p->tm_mday;
	int  Hour=p->tm_hour;
	int  Minute=p->tm_min;
	int  Second=p->tm_sec;

	//char  stringname[64];   //用于存放拼接后的字段名称
	//sprintf(stringname,"day%s_uppower_col",Day);  //int转字符+字段拼接

	 short dayup_col=0;
	 short daydown_col=0;	
	 dayup_col=Day+6;     //根据整站日电量表数据库中字段的域序号来算的     上网电网+6  
	 daydown_col=Day+37;  // 根据整站日电量表数据库中字段的域序号来算的    下网电网+37

	 short monthup_col=0;
	 short monthdown_col=0;
	 monthup_col=Month+8;      // 根据整站月电量表数据库中字段的域序号来算的     上网电网+8  举例 7月上网电量的域号是15  7+8=15
	 monthdown_col=Month+24;  //根据整站月电量表数据库中字段的域序号来算的       下网电网+24 举例 7月下网电量的域号是31  7+24=31

	 double value=0; 
 

	    Cgatepower_info *power = find_gatepower_from_list(TOTAL_POWER_METER);//读取第NUM行记录  //选择具体哪一个电量表

		Cstation_monthpower_info *monthpower = find_station_monthpower_from_list(Year-2020);

    	Cstation_daypower_info *daypower = find_station_daypower_from_list(Month);

    // 转存关口表中月份的用电信息 [8/2/2021 LJF]
		for (int i = 0; i < Month ; i++)
		{
			data_obj->read_rdb_value(power->table_id, power->record_id, 7+i, &value);  //关口电量表 当月上网电量域ID为7
			data_obj->set_rdb_value(monthpower ->table_id,monthpower ->record_id, monthup_col-i,value); 

			data_obj->read_rdb_value(power->table_id, power->record_id, 20+i, &value);  //关口电量表 当月下网电量域ID为20
			data_obj->set_rdb_value(monthpower ->table_id,monthpower ->record_id, monthdown_col-i,value); 
		}

    // 转存关口表中每日的用电信息 [8/2/2021 LJF]
		for (int i = 0; i < Day ; i++)
		{
	     data_obj->read_rdb_value(power->table_id, power->record_id, 33+i, &value);  //关口电量表 当日上网电量域ID为33
  	     data_obj->set_rdb_value(daypower->table_id,daypower->record_id, dayup_col-i,value); 

		 data_obj->read_rdb_value(power->table_id, power->record_id, 65+i, &value);  //关口电量表 当日下网电量域ID为65		
		 data_obj->set_rdb_value(daypower->table_id,daypower->record_id, daydown_col-i,value); 
			
		}

    // 计算季度的用电量信息 [8/2/2021 LJF]

           value= monthpower->season1_uppower+monthpower->season2_uppower+monthpower->season3_uppower+monthpower->season4_uppower;           //年上网用电量
			data_obj->set_rdb_value(monthpower ->table_id,monthpower->record_id, monthpower->year_uppower_col,value); 
		   value= monthpower->season1_downpower+monthpower->season2_downpower+monthpower->season3_downpower+monthpower->season4_downpower;    //年下网用电量
			data_obj->set_rdb_value(monthpower ->table_id,monthpower->record_id, monthpower->year_downpower_col,value); 

		   value= monthpower->month1_uppower+  monthpower->month2_uppower+   monthpower->month3_uppower;        //一季度上网用电量
		   data_obj->set_rdb_value(monthpower->table_id,monthpower->record_id, monthpower->season1_uppower_col,value); 
		   value= monthpower->month4_uppower+  monthpower->month5_uppower+   monthpower->month6_uppower;         //二季度上网用电量
		   data_obj->set_rdb_value(monthpower->table_id,monthpower->record_id, monthpower->season2_uppower_col,value); 
		   value= monthpower->month7_uppower+  monthpower->month8_uppower+   monthpower->month9_uppower;         //三季度上网用电量
		   data_obj->set_rdb_value(monthpower->table_id,monthpower ->record_id, monthpower->season3_uppower_col,value); 
		   value= monthpower->month10_uppower+  monthpower->month11_uppower+   monthpower->month12_uppower;         //四季度上网用电量
		   data_obj->set_rdb_value(monthpower->table_id,monthpower->record_id, monthpower->season4_uppower_col,value); 

		   value= monthpower->month1_downpower+  monthpower->month2_downpower+   monthpower->month3_downpower;        //一季度下网用电量
		   data_obj->set_rdb_value(monthpower ->table_id,monthpower ->record_id, monthpower->season1_downpower_col,value); 
		   value= monthpower->month4_downpower+  monthpower->month5_downpower+   monthpower->month6_downpower;         //二季度下网用电量
		   data_obj->set_rdb_value(monthpower ->table_id,monthpower ->record_id, monthpower->season2_downpower_col,value); 
		   value= monthpower->month7_downpower+  monthpower->month8_downpower+   monthpower->month9_downpower;         //三季度下网用电量
		   data_obj->set_rdb_value(monthpower ->table_id,monthpower ->record_id, monthpower->season3_downpower_col,value); 
		   value= monthpower->month10_downpower+  monthpower->month11_downpower+   monthpower->month12_downpower;         //四季度下网用电量
		   data_obj->set_rdb_value(monthpower ->table_id,monthpower ->record_id, monthpower->season4_downpower_col,value); 


	scada_report->send_all_modify_rdb();
	Sleep(1000*1);
	
		
}








void Cagvc_ctrl_mgr::save_runstate_to_unitpower_value()
{
	on_time_t cur_time;  
	on_time(&cur_time);    //获取当前日期
	struct tm *p;
	//p=gmtime(&cur_time);
	p = on_localtime(&cur_time); //分解时间
	int  Year= p->tm_year+1900;
	int  Month =p->tm_mon+1;
	int  Day=p->tm_mday;
	int  Hour=p->tm_hour;
	int  Minute=p->tm_min;
	int  Second=p->tm_sec;

	short dayup_col=0;
	short daydown_col=0;	
	dayup_col=Day+7;     //根据储能单元日电量表数据库中字段的域序号来算的     放电量
	daydown_col=Day+38;  // 根储能单元日电量表数据库中字段的域序号来算的    充电量+37
	 double value1=0; 
	 double value2=0;  

    for (int i = 1; i <=unit_runstate_list.size(); i++)
    {
	  Cunit_runstate_info *runstate = find_unit_runstate_from_list(i);
	  data_obj->read_rdb_value(runstate ->table_id, runstate ->record_id, runstate->today_downpower_col, &value1);  //储能单元日电量信息表
	  data_obj->read_rdb_value(runstate ->table_id, runstate ->record_id, runstate->today_uppower_col, &value2);  //储能单元日电量信息表
	  Cunit_daypower_info *daypower = find_unit_daypower_from_list(i);
	  data_obj->set_rdb_value(daypower->table_id,daypower->record_id, dayup_col,value1); 
	  data_obj->set_rdb_value(daypower->table_id,daypower->record_id, daydown_col,value2); 
	  scada_report->send_all_modify_rdb();
	  Sleep(1000*1);
    }







}

//设置安科瑞表函数  遥调合

int Cagvc_ctrl_mgr::set_on_ankerui(int a)
{

	Cpcs_info *pcs = find_pcs_from_list(a);
	for (int i = 0; i < 3; i++) {
		pcs->read_pcs_rdb();
		int s = pcs->on_off_stat ;
		//如果操作的指令和现在的状态一样,就不需要往数据库写命令了,否则写命令
		if (s == 1) {
			dnet_obj->write_log(0, 4459, "安科瑞表变流器表显示ID=%d的%s控制 合闸  成功----->",a,pcs->name);
			return 1;
		} else {

			pcs->set_const_pz_micro_ctrl(4096);   ///4096合闸  8192 分闸

			dnet_obj->write_log(0, 4459,"已发送命令 %d 次 ", i+1);
		}
		for(int j=0;j<30;j++){
			Sleep(1000);
			pcs->read_pcs_rdb();
			s = pcs->on_off_stat;
			if (s == 1) {
				dnet_obj->write_log(0, 4459, "安科瑞表变流器表显示ID=%d的%s控制 合闸 成功--->",a,pcs->name);
				return 1;
			}
		}
	}
	dnet_obj->write_log(0, 4459, "安科瑞表变流器表显示ID=%d的%s控制 合闸 失败---->",a,pcs->name);
	return -1;
}


//设置安科瑞表函数  遥调分
int Cagvc_ctrl_mgr::set_off_ankerui(int a)
{

	Cpcs_info *pcs = find_pcs_from_list(a);
	for (int i = 0; i < 3; i++) {
		pcs->read_pcs_rdb();
		int s = pcs->on_off_stat ;
		//如果操作的指令和现在的状态一样,就不需要往数据库写命令了,否则写命令
		if (s == 0) {
			dnet_obj->write_log(0, 4459, "安科瑞表变流器表显示ID=%d的%s控制 分闸  成功--->",a,pcs->name);
			return 1;
		} else {

			pcs->set_const_pz_micro_ctrl(8192);   ///4096合闸  8192 分闸

			dnet_obj->write_log(0, 4459,"已发送命令 %d 次 ", i+1);
		}
		for(int j=0;j<30;j++){
			Sleep(1000);
			pcs->read_pcs_rdb();
			s = pcs->on_off_stat;
			if (s == 0) {
				dnet_obj->write_log(0, 4459, "安科瑞表变流器表显示ID=%d的%s控制 分闸  成功--->",a,pcs->name);
				return 1;
			}
		}
	}
	dnet_obj->write_log(0, 4459, "安科瑞表变流器表显示ID=%d的%s控制 分闸 失败---->",a,pcs->name);
	return -1;
}














//冷库
int Cagvc_ctrl_mgr::lk_on(int a )
{
	Cpcs_info *pcs = find_pcs_from_list(a);         			
	return pcs->set_lk_onoff(1);  //开机     

}


int Cagvc_ctrl_mgr::lk_off(int a)
{                 
	Cpcs_info *pcs = find_pcs_from_list(a);       
	return  pcs->set_lk_onoff(0);  //关机  

}


int Cagvc_ctrl_mgr::lk_lowertemp(int a ,int b)  
{
	Cpcs_info *pcs = find_pcs_from_list(a); 

	return pcs->set_lk_lowertemp(b);   //设置下限温度       

}

int Cagvc_ctrl_mgr::lk_uppertemp(int a ,int b) 
{
	Cpcs_info *pcs = find_pcs_from_list(a);

	return pcs->set_lk_uppertemp(b);  //设置上限温度 

}






//485继电器 卷膜机 //继电器  D03 D05 D07 表示关闭    D04 D06 D08表示放风

 
int Cagvc_ctrl_mgr::roll1_on()     //打开通风
{	Cpcs_info *pcs = find_pcs_from_list(39);
	pcs->set_artu1_3_onoff(0);
	Sleep(1000*2);
	pcs->set_artu1_5_onoff(0);  
	Sleep(1000*2);
	pcs->set_artu1_7_onoff(0);  
	Sleep(1000*2);
	pcs->set_artu1_4_onoff(1); 
	Sleep(1000*2);
	pcs->set_artu1_6_onoff(1);  
	Sleep(1000*2);
	pcs->set_artu1_8_onoff(1);  
	Sleep(1000*2);
	return 1;
}



int Cagvc_ctrl_mgr::roll1_off()  //关闭
{
	Cpcs_info *pcs = find_pcs_from_list(39);

	pcs->set_artu1_4_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu1_6_onoff(0);  
	Sleep(1000*2);
	pcs->set_artu1_8_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu1_3_onoff(1); 
	Sleep(1000*2);
	pcs->set_artu1_5_onoff(1);  
	Sleep(1000*2);
	pcs->set_artu1_7_onoff(1); 
	Sleep(1000*2);
	return 1;
}


//继电器  D03 D05 D07 表示关闭    D04 D06 D08表示放风

int Cagvc_ctrl_mgr::roll2_on()    //放风
{	Cpcs_info *pcs = find_pcs_from_list(40);
	pcs->set_artu2_3_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu2_5_onoff(0);  
	Sleep(1000*2);
	pcs->set_artu2_7_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu2_4_onoff(1); 
	Sleep(1000*2);
	pcs->set_artu2_6_onoff(1); 
	Sleep(1000*2);
	pcs->set_artu2_8_onoff(1);  
	return 1;
}



int Cagvc_ctrl_mgr::roll2_off() ////关闭
{
	Cpcs_info *pcs = find_pcs_from_list(40);

	pcs->set_artu2_4_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu2_6_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu2_8_onoff(0); 
	Sleep(1000*2);
	pcs->set_artu2_3_onoff(1); 
	Sleep(1000*2);
	pcs->set_artu2_5_onoff(1);  
	Sleep(1000*2);
	pcs->set_artu2_7_onoff(1); 
	return 1;
}






























//写电站表
//注释与说明
//运行状态：1停止态;2并网态;3离网态;4故障态 （电站额定容量）  station->cap_r,
//运行状态信息：1离网启动中;2并网启动中;3离网停机中;4并网停机中;5无缝离网中;6无缝并网中;7故障复归中  （额定功率） station->p_r,
//运行模式：0,手动模式;1,自动模式   （间隔数量） station->bay_num ,
//1停机指令，2并网指令，3离网指令   （运行间隔） station->run_bay_num ,
//故障复归1                                （停运间隔）  station->stop_bay_num ,
//基础运行控制：1选择，0没选 （电池簇数量）  station->bats_num,  
//离网运行管理：1选择，0没选   （PCS数量）    station->pcs_num,
//离网策略选择：0没选，1离网功率平衡控制  （bms数量） station->bms_num,
//并网运行管理：1选择，0没选  （可放电量）     station->ava_kwh_disc,
//并网策略选择：0没选，1并网功率平衡控制，2削峰填谷策略控制, 3并网经济运行管理 （可充电量 ）  station->ava_kwh_char);


//清除  1停机指令，2并网指令，3离网指令  指令
int Cagvc_ctrl_mgr:: clearCMD()  
{
	set_Station_ACGridCmd(0); 
	return 1;

}
int Cagvc_ctrl_mgr:: clearFaultReset()  //清除表中 故障复归命令
{
	set_Station_FaultReset(0);
	return 1;
}



int Cagvc_ctrl_mgr:: set_Station_RunState(float a)   //运行状态：1停止态;2并网态;3离网态;4故障态  
{

	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->cap_r_col, (float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);

	return 1;

}

int Cagvc_ctrl_mgr:: set_Station_RunStateInfo(float a)  // 运行状态信息：1离网启动中;2并网启动中;3离网停机中;4并网停机中;5无缝离网中;6无缝并网中;7故障复归中
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,  station->p_r_col, (float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	  
	return 1;

}

int Cagvc_ctrl_mgr:: set_Station_RunMode(int a)       //  运行模式：0,手动模式;1,自动模式
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,  station->bay_num_col, (int)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	  
	return 1;

}

int Cagvc_ctrl_mgr:: set_Station_ACGridCmd(int a)      //   1停机指令，2并网指令，3离网指令
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,   station->run_bay_num_col, (int)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	  	
	return 1;

}
int Cagvc_ctrl_mgr:: set_Station_FaultReset(int a)    //     故障复归1
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,  station->stop_bay_num_col, (int)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	 
	return 1;

}
int Cagvc_ctrl_mgr:: set_Station_BasicCtrl(int a)      //   基础运行控制：1选择，0没选
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,   station->bats_num_col, (int)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);
	return 1;

}
int Cagvc_ctrl_mgr:: set_Station_OffgridManage(int a)   //    离网运行管理：1选择，0没选
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,  station->pcs_num_col, (int)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);
	return 1;

}
int Cagvc_ctrl_mgr:: set_Station_OffgridMSelect( int a)  //    离网策略选择：0没选，1离网功率平衡控制
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,  station->bms_num_col, (int)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);
	return 1;

}
int Cagvc_ctrl_mgr:: set_Station_OngridManage(float a)    //    并网运行管理：1选择，0没选
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id,   station->day_ongrid_energy_col, (float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);
	return 1;

}
int Cagvc_ctrl_mgr:: set_Station_OngridMSelect(float a)  //  并网策略选择：0没选，1并网功率平衡控制，2削峰填谷策略控制, 3并网经济运行管理
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->day_downgrid_energy_col, (float)a);  
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);
	return 1;

}





//////////////////////////////////
//电站表设置杨北变工作选择
//策略选择：1恒压，2是小功率
//手动设置运行模式


float Cagvc_ctrl_mgr:: red_YB_GridState()     //读YB系统状态 2离网 3并网
{
	Cstation_info *station = find_station_from_list(1);//电站表 
	station->read_station_rdb();
	return station->soc_low;

}

int Cagvc_ctrl_mgr:: Set_YB_GritState(float a)    
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->soc_low_col,(float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	
	return 1;

}



int Cagvc_ctrl_mgr:: set_YB_RunModet(float a) 
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->soc_total_col,(float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	
	return 1;

}

//读YB手动设置的运行状态
float Cagvc_ctrl_mgr:: read_YB_RunModet()
{
	Cstation_info *station = find_station_from_list(1);//电站表 
	station->read_station_rdb();
	return station->soc_total;

}

// 保存YB设置的运行模式
int Cagvc_ctrl_mgr:: Save_YB_RunModet(float a)  // 保存设置的运行模式
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->soc_up_col,(float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	
	return 1;

}

//写松降头系数
int Cagvc_ctrl_mgr:: set_sjt_percent(float a)  //设系数
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->ava_kwh_disc_col,(float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	
	return 1;

}

//读杨北系数
int Cagvc_ctrl_mgr::set_yb_percent(float a) 
{
	Cstation_info *station = find_station_from_list(1); 
	data_obj->set_rdb_value(station->table_id, station->record_id, station->ava_kwh_char_col,(float)a); 
	scada_report->send_all_modify_rdb();
	Sleep(1000*2);	
	return 1;

}




//读杨北系数
float Cagvc_ctrl_mgr:: read_yb_percent() ///可充电量    //杨北平衡系数  （0 ~1） 默认0.5
{
	Cstation_info *station = find_station_from_list(1);//电站表 
	station->read_station_rdb();
	float a=0.5;
	float percent = station->ava_kwh_char;
	if(percent>0 && percent <=1 )
	{return station->ava_kwh_char;}
	else{
		//回写
		data_obj->set_rdb_value(station->table_id, station->record_id, station->ava_kwh_char_col,(float)a); 
		scada_report->send_all_modify_rdb();
		Sleep(1000*1);
		//   set_yb_percent(a); 
		return a;
	}
}



















void Cagvc_ctrl_mgr::main_loop()
{

	agvc_ctrl_init();
	dnet_obj->write_log(0,5657,"程序初始化");

	while(1)	  
	{
		save_gatepower_to_dayanmonthpower_value(1);   

	    save_runstate_to_unitpower_value();
	}



}

