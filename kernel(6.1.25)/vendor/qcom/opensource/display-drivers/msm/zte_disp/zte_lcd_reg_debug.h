
#define ZTE_REG_LEN 64
#define REG_MAX_LEN 16 /*one lcd reg display info max length*/
enum {	/* read or write mode */
	REG_WRITE_MODE = 0,
	REG_READ_MODE,
	REG_WRITE_MODE_LP,
	REG_READ_MODE_LP
};

struct zte_lcd_reg_debug {
	int is_read_mode;  /*if 1 read ,0 write*/
	unsigned char length;
	char rbuf[ZTE_REG_LEN];
	char wbuf[ZTE_REG_LEN];
};

/*WARNING: Single statement macros should not use a do {} while (0) loop*/
//#define ZTE_LCD_INFO(fmt, args...) {pr_info("[MSM_LCD][Info]"fmt, ##args); }
#define PRI_REG "reg_debug"
#define SEC_REG "secreg_debug"
#define PRI_FILE 0
#define SEC_FILE 1