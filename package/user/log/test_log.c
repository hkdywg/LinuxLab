#include "log.h"

int main(int argc, char *argv[])
{
	/* basic usage test */
	log_trace("get function address: %x", main);	
	log_debug("init log system");	
	log_info("loading user drm library: %s", "test_drm_system");	
	log_warn("current backlight level: %d", 12);	
	log_error("start system cali error");	
	log_fatal("system fatal");	

	/* set log system output level, message will not be output if < level  */
	log_set_level(LOG_INFO);

	log_trace("get function address: %x", main);	
	log_debug("init log system");	
	log_info("loading user drm library: %s", "test_drm_system");	
	log_warn("current backlight level: %d", 12);	
	log_error("start system cali error");	
	log_fatal("system fatal");	

	/* write log to file */
	FILE *logfile = fopen("./application.log", "w");
	log_add_fp(logfile, LOG_WARN);
	log_trace("get function address: %x", main);	
	log_debug("init log system");	
	log_info("loading user drm library: %s", "test_drm_system");	
	log_warn("current backlight level: %d", 12);	
	log_error("start system cali error");	
	log_fatal("system fatal");	

	log_set_quiet(true);
	log_trace("get function address: %x", main);	
	log_debug("init log system");	
	log_info("loading user drm library: %s", "test_drm_system");	
	log_warn("current backlight level: %d", 12);	
	log_error("start system cali error");	
	log_fatal("system fatal");	

	return 0;
}
