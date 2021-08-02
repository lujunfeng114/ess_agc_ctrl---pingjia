#include "device.h"



Cbms_info::Cbms_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;

}

Cbms_info::~Cbms_info()
{

}

int Cbms_info::read_bms_rdb()
{
    dnet_obj->write_log_at_once(100, 1000, "read_bms_rdb:dnet_obj=%x, rdb_obj=%x, data_obj=%x",
                                dnet_obj, rdb_obj, data_obj);
    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, soc_min_col, &soc_min);
    data_obj->read_rdb_value(table_id, record_id, soc_max_col, &soc_max);
    data_obj->read_rdb_value(table_id, record_id, soc_now_col, &soc_now);
    data_obj->read_rdb_value(table_id, record_id, kwh_col, &kwh);
    data_obj->read_rdb_value(table_id, record_id, ava_char_col, &ava_char);
    data_obj->read_rdb_value(table_id, record_id, ava_dischar_col, &ava_disc);
    //对SOC进行合理阈值检查  防止误算越界,调度考核soc相当严格
    this->soc_now = this->soc_now > 100 ? 100 : this->soc_now;
    this->soc_now = this->soc_now < 0 ? 0 : this->soc_now;
	return 0;
}

Cpcs_info::Cpcs_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;
}

Cpcs_info::~Cpcs_info()
{

}

int Cpcs_info::read_pcs_rdb()
{
    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, p_col, &p);
    data_obj->read_rdb_value(table_id, record_id, q_col, &q);
    data_obj->read_rdb_value(table_id, record_id, p_max_char_col, &p_max_char);
   //dnet_obj->write_log_at_once(0, 999, "read_pcs_rdb:p_max_char:%.3f", p_max_char);

    data_obj->read_rdb_value(table_id, record_id, p_max_disc_col, &p_max_disc);
    //dnet_obj->write_log_at_once(0, 999, "read_pcs_rdb:p_max_disc:%.3f", p_max_disc);
    data_obj->read_rdb_value(table_id, record_id, if_use_col, &if_use);
    data_obj->read_rdb_value(table_id, record_id, fault_total_col, &fault_total);
    data_obj->read_rdb_value(table_id, record_id, ctrl_mode_col, &ctrl_mode);
    data_obj->read_rdb_value(table_id, record_id, run_mode_col, &run_mode);
    data_obj->read_rdb_value(table_id, record_id, ctrl_condition_col, &ctrl_condition);
    data_obj->read_rdb_value(table_id, record_id, on_off_stat_col, &on_off_stat);
    data_obj->read_rdb_value(table_id, record_id, on_off_grid_stat_col, &on_off_grid_stat);
    data_obj->read_rdb_value(table_id, record_id, standby_stat_col, &standby_stat);
    data_obj->read_rdb_value(table_id, record_id, q_max_char_col, &q_max_char);
    data_obj->read_rdb_value(table_id, record_id, q_max_disc_col, &q_max_disc);
    data_obj->read_rdb_value(table_id, record_id, capacity_col, &capacity);

    data_obj->read_rdb_value(table_id, record_id, alarm_level_1_col, &alarm_level_1); 
    data_obj->read_rdb_value(table_id, record_id, alarm_level_2_col, &alarm_level_2);
    data_obj->read_rdb_value(table_id, record_id, alarm_level_3_col, &alarm_level_3);


    data_obj->read_rdb_value(table_id, record_id, alarm_level_1_act_col, &alarm_level_1_act);  
    data_obj->read_rdb_value(table_id, record_id, alarm_level_2_act_col, &alarm_level_2_act);
    data_obj->read_rdb_value(table_id, record_id, alarm_level_3_act_col, &alarm_level_3_act);

	data_obj->read_rdb_value(table_id, record_id, ac_ua_col, &ac_ua);
	data_obj->read_rdb_value(table_id, record_id, ac_ub_col, &ac_ub);
	data_obj->read_rdb_value(table_id, record_id, ac_uc_col, &ac_uc);

	return 0;
}

bool Cpcs_info::check_status()
{
    //fault_tatal:结合了PCS和BMS的通讯故障,在公示定义表合成

    if(if_use == 1 &&\
            fault_total == 0 &&\
            ctrl_mode == 2 )    //控制模式 远程就地状态 + 并网状态 只有是2,才具备发送条件
    {
        ctrl_condition = 1; //当前PCS受控,未考虑储能SOC的因素,会在try_power_to_pcs中集中考虑

        data_obj->set_rdb_value(table_id, record_id, ctrl_condition_col, ctrl_condition);
        return true;
    }
    else
    {   dnet_obj->write_log(0, 1000, "-->%s不满足控制条件if_use=%d,fault_total=%d,ctrl_mode=%d",
                            this->name, this->if_use,this->fault_total, this->ctrl_mode);
        ctrl_condition = 0; //当前PCS不可控
        data_obj->set_rdb_value(table_id, record_id, ctrl_condition_col, ctrl_condition);
        return false;
    }
}

bool Cpcs_info::check_power_status(float p_ess)
{
    if(p_ess <= 0) //对充电功率分配,剔除已经充到上限的正常PCS
    {
        this->try_power_to_pcs(this->p_max_char, 0);
        if(fabsf(this->p_set) < 0.01)
        {
            dnet_obj->write_log(0, 1222, "%s不具备功率调节能力 充电", this->name);
            return false;
        }
    }
    else            //对放电功率分配,剔除已经充到下限的正常PCS
    {
        this->try_power_to_pcs(this->p_max_disc, 0);
        if(fabsf(this->p_set) < 0.01)
        {
            dnet_obj->write_log(0, 1222, "%s不具备功率调节能力 放电", this->name);
            return false;
        }
    }

    return true;
}



////////////////////////////////////以下PCS变流器表中设备的基础控制函数//////////////////////////////
//卢俊峰201903 
//
//    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;    遥调用和字符搭配
//    cmd.cmd_step = FORE_CMD_STEP_DERECT;   直接控制  变流器表中的都要用直接控制 切记
//    cmd.cmd_step = FORE_CMD_STEP_EXEC;     预制控制  开关表中用
//    cmd.dest_val_float = v;               遥调下发的值
//    cmd.cmd_type = FORE_CMD_TYPE_YK;  遥控需要加这一句 遥调不需要
//    我们的PCS 0开机  1关机 故在开机遥控中的时候要注意  （此处我们的PCS在变流器表中显示ID=12根据ID进行选择何种）
//if(cmd2 == 12)
//	{
//		cmd.dest_val_char = FORE_CMD_YK_OFF;   //1 对应104 中0  PCS下0开机
////		
//	}
//	else
//	{
//		cmd.dest_val_char = FORE_CMD_YK_ON;    //2 对应104中 1 其他设备下1开机
//	}
//
////////////////////////////////////////////////////////////////////////


//基础函数
//通用型遥控函数


