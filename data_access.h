#ifndef DATA_ACCESS_H
#define DATA_ACCESS_H

#include "const_def.h"

class Cdata_access
{

//系统操作接口对象定义
public:
    RDB_NET		*rdb_obj;       //实时库对象
    system_net_info 	*dnet_obj;      //网络操作对象
    scada_report_manager *scada_report; //批量提交rdb更新对象 暂时不用
public:
    Cdata_access(RDB_NET *_rdb_obj, system_net_info *_dnet_obj, scada_report_manager *scada_report_in);
    ~Cdata_access();

    int read_rdb_value(int table_id, int record_id, short field_id, void *return_value);
    //int read_rdb_value(int table_id, int record_id, short field_id, string &return_value);
    int set_rdb_value(int table_id, int record_id, short field_id, int set_value);
    int set_rdb_value(int table_id, int record_id, short field_id, char set_value);
    int set_rdb_value(int table_id, int record_id, short field_id, float set_value);
    int set_rdb_value(int table_id, int record_id, short field_id, double set_value);
    //int set_rdb_value(int table_id, int record_id, short field_id, on_time_t set_value);
    int send_cmd_to_fore_manager(ctrl_cmd_struct cmd);
};
#endif // DATA_ACCESS_H
