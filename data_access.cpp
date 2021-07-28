#include "data_access.h"



Cdata_access::Cdata_access(RDB_NET *_rdb_obj, system_net_info *_dnet_obj, scada_report_manager *scada_report_in)
{
    this->rdb_obj = _rdb_obj;
    this->dnet_obj = _dnet_obj;
    scada_report = scada_report_in;
}

Cdata_access::~Cdata_access()
{

}

int Cdata_access::read_rdb_value(int table_id, int record_id, short field_id, void *return_value)
{
    int	retcode;
    int record_num;
    int	result_len;
    char *buffer;
    buffer = (char *)MALLOC(256);

    char key_buf[500];
    int counter = 0;
    short tmp = 1;
    memcpy(key_buf+counter,(char *)&tmp,sizeof(short));			//序号
    counter+=sizeof(short);
    memcpy(key_buf+counter,(char *)&record_id,sizeof(int));		//关键字
    counter+=sizeof(int);
    memcpy(key_buf+counter,(char *)&field_id,sizeof(short));       //域号

    counter+=sizeof(short);

    retcode = this->rdb_obj->read_table_by_rdb_keys(
        table_id,
        DNET_APP_TYPE_SCADA,
        DNET_NO,
        key_buf,
        counter,
        buffer,
        record_num,
        result_len);
    if(retcode<=0)
    {
        dnet_obj->write_log(0,5199,"读取表号%d,记录号%d,域号%d 实时值失败", table_id, record_id, field_id);
        FREE((char *&)buffer);
        return -1;
    }
    //处理返回
    short rid;
    unsigned char rdata_type;
    short rdata_len;
    int offset = 0;

    memcpy(&rid, buffer+offset, sizeof(short));                 //序号
    offset += sizeof(short);
    memcpy(&rdata_type, buffer+offset, sizeof(unsigned char));  //数据类型
    offset += sizeof(unsigned char);
    memcpy(&rdata_len, buffer+offset, sizeof(short));           //域值长度FL
    offset += sizeof(short);
    memcpy(return_value, buffer+offset, rdata_len);             //域值

    FREE((char *&)buffer);
    return 1;
}
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, float set_value)
{
    char key_buff[500],*result;
    int counter = 0;
    int retcode,result_len;

//    exchange_short_order(field_id);
//    exchange_int_order(record_id);
    memcpy(key_buff+counter,(char *)&record_id,sizeof(int));	//关键字
    counter+=sizeof(int);
    memcpy(key_buff+counter,(char *)&field_id,sizeof(short));	//域号
    counter+=sizeof(short);

    result = (char *)MALLOC(sizeof(float));
    memcpy(result,&set_value,sizeof(float));
    result_len = 4;

    if (counter!=0)
    {
        //int ret = scada_report->set_scada_report_head_info();

        retcode = scada_report->add_one_table_modify_into_buf(
                    table_id,
                    DNET_APP_TYPE_SCADA,
                    0,
                    key_buff,
                    sizeof(int)+sizeof(short),
                    result,
                    result_len);
        //retcode = scada_report->send_all_modify_rdb();
        scada_report->send_all_modify_rdb();
    }
    else
    {
        FREE(result);
        return false;
    }
    if (retcode)
    {
        FREE(result);
        return true;
    }
    else
    {
        dnet_obj->write_log(
                    0,
                    1,
                    "组装报文出错");
        FREE(result);
        return false;
    }
}
#if 0
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, on_time_t set_value)
{
    char key_buff[500],*result;
    int counter = 0;
    int retcode,result_len;


    memcpy(key_buff+counter,(char *)&record_id,sizeof(int));	//关键字
    counter+=sizeof(int);
    memcpy(key_buff+counter,(char *)&field_id,sizeof(short));	//域号
    counter+=sizeof(short);

    result = (char *)MALLOC(sizeof(on_time_t));
    memcpy(result,&set_value,sizeof(on_time_t));
    result_len = 4;

    if (counter!=0)
    {

        retcode = scada_report->add_one_table_modify_into_buf(
                    table_id,
                    DNET_APP_TYPE_SCADA,
                    0,
                    key_buff,
                    sizeof(int)+sizeof(short),
                    result,
                    result_len);

    }
    else
    {
        FREE(result);
        return false;
    }
    if (retcode)
    {
        FREE(result);
        return true;
    }
    else
    {
        dnet_obj->write_log(
                    0,
                    1,
                    "组装报文出错");
        FREE(result);
        return false;
    }

}
#endif
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, int set_value)
{
    char key_buff[500],*result;
    int counter = 0;
    int retcode,result_len;


    memcpy(key_buff+counter,(char *)&record_id,sizeof(int));	//关键字
    counter+=sizeof(int);
    memcpy(key_buff+counter,(char *)&field_id,sizeof(short));	//域号
    counter+=sizeof(short);

    result = (char *)MALLOC(sizeof(int));
    memcpy(result,&set_value,sizeof(int));
    result_len = 4;

    if (counter!=0)
    {

        retcode = scada_report->add_one_table_modify_into_buf(
                    table_id,
                    DNET_APP_TYPE_SCADA,
                    0,
                    key_buff,
                    sizeof(int)+sizeof(short),
                    result,
                    result_len);

    }
    else
    {
        FREE(result);
        return false;
    }
    if (retcode)
    {
        FREE(result);
        return true;
    }
    else
    {
        dnet_obj->write_log(
                    0,
                    1,
                    "组装报文出错");
        FREE(result);
        return false;
    }
}
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, char set_value)
{
    char key_buff[500],*result;
    int counter = 0;
    int retcode,result_len;

    memcpy(key_buff+counter,(char *)&record_id,sizeof(int));	//关键字
    counter+=sizeof(int);
    memcpy(key_buff+counter,(char *)&field_id,sizeof(short));	//域号
    counter+=sizeof(short);

    result = (char *)MALLOC(sizeof(char));
    memcpy(result,&set_value,sizeof(char));
    result_len = 1;

    if (counter!=0)
    {

        retcode = scada_report->add_one_table_modify_into_buf(
                    table_id,
                    DNET_APP_TYPE_SCADA,
                    0,
                    key_buff,
                    sizeof(int)+sizeof(short),
                    result,
                    result_len);


    }
    else
    {
        FREE(result);
        return false;
    }
    if (retcode)
    {
        FREE(result);
        return true;
    }
    else
    {
        dnet_obj->write_log(
                    0,
                    1,
                    "组装报文出错");
        FREE(result);
        return false;
    }
}

