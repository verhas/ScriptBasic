'
' FILE: mysql.bas
'
' This is the module declaration of the ScriptBasic external module mysql
'
' To use this module you have to have mysql.dll or mysql.so installed in the
' modules directory.
'
' These implement the interface to the Berkeley Data Base library

module mysql

declare sub ::RealConnect alias "mys_real_connect" lib "mysql"
declare sub ::Connect alias "mys_config_connect" lib "mysql"
declare sub ::Close alias "mys_close" lib "mysql"
declare sub ::Query alias "mys_query" lib "mysql"
declare sub ::FetchArray alias "mys_fetcharray" lib "mysql"
declare sub ::FetchHash alias "mys_fetchhash" lib "mysql"
declare sub ::AffectedRows alias "mys_affected_rows" lib "mysql"
declare sub ::ChangeUser alias "mys_change_user" lib "mysql"
declare sub ::CharacterSetName alias "mys_character_set_name" lib "mysql"
declare sub ::DataSeek alias "mys_data_seek" lib "mysql"
declare sub ::ErrorMessage alias "mys_error" lib "mysql"
declare sub ::GetClientInfo alias "mys_get_client_info" lib "mysql"
declare sub ::GetHostInfo alias "mys_get_host_info" lib "mysql"
declare sub ::GetProtoInfo alias "mys_get_proto_info" lib "mysql"
declare sub ::GetServerInfo alias "mys_get_server_info" lib "mysql"
declare sub ::Info alias "mys_info" lib "mysql"
declare sub ::InsertId alias "mys_insert_id" lib "mysql"
declare sub ::Kill alias "mys_kill" lib "mysql"
declare sub ::Ping alias "mys_ping" lib "mysql"
declare sub ::EscapeString alias "mys_real_escape_string" lib "mysql"
declare sub ::SelectDatabase alias "mys_select_db" lib "mysql"
declare sub ::Shutdown alias "mys_shutdown" lib "mysql"
declare sub ::Stat alias "mys_stat" lib "mysql"
declare sub ::ThreadId alias "mys_thread_id" lib "mysql"

end module