//通用遥控函数
int Cpcs_info::set_YK(Cmicro_ctrl_info *yk_micro_ctrl)
{
 if(yk_micro_ctrl == NULL)
    {
	dnet_obj->write_log(0, 526, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;
	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = yk_micro_ctrl->fac_id;
	cmd.addr_no = yk_micro_ctrl->add_no;
	cmd.send_no = yk_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
   //cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	cmd.dest_val_char = FORE_CMD_YK_ON;    //设备下1开机   下0开机的另类 自己写
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
		dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下遥控命令失败", this->display_id,this->name);
		return -1;
	}
   
	
	
	}

//通用遥调函数
int Cpcs_info::set_YT(Cmicro_ctrl_info *yt_micro_ctrl,float value)
{
if(yt_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = yt_micro_ctrl->fac_id;
    cmd.addr_no = yt_micro_ctrl->add_no;
    cmd.send_no = yt_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = value;
    Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发遥调命令失败", this->display_id,this->name);
        return -1;
    }
		
}

/////////////////////////////////////以上基础函数/////////////////////////////////////

/////////////////////////////////以下能量路由器控制函数///  是不是快的一逼！！///////////////////////////////////////////


//路由器左侧并网开关
int Cpcs_info::set_router_left_ongird_onoff(float value)
{

return set_YT(this->yt_router_left_ongird_onoff_micro_ctrl,value);
	
	}


//路由器右侧并网开关
int Cpcs_info::set_router_right_ongird_onoff(float value)
{
return set_YT(this->yt_router_right_ongird_onoff_micro_ctrl,value);
	
	}



//路由器DC直流接触器开关
int Cpcs_info:: set_router_dconoff(float value)
{
return set_YT(this->yt__router_dconoff_micro_ctrl,value);
	}


//左侧稳压开关
int Cpcs_info:: set_router_left_voltage_onoff(float value)
{
return set_YT(this->yt__router_left_voltage_onoff_micro_ctrl,value);
	
	}

	
//右侧稳压开关
int Cpcs_info::set_router_right_voltage_onoff(float value)
{
return set_YT(this->yt__router_right_voltage_onoff_micro_ctrl,value);
	
	}


//设置电压
int Cpcs_info:: set_router_voltage(float value)  //设电压
{
return set_YT(this->yt_router_voltage_micro_ctrl,value);
	
	}

//功率设置
int Cpcs_info:: set_router_power(float value)  //设置功率
{
return set_YT(this->yt_router_power_micro_ctrl,value);
	
	}

//路由器故障复归
int Cpcs_info:: set_router_removefault(float value)
{
return set_YT(this->yt_router_removefault_micro_ctrl,value);
	
	}


///////////////////////////////青禾二期///////////////////////////////////////////


int Cpcs_info:: set_artu1_3_onoff(float value)
{
return set_YT(this->yt_artu1_3_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu1_4_onoff(float value)
{
return set_YT(this->yt_artu1_4_micro_ctrl,value);
	
}

int Cpcs_info:: set_artu1_5_onoff(float value)
{
return set_YT(this->yt_artu1_5_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu1_6_onoff(float value)
{
return set_YT(this->yt_artu1_6_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu1_7_onoff(float value)
{
return set_YT(this->yt_artu1_7_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu1_8_onoff(float value)
{
return set_YT(this->yt_artu1_8_micro_ctrl,value);
	
}

int Cpcs_info:: set_artu2_3_onoff(float value)
{
return set_YT(this->yt_artu2_3_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu2_4_onoff(float value)
{
return set_YT(this->yt_artu2_4_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu2_5_onoff(float value)
{
return set_YT(this->yt_artu2_5_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu2_6_onoff(float value)
{
return set_YT(this->yt_artu2_6_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu2_7_onoff(float value)
{
return set_YT(this->yt_artu2_7_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu2_8_onoff(float value)
{
return set_YT(this->yt_artu2_8_micro_ctrl,value);
	
}


int Cpcs_info:: set_artu3_3_onoff(float value)
{
return set_YT(this->yt_artu3_3_micro_ctrl,value);
	
}
int Cpcs_info:: set_artu3_4_onoff(float value)
{
return set_YT(this->yt_artu3_4_micro_ctrl,value);
	
}

int Cpcs_info:: set_light1_onoff(float value)
{
  return   set_YT(this->yt_light1_micro_ctrl,value);

	
}
int Cpcs_info:: set_light2_onoff(float value)
{
return set_YT(this->yt_light2_micro_ctrl,value);
	
}

int Cpcs_info:: set_dtsd1_onoff(float value)         //dtsd
{
return set_YT(this->yt_dtsd1_micro_ctrl,value);
	
}
int Cpcs_info:: set_dtsd2_onoff(float value)         //dtsd
{
return set_YT(this->yt_dtsd2_micro_ctrl,value);
	
}




int Cpcs_info:: set_rb1_onoff(float value)          //热泵开关机
{
return set_YT(this->yt_rb1_onoff_micro_ctrl,value);
	
}
int Cpcs_info:: set_rb1_cooltemp(float value)          //设热泵制冷温度
{
return set_YT(this->yt_rb1_cooltemp_micro_ctrl,value);
	
}
int Cpcs_info:: set_rb1_hottemp(float value)         //设热泵制热温度
{
return set_YT(this->yt_rb1_hottemp_micro_ctrl,value);
	
}



int Cpcs_info:: set_rb2_onoff(float value)          //热泵开关机
{
return set_YT(this->yt_rb2_onoff_micro_ctrl,value);
	
}
int Cpcs_info:: set_rb2_cooltemp(float value)          //设热泵制冷温度
{
return set_YT(this->yt_rb2_cooltemp_micro_ctrl,value);
	
}
int Cpcs_info:: set_rb2_hottemp(float value)         //设热泵制热温度
{
return set_YT(this->yt_rb2_hottemp_micro_ctrl,value);
	
}


int Cpcs_info:: set_rb3_onoff(float value)          //热泵开关机
{
return set_YT(this->yt_rb3_onoff_micro_ctrl,value);
	
}
int Cpcs_info:: set_rb3_cooltemp(float value)          //设热泵制冷温度
{
return set_YT(this->yt_rb3_cooltemp_micro_ctrl,value);
	
}
int Cpcs_info:: set_rb3_hottemp(float value)         //设热泵制热温度
{
return set_YT(this->yt_rb3_hottemp_micro_ctrl,value);
	
}


///电磁阀

int Cpcs_info::  set_artu4_3_onoff(float value)   //电磁阀1~6
{
return set_YT(this->yt_water_3_micro_ctrl,value);
	
}
int Cpcs_info::  set_artu4_4_onoff(float value)
{
return set_YT(this->yt_water_4_micro_ctrl,value);
	
}
int Cpcs_info::  set_artu4_5_onoff(float value)
{
return set_YT(this->yt_water_5_micro_ctrl,value);
	
}
int Cpcs_info::  set_artu4_6_onoff(float value)
{
return set_YT(this->yt_water_6_micro_ctrl,value);
	
}
int Cpcs_info::  set_artu4_7_onoff(float value)
{
return set_YT(this->yt_water_7_micro_ctrl,value);
	
}
int Cpcs_info::  set_artu4_8_onoff(float value)
{
return set_YT(this->yt_water_8_micro_ctrl,value);
	
}




int Cpcs_info:: set_lk_onoff(float value)        //冷库开关机
{
return set_YT(this->yt_lkonoff_micro_ctrl,value);
	
}

int Cpcs_info:: set_lk_lowertemp(float value)         //设温度下限
{
return set_YT(this->yt_lklowertemp_micro_ctrl,value);
	
}

int Cpcs_info:: set_lk_uppertemp(float value)          //设温度上限
{
return set_YT(this->yt_lkuppertemp_micro_ctrl,value);
	
}










//以下已经有的大量重复代码的函数就不改了，以后都建议用通用函数来写///////////////////////
//PCS有功充电
int Cpcs_info::set_const_p_power_chong(float p)
{
    if(yt_p_chong_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = this->yt_p_chong_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_p_chong_micro_ctrl->add_no;
    cmd.send_no = this->yt_p_chong_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = p;
    Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发有功失败", this->display_id,this->name);
        return -1;
    }
}


 //PCS放电	
int Cpcs_info::set_const_p_power_fang(float p)
{
	if(yt_p_fang_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = this->yt_p_fang_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_p_fang_micro_ctrl->add_no;
    cmd.send_no = this->yt_p_fang_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = p;
  Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发有功失败", this->display_id,this->name);
        return -1;
    }
}
//PCS恒压浮充电压设定
int Cpcs_info::set_const_dc(int v)
{
	if(yt_voltage_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = this->yt_voltage_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_voltage_micro_ctrl->add_no;
    cmd.send_no = this->yt_voltage_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = v;
     Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发有功失败", this->display_id,this->name);
        return -1;
    }
}
//VF 电压值
int Cpcs_info::set_const_vf_ac(int v)
{
	if(yt_vf_ac_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = this->yt_vf_ac_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_vf_ac_micro_ctrl->add_no;
    cmd.send_no = this->yt_vf_ac_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = v;
    Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发有功失败", this->display_id,this->name);
        return -1;
    }
}
//VF频率
int Cpcs_info::set_const_vf_hz(int h)
{
	if(yt_vf_hz_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = this->yt_vf_hz_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_vf_hz_micro_ctrl->add_no;
    cmd.send_no = this->yt_vf_hz_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = h;
   Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发有功失败", this->display_id,this->name);
        return -1;
    }
}
//PCS无功调节
int Cpcs_info::set_const_q_power(int q)
{
    if(yt_q_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表中定义%d号显示ID设备%s有功设定记录,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->yt_q_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_q_micro_ctrl->add_no;
    cmd.send_no = this->yt_q_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = q;
   Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发无功失败", this->display_id,this->name);
        return -1;
    }

}



//PZ遥调
		//卢俊峰 20190307 青禾 新增
	  // 作为PZ的遥调控制开关  4096合 8192分

int Cpcs_info::set_const_pz_micro_ctrl(int q)
{
    if(yt_pz_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表中定义%d号%s-PZ的遥调控制开关,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->yt_pz_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_pz_micro_ctrl->add_no;
    cmd.send_no = this->yt_pz_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = q;
   	 Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号%s的遥调控制开关", this->display_id,this->name);
        return -1;
    }

}



//作为AEM表的遥调控制开关 分

int Cpcs_info::set_const_aemfen_micro_ctrl(int q)
{
    if(yt_aemfen_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表中定义%d号%s 分的遥调控制开关,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->yt_aemfen_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_aemfen_micro_ctrl->add_no;
    cmd.send_no = this->yt_aemfen_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = q;
   	 Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号%s  分的遥调控制开关", this->display_id,this->name);
        return -1;
    }
}

//作为AEM表的遥调控制开关 合

int Cpcs_info::set_const_aemhe_micro_ctrl(int q)
{
    if(yt_aemfen_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表中定义%d号%s 合的遥调控制开关,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->yt_aemfen_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_aemfen_micro_ctrl->add_no;
    cmd.send_no = this->yt_aemfen_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = q;
   	 Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号%s  合的遥调控制开关", this->display_id,this->name);
        return -1;
    }
}

//作为sun光伏逆变器的开机
int Cpcs_info::set_const_sunon_micro_ctrl(int q)
{
    if(yt_sunon_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表中定义%d#号%s光伏并网开机 的遥调控制开关,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->yt_sunon_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_sunon_micro_ctrl->add_no;
    cmd.send_no = this->yt_sunon_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.dest_val_float = q;
    	 Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d#号%s光伏并网开机 的遥调控制开关", this->display_id,this->name);
        return -1;
    }
}

//作为sun光伏逆变器的关机
int Cpcs_info::set_const_sunoff_micro_ctrl(int q)
{
    if(yt_sunoff_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表中定义%d#号%s 光伏关机 的遥调控制开关,无法调节", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->yt_sunoff_micro_ctrl->fac_id;
    cmd.addr_no = this->yt_sunoff_micro_ctrl->add_no;
    cmd.send_no = this->yt_sunoff_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_FLOAT;
    //cmd.cmd_step = FORE_CMD_STEP_DERECT;
    cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
    cmd.dest_val_float = q;
    	 Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d#号%s 光伏关机 的遥调控制开关", this->display_id,this->name);
        return -1;
    }
}



//PCS待机
int Cpcs_info::set_standby()
{
    if( yk_standby_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s遥控记录,无法遥控", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_standby_micro_ctrl->fac_id;
    cmd.addr_no = this->yk_standby_micro_ctrl->add_no;
    cmd.send_no = this->yk_standby_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_YK;
   //cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
    cmd.dest_val_char = FORE_CMD_YK_ON;
	 Sleep(1000*1);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
       dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发待机失败", this->display_id,this->name);
        return -1;
    }
}

//开机
int Cpcs_info::set_poweron()
{
    if(yk_poweron_micro_ctrl == NULL)
    {
	dnet_obj->write_log(0, 526, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_poweron_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_poweron_micro_ctrl->add_no;
	cmd.send_no = this->yk_poweron_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
  //cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
		
	if(cmd2 == 12)
	{
		cmd.dest_val_char = FORE_CMD_YK_OFF;   //1 对应104 中0  PCS下0开机
		
	}
	else
	{
		cmd.dest_val_char = FORE_CMD_YK_ON;    //2 对应104中 1 其他设备下1开机
	}
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
		dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发开机失败", this->display_id,this->name);
		return -1;
	}

}

//关机
int Cpcs_info::set_poweroff()
{
    if(yk_poweroff_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_poweroff_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_poweroff_micro_ctrl->add_no;
	cmd.send_no = this->yk_poweroff_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;  //预制控制  建议在开关表中用
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	/*if(cmd2 < 13)
	{
		cmd.dest_val_char = FORE_CMD_YK_ON;
	}
	else
	{
		cmd.dest_val_char = FORE_CMD_YK_OFF;
	}*/

	cmd.dest_val_char = FORE_CMD_YK_ON;  //2 对应104中 1  

	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
		dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发关机失败", this->display_id,this->name);
		return -1;
	}
}



//PCS同期
int Cpcs_info::set_tongqi()
{
    if(yk_tongqi_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "设置PCS同期,未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_tongqi_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_tongqi_micro_ctrl->add_no;
	cmd.send_no = this->yk_tongqi_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;  //预制控制  建议在开关表中用
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	cmd.dest_val_char = FORE_CMD_YK_ON;  //2 对应104中 1  

	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
			dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发同期失败", this->display_id,this->name);
		return -1;
	}
}







//PCS放电使能
int Cpcs_info::set_power_fang()
{
	if(yk_p_fang_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_p_fang_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_p_fang_micro_ctrl->add_no;
	cmd.send_no = this->yk_p_fang_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	/*if(cmd2 < 20)
	{
		cmd.dest_val_char = FORE_CMD_YK_ON;
	}
	else
	{
		cmd.dest_val_char = FORE_CMD_YK_OFF;
	}*/
    cmd.dest_val_char = FORE_CMD_YK_ON;
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
			dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发放电使能失败", this->display_id,this->name);
		return -1;
	}
}
//PCS充电使能
int Cpcs_info::set_power_chong()
{
	if(yk_p_chong_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "设置PCS关机,未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_p_chong_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_p_chong_micro_ctrl->add_no;
	cmd.send_no = this->yk_p_chong_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制

	//if(cmd2 < 13)
	//{
	//	cmd.dest_val_char = FORE_CMD_YK_ON;
	//}
	//else
	//{
	//	cmd.dest_val_char = FORE_CMD_YK_OFF;
	//}
    cmd.dest_val_char = FORE_CMD_YK_ON;
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
		dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发充电使能失败", this->display_id,this->name);
		return -1;
	}
}
//PCS恒压浮充使能
int Cpcs_info::set_const_voltage_dc()
{
	if(yk_voltage_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_voltage_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_voltage_micro_ctrl->add_no;
	cmd.send_no = this->yk_voltage_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	/*if(cmd2 < 13)
	{
		cmd.dest_val_char = FORE_CMD_YK_ON;
	}
	else
	{
		cmd.dest_val_char = FORE_CMD_YK_OFF;
	}*/
	cmd.dest_val_char = FORE_CMD_YK_ON;
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
			dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发恒压浮充使能失败", this->display_id,this->name);
		return -1;
	}
}
//PCS故障复归使能
int Cpcs_info::set_failure_recovery()
{
	if(yk_failure_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_failure_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_failure_micro_ctrl->add_no;
	cmd.send_no = this->yk_failure_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	/*if(cmd2 < 13)
	{
		cmd.dest_val_char = FORE_CMD_YK_ON;
	}
	else
	{
		cmd.dest_val_char = FORE_CMD_YK_OFF;
	}*/
	cmd.dest_val_char = FORE_CMD_YK_ON;
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
			dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发故障复归使能使能失败", this->display_id,this->name);
		return -1;
	}
}
//PCS离网VF使能
int Cpcs_info::set_off_grid()
{
	if(yk_vf_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "设置PCS关机,未在微电网控制定义表定义%d号显示ID设备%s设定值记录,无法调节", this->display_id,this->name);
        return -1;
    }
	int cmd2 = this->display_id;

	ctrl_cmd_struct cmd;
	cmd.table_id = this->table_id;
	cmd.record_id = this->record_id;
	cmd.fac_id = this->yk_vf_micro_ctrl->fac_id;
	cmd.addr_no = this->yk_vf_micro_ctrl->add_no;
	cmd.send_no = this->yk_vf_micro_ctrl->dot_no;
	cmd.cmd_type = FORE_CMD_TYPE_YK;
	//cmd.cmd_step = FORE_CMD_STEP_EXEC;
	cmd.cmd_step = FORE_CMD_STEP_DERECT ; //直接控制
	/*if(cmd2 < 13)
	{
		cmd.dest_val_char = FORE_CMD_YK_ON;
	}
	else
	{
		cmd.dest_val_char = FORE_CMD_YK_OFF;
	}*/
	cmd.dest_val_char = FORE_CMD_YK_ON;
	 Sleep(1000*1);
	if(data_obj->send_cmd_to_fore_manager(cmd)>0)
	{
		return 1;
	}
	else
	{
			dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发离网VF使能失败", this->display_id,this->name);
		return -1;
	}
}



////////////////////////////////////////////////////////// 以下未用到//////////////////////////////////////////////////////////


int Cpcs_info::emergency_reset()
{
    if(emergency_reset_micro_ctrl == NULL)
    {
        dnet_obj->write_log(0, 1000, "设置PCS源网荷紧急复归,未在微电网控制定义表定义%d号显示ID设备%s紧急复归记录,无法控制", this->display_id,this->name);
        return -1;
    }
    ctrl_cmd_struct cmd;
    cmd.table_id = this->table_id;
    cmd.record_id = this->record_id;
    cmd.fac_id = this->emergency_reset_micro_ctrl->fac_id;
    cmd.addr_no = this->emergency_reset_micro_ctrl->add_no;
    cmd.send_no = this->emergency_reset_micro_ctrl->dot_no;
    cmd.cmd_type = FORE_CMD_TYPE_YK;
    cmd.cmd_step = FORE_CMD_STEP_EXEC;
    cmd.dest_val_char = FORE_CMD_YK_ON;
    Sleep(1000*50);
    if(data_obj->send_cmd_to_fore_manager(cmd)>0)
    {

        return 1;
    }
    else
    {
        dnet_obj->write_log(0, 1000, "发送控制命令报文失败,%d号显示ID设备%s下发源网荷紧急复归失败", this->display_id,this->name);
        return -1;
    }
}
void Cpcs_info::try_power_to_pcs(float p_try, float q_try)
{
    float p_max_disc = this->p_max_disc;
    float p_max_char = this->p_max_char;

    dnet_obj->write_log(0, 1000, "%s 尝试分配有功:%.3f,无功:%.3f", this->name, p_try, q_try);

    if(check_status() == false)
    {
        dnet_obj->write_log(0, 1000, "%s 不满足功率分配条件", this->name);
        this->p_set = 0;
        this->q_set = 0;

        return;
    }

    //处理三个级别告警  //逻辑有点麻烦,木的屌办法,直接流水账写了
    //出现3级告警,直接PCS停机    出现2级告警,对应方向上的功率值置0  出现1级告警,对应方向上的功率值下调一个比例
    //充电方向  放电方向     合成值
    //  0       0         00
    //  0       1         01
    //  1       0         10
    //  1       1         11
    //处理一级告警
    if(this->alarm_level_1 % 10 == 1)//一级故障放电方向
    {
        p_max_disc = this->p_max_disc * this->alarm_level_1_act/100;
        dnet_obj->write_log(0, 1000, "%s 处于一级放电告警,有功下发百分比:%d\%", this->name, this->alarm_level_1_act);

    }
    if(this->alarm_level_1 / 10 == 1)//一级故障充电方向
    {
        p_max_char = this->p_max_char * this->alarm_level_1_act/100;
        dnet_obj->write_log(0, 1000, "%s 处于一级充电告警,有功下发百分比:%d\%", this->name, this->alarm_level_1_act);
    }

    //处理二级告警
    if(this->alarm_level_2 % 10 == 1)//二级故障放电方向
    {
        p_max_disc = this->p_max_disc * this->alarm_level_2_act/100;
        dnet_obj->write_log(0, 1000, "%s 处于二级放电告警,有功下发百分比:%d\%", this->name, this->alarm_level_2_act);

    }
    if(this->alarm_level_2 / 10 == 1)//二级故障充电方向
    {
        p_max_char = this->p_max_char * this->alarm_level_2_act/100;
        dnet_obj->write_log(0, 1000, "%s 处于二级充电告警,有功下发百分比:%d\%", this->name, this->alarm_level_2_act);
    }

    //处理三级告警
    if(this->alarm_level_3 != 0)
    {
        this->p_set = 0;
        this->q_set = 0;
        this->set_poweroff();
        dnet_obj->write_log(0, 1000, "%s 处于三级告警,有功无功下发0,下发关机动作", this->name);
        return ;
    }


    //处于恒功率模式处理
    //处理有功
    p_try = p_try>p_max_disc ? p_max_disc:p_try;
    p_try = p_try<p_max_char ? p_max_char:p_try;

    //处理无功
    q_try = q_try>q_max_disc ? this->q_max_disc:q_try;
    q_try = q_try<q_max_char ? this->q_max_char:q_try;

    //按照有功优先 处理无功输出
    float q_max_cal;
    if(q_try<0)
    {
       q_max_cal = -sqrt(this->capacity*this->capacity - this->p*this->p);
       q_try = q_try < q_max_cal? q_max_cal:q_try;
    }
    else
    {
        q_max_cal = sqrt(this->capacity*this->capacity - this->p*this->p);
        q_try = q_try > q_max_cal? q_max_cal:q_try;
    }


    this->p_set = p_try;
    this->q_set = q_try;

    //处理有功过充/过放
    if(this->p_set >0 && (this->bms->soc_now <= this->bms->soc_min)) //过放
    {
        this->p_set = 0;
        dnet_obj->write_log(0, 1000, "%s 过放,功率下0", this->name);
    }else if(this->p_set <0 && (this->bms->soc_now >= this->bms->soc_max)) //过充
    {
        this->p_set  =0;
        dnet_obj->write_log(0, 1000, "%s 过充,功率下0", this->name);
    }


    dnet_obj->write_log(0, 1000, "%s 实际分配有功:%.3f,无功:%.3f",
                        this->name, this->p_set, this->q_set);

    return;
}






///////////////////////////////////////以下回读刷新函数 根据需要添加和修改//////////////////////////////////////
//常用PCS表和station表


Cstation_info::Cstation_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;
}

Cstation_info::~Cstation_info()
{

}

int Cstation_info::read_station_rdb()
{
    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, cap_r_col, &cap_r);
    data_obj->read_rdb_value(table_id, record_id, p_r_col, &p_r);


    data_obj->read_rdb_value(table_id, record_id, run_bay_num_col, &run_bay_num);
    data_obj->read_rdb_value(table_id, record_id, stop_bay_num_col, &stop_bay_num);

    data_obj->read_rdb_value(table_id, record_id, plt_run_status_col, &plt_run_status);
    data_obj->read_rdb_value(table_id, record_id, ava_kwh_disc_col, &ava_kwh_disc);
    data_obj->read_rdb_value(table_id, record_id, ava_kwh_char_col, &ava_kwh_char);
    data_obj->read_rdb_value(table_id, record_id, soc_total_col, &soc_total);
    data_obj->read_rdb_value(table_id, record_id, soc_up_col, &soc_up);
    data_obj->read_rdb_value(table_id, record_id, soc_low_col, &soc_low);

    data_obj->read_rdb_value(table_id, record_id, plt_run_mod_col, &plt_run_mod);
    data_obj->read_rdb_value(table_id, record_id, latitude_col, &latitude);
    data_obj->read_rdb_value(table_id, record_id, longtitude_col, &longtitude);

    data_obj->read_rdb_value(table_id, record_id, status_col, &status);
	data_obj->read_rdb_value(table_id, record_id, bay_num_col, &bay_num);
    data_obj->read_rdb_value(table_id, record_id, bats_num_col, &bats_num);
    data_obj->read_rdb_value(table_id, record_id, pcs_num_col, &pcs_num);

	//新增 
	data_obj->read_rdb_value(table_id, record_id, bms_num_col, &bms_num);
    data_obj->read_rdb_value(table_id, record_id, day_ongrid_energy_col, &day_ongrid_energy);
	data_obj->read_rdb_value(table_id, record_id, day_downgrid_energy_col, &day_downgrid_energy);
	return 0;
}


Cpoint_info::Cpoint_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;


}

Cpoint_info::~Cpoint_info()
{

}

int Cpoint_info::read_point_rdb()
{
    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, p_col, &p);
    data_obj->read_rdb_value(table_id, record_id, q_col, &q);
    data_obj->read_rdb_value(table_id, record_id, cos_col, &cos);
    data_obj->read_rdb_value(table_id, record_id, p_kwh_col, &p_kwh);
    data_obj->read_rdb_value(table_id, record_id, pn_kwh_col, &pn_kwh);
    data_obj->read_rdb_value(table_id, record_id, q_kvarh_col, &q_kvarh);
    data_obj->read_rdb_value(table_id, record_id, qn_kvarh_col, &qn_kvarh);

	return 0;
}

Cagvc_info::Cagvc_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;

}

Cagvc_info::~Cagvc_info()
{

}

int Cagvc_info::read_agvc_rdb()
{
    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, total_fault_col, &total_fault);
    data_obj->read_rdb_value(table_id, record_id, char_done_col, &char_done);
    data_obj->read_rdb_value(table_id, record_id, dischar_done_col, &dischar_done);
    data_obj->read_rdb_value(table_id, record_id, if_agc_ctrl_col, &if_agc_ctrl);
    data_obj->read_rdb_value(table_id, record_id, agc_remote_mode_col, &agc_remote_mode);
    data_obj->read_rdb_value(table_id, record_id, char_lock_col, &char_lock);
    data_obj->read_rdb_value(table_id, record_id, dischar_lock_col, &dischar_lock);
    data_obj->read_rdb_value(table_id, record_id, ask_agc_on_col, &ask_agc_on);
    data_obj->read_rdb_value(table_id, record_id, ask_agc_status_col, &ask_agc_status);
    data_obj->read_rdb_value(table_id, record_id, agc_p_dest_col, &agc_p_dest);
    data_obj->read_rdb_value(table_id, record_id, agc_p_return_col, &agc_p_return);
    data_obj->read_rdb_value(table_id, record_id, p_max_char_col, &p_max_char);
    data_obj->read_rdb_value(table_id, record_id, p_max_dischar_col, &p_max_dischar);
    data_obj->read_rdb_value(table_id, record_id, p_max_dischar_time_col, &p_max_dischar_time);
    data_obj->read_rdb_value(table_id, record_id, p_max_char_time_col, &p_max_char_time);
    data_obj->read_rdb_value(table_id, record_id, dead_value_col, &dead_value);
    data_obj->read_rdb_value(table_id, record_id, ctrl_interval_col, &ctrl_interval);

    data_obj->read_rdb_value(table_id, record_id, if_agv_ctrl_col, &if_agv_ctrl);
    data_obj->read_rdb_value(table_id, record_id, ask_avc_on_col, &ask_avc_on);
    data_obj->read_rdb_value(table_id, record_id, ask_avc_status_col, &ask_avc_status);
    data_obj->read_rdb_value(table_id, record_id, avc_remote_mode_col, &avc_remote_mode);
    data_obj->read_rdb_value(table_id, record_id, add_q_lock_col, &add_q_lock);
    data_obj->read_rdb_value(table_id, record_id, reduce_q_lock_col, &reduce_q_lock);
    data_obj->read_rdb_value(table_id, record_id, avc_u_q_ctrl_mode_col, &avc_u_q_ctrl_mode);
    data_obj->read_rdb_value(table_id, record_id, avc_add_q_col, &avc_add_q);
    data_obj->read_rdb_value(table_id, record_id, avc_reduce_q_col, &avc_reduce_q);

    data_obj->read_rdb_value(table_id, record_id, avc_u_dest_col, &avc_u_dest);
    data_obj->read_rdb_value(table_id, record_id, avc_u_ref_col, &avc_u_ref);
    data_obj->read_rdb_value(table_id, record_id, avc_q_dest_col, &avc_q_dest);
    data_obj->read_rdb_value(table_id, record_id, avc_q_ref_col, &avc_q_ref);

    data_obj->read_rdb_value(table_id, record_id, agc_chanel_stat_col, &agc_chanel_stat);
    data_obj->read_rdb_value(table_id, record_id, avc_chanel_stat_col, &avc_chanel_stat);
    data_obj->read_rdb_value(table_id, record_id, agc_last_modify_time_col, &agc_last_modify_time);
    data_obj->read_rdb_value(table_id, record_id, avc_last_modify_time_col, &avc_last_modify_time);
	return 0;
}



Cmicro_ctrl_info::Cmicro_ctrl_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;

}

Cmicro_ctrl_info::~Cmicro_ctrl_info()
{

}

int Cmicro_ctrl_info::read_micro_ctrl_rdb()
{

    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, fac_id_col, &fac_id);
	data_obj->read_rdb_value(table_id, record_id, ctrl_data_col, &ctrl_data);
    data_obj->read_rdb_value(table_id, record_id, ctrl_format_col, &ctrl_format);
    data_obj->read_rdb_value(table_id, record_id, ctrl_type_col, &ctrl_type);
    data_obj->read_rdb_value(table_id, record_id, ctrl_mode_col, &ctrl_mode);
    data_obj->read_rdb_value(table_id, record_id, add_no_col, &add_no);
    data_obj->read_rdb_value(table_id, record_id, dot_no_col, &dot_no);
    data_obj->read_rdb_value(table_id, record_id, control_sq_col, &control_sq);


	return 0;
}

Cfac_info::Cfac_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;

}

Cfac_info::~Cfac_info()
{

}

int Cfac_info::read_fac_rdb()
{
    data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, if_allow_yk_col, &if_allow_yk);
    data_obj->read_rdb_value(table_id, record_id, fore_group_col, &fore_group);
	return 0;
}

Cmeas_info::Cmeas_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;

}

Cmeas_info::~Cmeas_info()
{

}

int Cmeas_info::read_meas_rdb()
{
	data_obj->read_rdb_value(table_id, record_id, meas_id_col, &meas_id);
    data_obj->read_rdb_value(table_id, record_id, name_col, &name);
    data_obj->read_rdb_value(table_id, record_id, meas_value_col, &meas_value);
	return 0;
}



//20210727 卢俊峰  关口电量表操作
Cgatepower_info::Cgatepower_info(Cdata_access *_data_obj)
{
    this->data_obj = _data_obj;
    this->rdb_obj = data_obj->rdb_obj;
    this->dnet_obj = data_obj->dnet_obj;

}

Cgatepower_info::~Cgatepower_info()
{

}

int Cgatepower_info::read_gatepower_rdb()
{
    data_obj->read_rdb_value(table_id, record_id, display_id_col, &display_id);
	data_obj->read_rdb_value(table_id, record_id, name_col, name);
    data_obj->read_rdb_value(table_id, record_id, month_col, &month);
    data_obj->read_rdb_value(table_id, record_id, thismonth_uppower_col, &thismonth_uppower);
    data_obj->read_rdb_value(table_id, record_id, last1_uppower_col, &last1_uppower);
    data_obj->read_rdb_value(table_id, record_id, last2_uppower_col, &last2_uppower);
    data_obj->read_rdb_value(table_id, record_id, last3_uppower_col, &last3_uppower);
    data_obj->read_rdb_value(table_id, record_id, last4_uppower_col, &last4_uppower);
    data_obj->read_rdb_value(table_id, record_id, last5_uppower_col, &last5_uppower);
    data_obj->read_rdb_value(table_id, record_id, last6_uppower_col, &last6_uppower);
    data_obj->read_rdb_value(table_id, record_id, last7_uppower_col, &last7_uppower);
    data_obj->read_rdb_value(table_id, record_id, last8_uppower_col, &last8_uppower);
    data_obj->read_rdb_value(table_id, record_id, last9_uppower_col, &last9_uppower);
    data_obj->read_rdb_value(table_id, record_id, last10_uppower_col, &last10_uppower);
    data_obj->read_rdb_value(table_id, record_id, last11_uppower_col, &last11_uppower);
    data_obj->read_rdb_value(table_id, record_id, last12_uppower_col, &last12_uppower);

    data_obj->read_rdb_value(table_id, record_id, thismonth_downpower_col, &thismonth_downpower);
    data_obj->read_rdb_value(table_id, record_id, last1_downpower_col, &last1_downpower);
    data_obj->read_rdb_value(table_id, record_id, last2_downpower_col, &last2_downpower);
    data_obj->read_rdb_value(table_id, record_id, last3_downpower_col, &last3_downpower);
    data_obj->read_rdb_value(table_id, record_id, last4_downpower_col, &last4_downpower);
    data_obj->read_rdb_value(table_id, record_id, last5_downpower_col, &last5_downpower);
    data_obj->read_rdb_value(table_id, record_id, last6_downpower_col, &last6_downpower);
    data_obj->read_rdb_value(table_id, record_id, last7_downpower_col, &last7_downpower);
    data_obj->read_rdb_value(table_id, record_id, last8_downpower_col, &last8_downpower);
    data_obj->read_rdb_value(table_id, record_id, last9_downpower_col, &last9_downpower);
    data_obj->read_rdb_value(table_id, record_id, last10_downpower_col, &last10_downpower);
    data_obj->read_rdb_value(table_id, record_id, last11_downpower_col, &last11_downpower);
    data_obj->read_rdb_value(table_id, record_id, last12_downpower_col, &last12_downpower);


    data_obj->read_rdb_value(table_id, record_id, today_uppower_col, &today_uppower);
    data_obj->read_rdb_value(table_id, record_id, day1_uppower_col, &day1_uppower);
    data_obj->read_rdb_value(table_id, record_id, day2_uppower_col, &day2_uppower);
    data_obj->read_rdb_value(table_id, record_id, day3_uppower_col, &day3_uppower);
    data_obj->read_rdb_value(table_id, record_id, day4_uppower_col, &day4_uppower);
    data_obj->read_rdb_value(table_id, record_id, day5_uppower_col, &day5_uppower);
    data_obj->read_rdb_value(table_id, record_id, day6_uppower_col, &day6_uppower);
    data_obj->read_rdb_value(table_id, record_id, day7_uppower_col, &day7_uppower);
    data_obj->read_rdb_value(table_id, record_id, day8_uppower_col, &day8_uppower);
    data_obj->read_rdb_value(table_id, record_id, day9_uppower_col, &day9_uppower);
    data_obj->read_rdb_value(table_id, record_id, day10_uppower_col, &day10_uppower);
    data_obj->read_rdb_value(table_id, record_id, day11_uppower_col, &day11_uppower);
    data_obj->read_rdb_value(table_id, record_id, day12_uppower_col, &day12_uppower);
    data_obj->read_rdb_value(table_id, record_id, day13_uppower_col, &day13_uppower);
    data_obj->read_rdb_value(table_id, record_id, day14_uppower_col, &day14_uppower);
    data_obj->read_rdb_value(table_id, record_id, day15_uppower_col, &day15_uppower);
    data_obj->read_rdb_value(table_id, record_id, day16_uppower_col, &day16_uppower);
    data_obj->read_rdb_value(table_id, record_id, day17_uppower_col, &day17_uppower);
    data_obj->read_rdb_value(table_id, record_id, day18_uppower_col, &day18_uppower);
    data_obj->read_rdb_value(table_id, record_id, day19_uppower_col, &day19_uppower);
    data_obj->read_rdb_value(table_id, record_id, day20_uppower_col, &day20_uppower);
    data_obj->read_rdb_value(table_id, record_id, day21_uppower_col, &day21_uppower);
    data_obj->read_rdb_value(table_id, record_id, day22_uppower_col, &day22_uppower);
    data_obj->read_rdb_value(table_id, record_id, day23_uppower_col, &day23_uppower);
    data_obj->read_rdb_value(table_id, record_id, day24_uppower_col, &day24_uppower);
    data_obj->read_rdb_value(table_id, record_id, day25_uppower_col, &day25_uppower);
    data_obj->read_rdb_value(table_id, record_id, day26_uppower_col, &day26_uppower);
    data_obj->read_rdb_value(table_id, record_id, day27_uppower_col, &day27_uppower);
    data_obj->read_rdb_value(table_id, record_id, day28_uppower_col, &day28_uppower);
    data_obj->read_rdb_value(table_id, record_id, day29_uppower_col, &day29_uppower);
    data_obj->read_rdb_value(table_id, record_id, day30_uppower_col, &day30_uppower);
    data_obj->read_rdb_value(table_id, record_id, day31_uppower_col, &day31_uppower);

    data_obj->read_rdb_value(table_id, record_id, today_downpower_col, &today_downpower);
    data_obj->read_rdb_value(table_id, record_id, day1_downpower_col, &day1_downpower);
    data_obj->read_rdb_value(table_id, record_id, day2_downpower_col, &day2_downpower);
    data_obj->read_rdb_value(table_id, record_id, day3_downpower_col, &day3_downpower);
    data_obj->read_rdb_value(table_id, record_id, day4_downpower_col, &day4_downpower);
    data_obj->read_rdb_value(table_id, record_id, day5_downpower_col, &day5_downpower);
    data_obj->read_rdb_value(table_id, record_id, day6_downpower_col, &day6_downpower);
    data_obj->read_rdb_value(table_id, record_id, day7_downpower_col, &day7_downpower);
    data_obj->read_rdb_value(table_id, record_id, day8_downpower_col, &day8_downpower);
    data_obj->read_rdb_value(table_id, record_id, day9_downpower_col, &day9_downpower);
    data_obj->read_rdb_value(table_id, record_id, day10_downpower_col, &day10_downpower);
    data_obj->read_rdb_value(table_id, record_id, day11_downpower_col, &day11_downpower);
    data_obj->read_rdb_value(table_id, record_id, day12_downpower_col, &day12_downpower);
    data_obj->read_rdb_value(table_id, record_id, day13_downpower_col, &day13_downpower);
    data_obj->read_rdb_value(table_id, record_id, day14_downpower_col, &day14_downpower);
    data_obj->read_rdb_value(table_id, record_id, day15_downpower_col, &day15_downpower);
    data_obj->read_rdb_value(table_id, record_id, day16_downpower_col, &day16_downpower);
    data_obj->read_rdb_value(table_id, record_id, day17_downpower_col, &day17_downpower);
    data_obj->read_rdb_value(table_id, record_id, day18_downpower_col, &day18_downpower);
    data_obj->read_rdb_value(table_id, record_id, day19_downpower_col, &day19_downpower);
    data_obj->read_rdb_value(table_id, record_id, day20_downpower_col, &day20_downpower);
    data_obj->read_rdb_value(table_id, record_id, day21_downpower_col, &day21_downpower);
    data_obj->read_rdb_value(table_id, record_id, day22_downpower_col, &day22_downpower);
    data_obj->read_rdb_value(table_id, record_id, day23_downpower_col, &day23_downpower);
    data_obj->read_rdb_value(table_id, record_id, day24_downpower_col, &day24_downpower);
    data_obj->read_rdb_value(table_id, record_id, day25_downpower_col, &day25_downpower);
    data_obj->read_rdb_value(table_id, record_id, day26_downpower_col, &day26_downpower);
    data_obj->read_rdb_value(table_id, record_id, day27_downpower_col, &day27_downpower);
    data_obj->read_rdb_value(table_id, record_id, day28_downpower_col, &day28_downpower);
    data_obj->read_rdb_value(table_id, record_id, day29_downpower_col, &day29_downpower);
    data_obj->read_rdb_value(table_id, record_id, day30_downpower_col, &day30_downpower);
    data_obj->read_rdb_value(table_id, record_id, day31_downpower_col, &day31_downpower);



	return 0;
}


//20210729 卢俊峰  整站月电量表操作
Cstation_monthpower_info::Cstation_monthpower_info(Cdata_access *_data_obj)
{
	this->data_obj = _data_obj;
	this->rdb_obj = data_obj->rdb_obj;
	this->dnet_obj = data_obj->dnet_obj;

}

Cstation_monthpower_info::~Cstation_monthpower_info()
{

}

int Cstation_monthpower_info::read_station_monthpower_rdb()
{
	data_obj->read_rdb_value(table_id, record_id, display_id_col, &display_id);
	data_obj->read_rdb_value(table_id, record_id, name_col, name);
	data_obj->read_rdb_value(table_id, record_id, year_col, &year);

    data_obj->read_rdb_value(table_id, record_id, year_uppower_col, &year_uppower);
    data_obj->read_rdb_value(table_id, record_id, year_downpower_col, &year_downpower);

	data_obj->read_rdb_value(table_id, record_id, month1_uppower_col, &month1_uppower);
	data_obj->read_rdb_value(table_id, record_id, month2_uppower_col, &month2_uppower);
	data_obj->read_rdb_value(table_id, record_id, month3_uppower_col, &month3_uppower);
	data_obj->read_rdb_value(table_id, record_id, month4_uppower_col, &month4_uppower);
	data_obj->read_rdb_value(table_id, record_id, month5_uppower_col, &month5_uppower);
	data_obj->read_rdb_value(table_id, record_id, month6_uppower_col, &month6_uppower);
	data_obj->read_rdb_value(table_id, record_id, month7_uppower_col, &month7_uppower);
	data_obj->read_rdb_value(table_id, record_id, month8_uppower_col, &month8_uppower);
	data_obj->read_rdb_value(table_id, record_id, month9_uppower_col, &month9_uppower);
	data_obj->read_rdb_value(table_id, record_id, month10_uppower_col, &month10_uppower);
	data_obj->read_rdb_value(table_id, record_id, month11_uppower_col, &month11_uppower);
	data_obj->read_rdb_value(table_id, record_id, month12_uppower_col, &month12_uppower);

	data_obj->read_rdb_value(table_id, record_id, season1_uppower_col, &season1_uppower);
	data_obj->read_rdb_value(table_id, record_id, season2_uppower_col, &season2_uppower);
	data_obj->read_rdb_value(table_id, record_id, season3_uppower_col, &season3_uppower);
	data_obj->read_rdb_value(table_id, record_id, season4_uppower_col, &season4_uppower);

	data_obj->read_rdb_value(table_id, record_id, month1_downpower_col, &month1_downpower);
	data_obj->read_rdb_value(table_id, record_id, month2_downpower_col, &month2_downpower);
	data_obj->read_rdb_value(table_id, record_id, month3_downpower_col, &month3_downpower);
	data_obj->read_rdb_value(table_id, record_id, month4_downpower_col, &month4_downpower);
	data_obj->read_rdb_value(table_id, record_id, month5_downpower_col, &month5_downpower);
	data_obj->read_rdb_value(table_id, record_id, month6_downpower_col, &month6_downpower);
	data_obj->read_rdb_value(table_id, record_id, month7_downpower_col, &month7_downpower);
	data_obj->read_rdb_value(table_id, record_id, month8_downpower_col, &month8_downpower);
	data_obj->read_rdb_value(table_id, record_id, month9_downpower_col, &month9_downpower);
	data_obj->read_rdb_value(table_id, record_id, month10_downpower_col, &month10_downpower);
	data_obj->read_rdb_value(table_id, record_id, month11_downpower_col, &month11_downpower);
	data_obj->read_rdb_value(table_id, record_id, month12_downpower_col, &month12_downpower);

	data_obj->read_rdb_value(table_id, record_id, season1_downpower_col, &season1_downpower);
	data_obj->read_rdb_value(table_id, record_id, season2_downpower_col, &season2_downpower);
	data_obj->read_rdb_value(table_id, record_id, season3_downpower_col, &season3_downpower);
	data_obj->read_rdb_value(table_id, record_id, season4_downpower_col, &season4_downpower);

	return 0;
}

//20210729 卢俊峰  整站日电量表操作
Cstation_daypower_info::Cstation_daypower_info(Cdata_access *_data_obj)
{
	this->data_obj = _data_obj;
	this->rdb_obj = data_obj->rdb_obj;
	this->dnet_obj = data_obj->dnet_obj;

}

Cstation_daypower_info::~Cstation_daypower_info()
{

}

int Cstation_daypower_info::read_station_daypower_rdb()
{

	data_obj->read_rdb_value(table_id, record_id, display_id_col, &display_id);
	data_obj->read_rdb_value(table_id, record_id, name_col, name);
	data_obj->read_rdb_value(table_id, record_id, month_col, &month);
	data_obj->read_rdb_value(table_id, record_id, day1_uppower_col, &day1_uppower);
	data_obj->read_rdb_value(table_id, record_id, day2_uppower_col, &day2_uppower);
	data_obj->read_rdb_value(table_id, record_id, day3_uppower_col, &day3_uppower);
	data_obj->read_rdb_value(table_id, record_id, day4_uppower_col, &day4_uppower);
	data_obj->read_rdb_value(table_id, record_id, day5_uppower_col, &day5_uppower);
	data_obj->read_rdb_value(table_id, record_id, day6_uppower_col, &day6_uppower);
	data_obj->read_rdb_value(table_id, record_id, day7_uppower_col, &day7_uppower);
	data_obj->read_rdb_value(table_id, record_id, day8_uppower_col, &day8_uppower);
	data_obj->read_rdb_value(table_id, record_id, day9_uppower_col, &day9_uppower);
	data_obj->read_rdb_value(table_id, record_id, day10_uppower_col, &day10_uppower);
	data_obj->read_rdb_value(table_id, record_id, day11_uppower_col, &day11_uppower);
	data_obj->read_rdb_value(table_id, record_id, day12_uppower_col, &day12_uppower);
	data_obj->read_rdb_value(table_id, record_id, day13_uppower_col, &day13_uppower);
	data_obj->read_rdb_value(table_id, record_id, day14_uppower_col, &day14_uppower);
	data_obj->read_rdb_value(table_id, record_id, day15_uppower_col, &day15_uppower);
	data_obj->read_rdb_value(table_id, record_id, day16_uppower_col, &day16_uppower);
	data_obj->read_rdb_value(table_id, record_id, day17_uppower_col, &day17_uppower);
	data_obj->read_rdb_value(table_id, record_id, day18_uppower_col, &day18_uppower);
	data_obj->read_rdb_value(table_id, record_id, day19_uppower_col, &day19_uppower);
	data_obj->read_rdb_value(table_id, record_id, day20_uppower_col, &day20_uppower);
	data_obj->read_rdb_value(table_id, record_id, day21_uppower_col, &day21_uppower);
	data_obj->read_rdb_value(table_id, record_id, day22_uppower_col, &day22_uppower);
	data_obj->read_rdb_value(table_id, record_id, day23_uppower_col, &day23_uppower);
	data_obj->read_rdb_value(table_id, record_id, day24_uppower_col, &day24_uppower);
	data_obj->read_rdb_value(table_id, record_id, day25_uppower_col, &day25_uppower);
	data_obj->read_rdb_value(table_id, record_id, day26_uppower_col, &day26_uppower);
	data_obj->read_rdb_value(table_id, record_id, day27_uppower_col, &day27_uppower);
	data_obj->read_rdb_value(table_id, record_id, day28_uppower_col, &day28_uppower);
	data_obj->read_rdb_value(table_id, record_id, day29_uppower_col, &day29_uppower);
	data_obj->read_rdb_value(table_id, record_id, day30_uppower_col, &day30_uppower);
	data_obj->read_rdb_value(table_id, record_id, day31_uppower_col, &day31_uppower);

	data_obj->read_rdb_value(table_id, record_id, day1_downpower_col, &day1_downpower);
	data_obj->read_rdb_value(table_id, record_id, day2_downpower_col, &day2_downpower);
	data_obj->read_rdb_value(table_id, record_id, day3_downpower_col, &day3_downpower);
	data_obj->read_rdb_value(table_id, record_id, day4_downpower_col, &day4_downpower);
	data_obj->read_rdb_value(table_id, record_id, day5_downpower_col, &day5_downpower);
	data_obj->read_rdb_value(table_id, record_id, day6_downpower_col, &day6_downpower);
	data_obj->read_rdb_value(table_id, record_id, day7_downpower_col, &day7_downpower);
	data_obj->read_rdb_value(table_id, record_id, day8_downpower_col, &day8_downpower);
	data_obj->read_rdb_value(table_id, record_id, day9_downpower_col, &day9_downpower);
	data_obj->read_rdb_value(table_id, record_id, day10_downpower_col, &day10_downpower);
	data_obj->read_rdb_value(table_id, record_id, day11_downpower_col, &day11_downpower);
	data_obj->read_rdb_value(table_id, record_id, day12_downpower_col, &day12_downpower);
	data_obj->read_rdb_value(table_id, record_id, day13_downpower_col, &day13_downpower);
	data_obj->read_rdb_value(table_id, record_id, day14_downpower_col, &day14_downpower);
	data_obj->read_rdb_value(table_id, record_id, day15_downpower_col, &day15_downpower);
	data_obj->read_rdb_value(table_id, record_id, day16_downpower_col, &day16_downpower);
	data_obj->read_rdb_value(table_id, record_id, day17_downpower_col, &day17_downpower);
	data_obj->read_rdb_value(table_id, record_id, day18_downpower_col, &day18_downpower);
	data_obj->read_rdb_value(table_id, record_id, day19_downpower_col, &day19_downpower);
	data_obj->read_rdb_value(table_id, record_id, day20_downpower_col, &day20_downpower);
	data_obj->read_rdb_value(table_id, record_id, day21_downpower_col, &day21_downpower);
	data_obj->read_rdb_value(table_id, record_id, day22_downpower_col, &day22_downpower);
	data_obj->read_rdb_value(table_id, record_id, day23_downpower_col, &day23_downpower);
	data_obj->read_rdb_value(table_id, record_id, day24_downpower_col, &day24_downpower);
	data_obj->read_rdb_value(table_id, record_id, day25_downpower_col, &day25_downpower);
	data_obj->read_rdb_value(table_id, record_id, day26_downpower_col, &day26_downpower);
	data_obj->read_rdb_value(table_id, record_id, day27_downpower_col, &day27_downpower);
	data_obj->read_rdb_value(table_id, record_id, day28_downpower_col, &day28_downpower);
	data_obj->read_rdb_value(table_id, record_id, day29_downpower_col, &day29_downpower);
	data_obj->read_rdb_value(table_id, record_id, day30_downpower_col, &day30_downpower);
	data_obj->read_rdb_value(table_id, record_id, day31_downpower_col, &day31_downpower);


	return 0;
}




/*函数名：read_unit_monthpower_rdb()
 *输入:
 *输出：
 *功能简介：储能单元月电量全部获取
 *时间：[8/2/2021 LJF]
 */
Cunit_monthpower_info::Cunit_monthpower_info(Cdata_access *_data_obj)
{
	this->data_obj = _data_obj;
	this->rdb_obj = data_obj->rdb_obj;
	this->dnet_obj = data_obj->dnet_obj;

}

Cunit_monthpower_info::~Cunit_monthpower_info()
{

}
int Cunit_monthpower_info::read_unit_monthpower_rdb()
{


	data_obj->read_rdb_value(table_id, record_id, display_id_col, &display_id);
	data_obj->read_rdb_value(table_id, record_id, name_col, name);
	data_obj->read_rdb_value(table_id, record_id, year_col, &year);

	data_obj->read_rdb_value(table_id, record_id, year_uppower_col, &year_uppower);
	data_obj->read_rdb_value(table_id, record_id, year_downpower_col, &year_downpower);

	data_obj->read_rdb_value(table_id, record_id, month1_uppower_col, &month1_uppower);
	data_obj->read_rdb_value(table_id, record_id, month2_uppower_col, &month2_uppower);
	data_obj->read_rdb_value(table_id, record_id, month3_uppower_col, &month3_uppower);
	data_obj->read_rdb_value(table_id, record_id, month4_uppower_col, &month4_uppower);
	data_obj->read_rdb_value(table_id, record_id, month5_uppower_col, &month5_uppower);
	data_obj->read_rdb_value(table_id, record_id, month6_uppower_col, &month6_uppower);
	data_obj->read_rdb_value(table_id, record_id, month7_uppower_col, &month7_uppower);
	data_obj->read_rdb_value(table_id, record_id, month8_uppower_col, &month8_uppower);
	data_obj->read_rdb_value(table_id, record_id, month9_uppower_col, &month9_uppower);
	data_obj->read_rdb_value(table_id, record_id, month10_uppower_col, &month10_uppower);
	data_obj->read_rdb_value(table_id, record_id, month11_uppower_col, &month11_uppower);
	data_obj->read_rdb_value(table_id, record_id, month12_uppower_col, &month12_uppower);

	data_obj->read_rdb_value(table_id, record_id, season1_uppower_col, &season1_uppower);
	data_obj->read_rdb_value(table_id, record_id, season2_uppower_col, &season2_uppower);
	data_obj->read_rdb_value(table_id, record_id, season3_uppower_col, &season3_uppower);
	data_obj->read_rdb_value(table_id, record_id, season4_uppower_col, &season4_uppower);

	data_obj->read_rdb_value(table_id, record_id, month1_downpower_col, &month1_downpower);
	data_obj->read_rdb_value(table_id, record_id, month2_downpower_col, &month2_downpower);
	data_obj->read_rdb_value(table_id, record_id, month3_downpower_col, &month3_downpower);
	data_obj->read_rdb_value(table_id, record_id, month4_downpower_col, &month4_downpower);
	data_obj->read_rdb_value(table_id, record_id, month5_downpower_col, &month5_downpower);
	data_obj->read_rdb_value(table_id, record_id, month6_downpower_col, &month6_downpower);
	data_obj->read_rdb_value(table_id, record_id, month7_downpower_col, &month7_downpower);
	data_obj->read_rdb_value(table_id, record_id, month8_downpower_col, &month8_downpower);
	data_obj->read_rdb_value(table_id, record_id, month9_downpower_col, &month9_downpower);
	data_obj->read_rdb_value(table_id, record_id, month10_downpower_col, &month10_downpower);
	data_obj->read_rdb_value(table_id, record_id, month11_downpower_col, &month11_downpower);
	data_obj->read_rdb_value(table_id, record_id, month12_downpower_col, &month12_downpower);

	data_obj->read_rdb_value(table_id, record_id, season1_downpower_col, &season1_downpower);
	data_obj->read_rdb_value(table_id, record_id, season2_downpower_col, &season2_downpower);
	data_obj->read_rdb_value(table_id, record_id, season3_downpower_col, &season3_downpower);
	data_obj->read_rdb_value(table_id, record_id, season4_downpower_col, &season4_downpower);

	return 0;




}



/*函数名：read_unit_daypower_rdb()
 *输入:
 *输出：
 *功能简介：储能单元日电量全部获取
 *时间：[8/2/2021 LJF]
 */
Cunit_daypower_info::Cunit_daypower_info(Cdata_access *_data_obj)
{
	this->data_obj = _data_obj;
	this->rdb_obj = data_obj->rdb_obj;
	this->dnet_obj = data_obj->dnet_obj;

}

Cunit_daypower_info::~Cunit_daypower_info()
{

}
int Cunit_daypower_info::read_unit_daypower_rdb()
{

	data_obj->read_rdb_value(table_id, record_id, display_id_col, &display_id);
	data_obj->read_rdb_value(table_id, record_id, name_col, name);
	data_obj->read_rdb_value(table_id, record_id, month_col, &month);
	data_obj->read_rdb_value(table_id, record_id, day1_uppower_col, &day1_uppower);
	data_obj->read_rdb_value(table_id, record_id, day2_uppower_col, &day2_uppower);
	data_obj->read_rdb_value(table_id, record_id, day3_uppower_col, &day3_uppower);
	data_obj->read_rdb_value(table_id, record_id, day4_uppower_col, &day4_uppower);
	data_obj->read_rdb_value(table_id, record_id, day5_uppower_col, &day5_uppower);
	data_obj->read_rdb_value(table_id, record_id, day6_uppower_col, &day6_uppower);
	data_obj->read_rdb_value(table_id, record_id, day7_uppower_col, &day7_uppower);
	data_obj->read_rdb_value(table_id, record_id, day8_uppower_col, &day8_uppower);
	data_obj->read_rdb_value(table_id, record_id, day9_uppower_col, &day9_uppower);
	data_obj->read_rdb_value(table_id, record_id, day10_uppower_col, &day10_uppower);
	data_obj->read_rdb_value(table_id, record_id, day11_uppower_col, &day11_uppower);
	data_obj->read_rdb_value(table_id, record_id, day12_uppower_col, &day12_uppower);
	data_obj->read_rdb_value(table_id, record_id, day13_uppower_col, &day13_uppower);
	data_obj->read_rdb_value(table_id, record_id, day14_uppower_col, &day14_uppower);
	data_obj->read_rdb_value(table_id, record_id, day15_uppower_col, &day15_uppower);
	data_obj->read_rdb_value(table_id, record_id, day16_uppower_col, &day16_uppower);
	data_obj->read_rdb_value(table_id, record_id, day17_uppower_col, &day17_uppower);
	data_obj->read_rdb_value(table_id, record_id, day18_uppower_col, &day18_uppower);
	data_obj->read_rdb_value(table_id, record_id, day19_uppower_col, &day19_uppower);
	data_obj->read_rdb_value(table_id, record_id, day20_uppower_col, &day20_uppower);
	data_obj->read_rdb_value(table_id, record_id, day21_uppower_col, &day21_uppower);
	data_obj->read_rdb_value(table_id, record_id, day22_uppower_col, &day22_uppower);
	data_obj->read_rdb_value(table_id, record_id, day23_uppower_col, &day23_uppower);
	data_obj->read_rdb_value(table_id, record_id, day24_uppower_col, &day24_uppower);
	data_obj->read_rdb_value(table_id, record_id, day25_uppower_col, &day25_uppower);
	data_obj->read_rdb_value(table_id, record_id, day26_uppower_col, &day26_uppower);
	data_obj->read_rdb_value(table_id, record_id, day27_uppower_col, &day27_uppower);
	data_obj->read_rdb_value(table_id, record_id, day28_uppower_col, &day28_uppower);
	data_obj->read_rdb_value(table_id, record_id, day29_uppower_col, &day29_uppower);
	data_obj->read_rdb_value(table_id, record_id, day30_uppower_col, &day30_uppower);
	data_obj->read_rdb_value(table_id, record_id, day31_uppower_col, &day31_uppower);
	data_obj->read_rdb_value(table_id, record_id, day1_downpower_col, &day1_downpower);
	data_obj->read_rdb_value(table_id, record_id, day2_downpower_col, &day2_downpower);
	data_obj->read_rdb_value(table_id, record_id, day3_downpower_col, &day3_downpower);
	data_obj->read_rdb_value(table_id, record_id, day4_downpower_col, &day4_downpower);
	data_obj->read_rdb_value(table_id, record_id, day5_downpower_col, &day5_downpower);
	data_obj->read_rdb_value(table_id, record_id, day6_downpower_col, &day6_downpower);
	data_obj->read_rdb_value(table_id, record_id, day7_downpower_col, &day7_downpower);
	data_obj->read_rdb_value(table_id, record_id, day8_downpower_col, &day8_downpower);
	data_obj->read_rdb_value(table_id, record_id, day9_downpower_col, &day9_downpower);
	data_obj->read_rdb_value(table_id, record_id, day10_downpower_col, &day10_downpower);
	data_obj->read_rdb_value(table_id, record_id, day11_downpower_col, &day11_downpower);
	data_obj->read_rdb_value(table_id, record_id, day12_downpower_col, &day12_downpower);
	data_obj->read_rdb_value(table_id, record_id, day13_downpower_col, &day13_downpower);
	data_obj->read_rdb_value(table_id, record_id, day14_downpower_col, &day14_downpower);
	data_obj->read_rdb_value(table_id, record_id, day15_downpower_col, &day15_downpower);
	data_obj->read_rdb_value(table_id, record_id, day16_downpower_col, &day16_downpower);
	data_obj->read_rdb_value(table_id, record_id, day17_downpower_col, &day17_downpower);
	data_obj->read_rdb_value(table_id, record_id, day18_downpower_col, &day18_downpower);
	data_obj->read_rdb_value(table_id, record_id, day19_downpower_col, &day19_downpower);
	data_obj->read_rdb_value(table_id, record_id, day20_downpower_col, &day20_downpower);
	data_obj->read_rdb_value(table_id, record_id, day21_downpower_col, &day21_downpower);
	data_obj->read_rdb_value(table_id, record_id, day22_downpower_col, &day22_downpower);
	data_obj->read_rdb_value(table_id, record_id, day23_downpower_col, &day23_downpower);
	data_obj->read_rdb_value(table_id, record_id, day24_downpower_col, &day24_downpower);
	data_obj->read_rdb_value(table_id, record_id, day25_downpower_col, &day25_downpower);
	data_obj->read_rdb_value(table_id, record_id, day26_downpower_col, &day26_downpower);
	data_obj->read_rdb_value(table_id, record_id, day27_downpower_col, &day27_downpower);
	data_obj->read_rdb_value(table_id, record_id, day28_downpower_col, &day28_downpower);
	data_obj->read_rdb_value(table_id, record_id, day29_downpower_col, &day29_downpower);
	data_obj->read_rdb_value(table_id, record_id, day30_downpower_col, &day30_downpower);
	data_obj->read_rdb_value(table_id, record_id, day31_downpower_col, &day31_downpower);
	return 0;
}