#if 0
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, int set_value)
{
//    dnet_obj->write_log(0,5199,"set_rdb_value:表号%d,记录号%d,域号%d,设置值%d",
//                        table_id, record_id, field_id, set_value);
    int	retcode;
    int	result_len = 1;
    char *buffer;
    buffer = (char *)MALLOC(500);
    char *result = (char *)MALLOC(sizeof(char));
    char key_buf[500];
    int counter = 0;

    memcpy(key_buf+counter,(char *)&record_id,sizeof(int));
    counter += sizeof(int);
    memcpy(key_buf+counter,(char *)&field_id,sizeof(short));
    counter += sizeof(short);
    memcpy(key_buf+counter,(char *)&set_value,sizeof(int));
    counter += sizeof(int);

    retcode = this->rdb_obj->modify_table_by_rdb_keys(
        table_id,
        DNET_APP_TYPE_SCADA,
        DNET_NO,
        key_buf,
        counter,
        result,
        result_len);
    if(retcode<=0)
    {
        FREE((char *&)buffer);
        return -1;
    }

    FREE((char *&)buffer);
    return 0;
}
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, double set_value)
{

    int	retcode;
    int record_num;
    int	result_len = 1;
    char *buffer;
    buffer = (char *)MALLOC(256);

    char key_buf[500];
    char *result = (char *)MALLOC(sizeof(char));

    int counter = 0;

    memcpy(key_buf+counter,(char *)&record_id,sizeof(int));
    counter += sizeof(int);
    memcpy(key_buf+counter,(char *)&field_id,sizeof(short));
    counter += sizeof(short);
    memcpy(key_buf+counter,(char *)&set_value,sizeof(double));
    counter += sizeof(double);

    retcode = this->rdb_obj->modify_table_by_rdb_keys(
        table_id,
        DNET_APP_TYPE_SCADA,
        DNET_NO,
        key_buf,
        counter,
        result,
        result_len);
    if(retcode<=0)
    {
        FREE((char *&)buffer);
        return -1;
    }

    FREE((char *&)buffer);
    return 0;
}
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, char set_value)
{

    int	retcode;
    int record_num;
    int	result_len = 1;
    char *buffer;
    buffer = (char *)MALLOC(256);

    char key_buf[500];
    char *result = (char *)MALLOC(sizeof(char));

    int counter = 0;

    memcpy(key_buf+counter,(char *)&record_id,sizeof(int));
    counter += sizeof(int);
    memcpy(key_buf+counter,(char *)&field_id,sizeof(short));
    counter += sizeof(short);
    memcpy(key_buf+counter,(char *)&set_value,sizeof(char));
    counter += sizeof(char);

    retcode = this->rdb_obj->modify_table_by_rdb_keys(
        table_id,
        DNET_APP_TYPE_SCADA,
        DNET_NO,
        key_buf,
        counter,
        result,
        result_len);
    if(retcode<=0)
    {
        FREE((char *&)buffer);
        return -1;
    }

    FREE((char *&)buffer);
    return 0;
}
int Cdata_access::set_rdb_value(int table_id, int record_id, short field_id, float set_value)
{
    //dnet_obj->write_log(0,5199,"set_rdb_value:表号%d,记录号%d,域号%d,设置值%f",
      //                  table_id, record_id, field_id, set_value);
    int	retcode;
    int	result_len = 1;
    char *buffer = (char *)MALLOC(256);

    char key_buf[500];
    char *result = (char *)MALLOC(sizeof(char));
    int counter = 0;

    memcpy(key_buf+counter,(char *)&record_id,sizeof(int));
    counter += sizeof(int);
    memcpy(key_buf+counter,(char *)&field_id,sizeof(short));
    counter += sizeof(short);
    memcpy(key_buf+counter,(char *)&set_value,sizeof(float));
    counter += sizeof(float);

    retcode = rdb_obj->modify_table_by_rdb_keys(
        table_id,
        DNET_APP_TYPE_SCADA,
        DNET_NO,
        key_buf,
        counter,
        result,
        result_len);
    if(retcode<=0)
    {
        dnet_obj->write_log(0,5199,"set_rdb_value:返回失败");
        FREE((char *&)buffer);
        return -1;
    }

    FREE((char *&)buffer);
    return 0;
}
#endif
int Cdata_access::send_cmd_to_fore_manager(ctrl_cmd_struct cmd)
{

    int app_type =  (0x00000001) << (17-1);

    char buffer[256];
    int counter = 0;
    int cmd_info_len;
    //报文头
    put_one_int_to_buffer(buffer, app_type, counter);	//前置应用类型
    put_one_short_to_buffer(buffer, 1, counter);	//结点类型 1(操作界面到前置) 2(前置到操作界面)
    put_one_short_to_buffer(buffer, -1, counter);	//值班状态
    put_one_short_to_buffer(buffer, 1, counter);	//命令个数
    if(cmd.cmd_type == FORE_CMD_TYPE_YK)
    {
        cmd_info_len = sizeof(int)*5 + sizeof(short)*2 + sizeof(char)*2;

    }
    else if(cmd.cmd_type == FORE_CMD_TYPE_FLOAT)
    {
        cmd_info_len = sizeof(int)*5 + sizeof(short)*2 + sizeof(char)*2 + sizeof(float)*2; //
    }

    put_one_int_to_buffer(buffer, cmd_info_len, counter);		//命令长度


    //控制命令报文体
    put_one_int_to_buffer(buffer, cmd.table_id, counter);		//设备表号
    put_one_int_to_buffer(buffer, cmd.record_id, counter);	//记录号

    put_one_int_to_buffer(buffer, cmd.fac_id, counter);	//厂站ID

    put_one_short_to_buffer(buffer, cmd.addr_no, counter);	//公共地址
    put_one_int_to_buffer(buffer, cmd.send_no, counter);	//信息体地址（点号）

    put_one_char_to_buffer(buffer, cmd.cmd_type, counter);	//命令类型-- OPERATE_CMD_TYPE_INT 3 整形设定
    put_one_char_to_buffer(buffer, cmd.cmd_step, counter);	//命令阶段

    if(cmd.cmd_type == FORE_CMD_TYPE_YK)
    {
        //按照遥控执行方式组装
        put_one_char_to_buffer(buffer, cmd.dest_val_char, counter);
        put_one_short_to_buffer(buffer, 10, counter);	//超时时间
        put_one_int_to_buffer(buffer, -1, counter);	//exect_chan_id
    }
    else if(cmd.cmd_type == FORE_CMD_TYPE_FLOAT)
    {
        //按照浮点设定方式组装
        put_one_float_to_buffer(buffer, 0, counter);	//param_sour_float
        put_one_float_to_buffer(buffer, cmd.dest_val_float, counter);	//param_dest_float
        put_one_short_to_buffer(buffer, 5, counter);	//超时时间
        put_one_int_to_buffer(buffer, -1, counter);	//所属通道ID,-1表示默认通道发送
    }

    int ret_code = dnet_obj->dnet_send_message(DNET_REPORT_POINT_TO_POINT,
                                                DNET_SCADA_OPERATION_YK,
                                            app_type,
                                            "fore_cmd_manager",
                                            buffer,
                                            counter);
    if(ret_code < 0)
    {
        dnet_obj->write_log(0, 1000,"发送遥控报文到前置机fore_cmd_manager失败,rec_code=%d" , ret_code);
        return -1;
    }
    return 1;
}
