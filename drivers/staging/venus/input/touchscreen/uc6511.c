#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
//CHERRY ADD
#include <linux/venus/uc6511.h>
//CHERRY ADD
#include <linux/venus/power_control.h>
#include <linux/earlysuspend.h>

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#define PRINT_XY		0
#define MAX_TRACK_POINT		5

#define	MAX_12BIT			((1<<12)-1)

#define UC6511_DEVID		0x0A
#define COMMON_FUNCTION   1
typedef struct i2c_client	bus_device;
#define PROC_CMD_LEN 20

static u16 fingers_x[MAX_TRACK_POINT];
static u16 fingers_y[MAX_TRACK_POINT];
static char fwversion[5];

//################################################################################################
// read and check parameter file, begin
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/uaccess.h>

#define Calibrate_Name 	 	"/data/data/com.TPCalibration/CalibrateData.txt"
#define Default_Name 	 	"/data/data/com.TPCalibration/DefaultData.txt"

static u16 Calibrate_data[10];		//x0,y0,x1,y1,x2,y2,x3,y4,host_width,host_height
static u16 Default_data[10];		//x0,y0,x1,y1,x2,y2,x3,y4,host_width,host_height
//static bool Calibrate_run = false;
//static bool init_read= false;
static bool Default_file = false;
static bool Calibrate_file = false;
static int Calibrateing = false;
static int Irq_enable = false;
static u16 TOUCH_MAX_X_D = 0;
static u16 TOUCH_MAX_Y_D = 0;
static u16 TOUCH_MIN_X_D = 0;
static u16 TOUCH_MIN_Y_D = 0;
static u16 MUL_X_PARAMETER_D = 0;
static u16 DIV_X_PARAMETER_D = 0;            
static u16 MUL_Y_PARAMETER_D = 0;
static u16 DIV_Y_PARAMETER_D = 0;
static u16 TOUCH_MAX_X_C = 0;
static u16 TOUCH_MAX_Y_C = 0;
static u16 TOUCH_MIN_X_C = 0;
static u16 TOUCH_MIN_Y_C = 0;
static u16 MUL_X_PARAMETER_C = 0;
static u16 DIV_X_PARAMETER_C = 0;            
static u16 MUL_Y_PARAMETER_C = 0;
static u16 DIV_Y_PARAMETER_C = 0;      
static u16 report_count = 0;

mm_segment_t oldfs;
static struct file *openFile(char *path,int flag,int mode)
{
        struct file *fp = NULL;
        fp=filp_open(path, flag, 0);

//	printk(KERN_ALERT "fp===== %x",fp);
        if(IS_ERR(fp))
        {
                printk(KERN_ALERT "NO FILE");
                return NULL;
        }
        printk(KERN_ALERT " HAVE FILE");
        return fp;
}
/*
static struct file *openFile(char *path,int flag,int mode)
{
        struct file *fp;

        fp=filp_open(path, flag, 0);
        if (fp)
                return fp;
        else
                return NULL;
}
*/
static int readFile(struct file *fp,char *buf,int readlen)
{
        if (fp->f_op && fp->f_op->read)
                return fp->f_op->read(fp,buf,readlen, &fp->f_pos);
        else
                return -1;
}

static int closeFile(struct file *fp)
{
        filp_close(fp,NULL);
        return 0;
}
static void initKernelEnv(void)
{
        oldfs = get_fs();
        set_fs(KERNEL_DS);
}

/*
static void readDefaultValue()
{
        char buf[1024]={0};
        struct file *fp;
	int ret = 0;
	int i = 0;
	int k = 0;
	int last_number = 0;
	int file_data_number = 0;
	initKernelEnv();
       
//	fp=openFile(Default_Name,O_RDONLY,0);
//
//	printk(KERN_ALERT"fp=  %x ----",fp);
//       	if (fp!= NULL)
//       {               
//               if ((ret=readFile(fp,buf,1024))>0)//get file data
//                                    ;			
//                       
//                else
//                        printk(KERN_ALERT"read file error %d\n",ret);
//                closeFile(fp);
		strcpy(buf,"400,81,92,300,709,300,400,517,800,600,");
		set_fs(oldfs);
        
		for( i = 0 ; i < 1024 ; i++ )				
		{
			if( buf[i]==',' )				//Calibrate_data[10]
			{
                        	for( k = last_number ; k < i ; k++)
                        	{
                                	if( k == last_number )
                                        	Calibrate_data[file_data_number] = buf[k] -48 ;
                                	else
                                        	Calibrate_data[file_data_number] = Calibrate_data[file_data_number]*10 + buf[k] - 48;
                        	}
                        	last_number=i+1;
				file_data_number++;			
			}
		}
		bool data_error = false;

		for( i = 0 ; i < 10 ; i++)			
		{
			if ( (i % 2) == 0 )
			{
				if( Calibrate_data[i] > Calibrate_data[8])
                               	Calibrate_data[i]=Calibrate_data[8];
					//data_error = true;	
			}
			else if( (i % 2) == 1 )
			{
		        	if( Calibrate_data[i] > Calibrate_data[9]) 
				Calibrate_data[i]=Calibrate_data[9];     
                                      	//data_error = true;
			}
			//printk(KERN_INFO "%d\n",Calibrate_data[i] );
		} 
		if( data_error == false )				
		{
        		TOUCH_MAX_X_D= Calibrate_data[4] * (MAX_12BIT + 1) /Calibrate_data[8];
        		TOUCH_MAX_Y_D = Calibrate_data[7] * (MAX_12BIT + 1) /Calibrate_data[9];
        		TOUCH_MIN_X_D = Calibrate_data[2] * (MAX_12BIT + 1) /Calibrate_data[8];
        		TOUCH_MIN_Y_D = Calibrate_data[1] * (MAX_12BIT + 1) /Calibrate_data[9];
        		MUL_X_PARAMETER_D = MAX_12BIT * 1000 / (TOUCH_MAX_X_D - TOUCH_MIN_X_D);
        		DIV_X_PARAMETER_D = 1000;             
        		MUL_Y_PARAMETER_D = MAX_12BIT * 1000 / (TOUCH_MAX_Y_D - TOUCH_MIN_Y_D); 
        		DIV_Y_PARAMETER_D = 1000;
        		Default_file = true;            
		}
		else
			Default_file = false;
//	}
//	else
//	{
//		printk(KERN_INFO "=no default file=");
//		Default_file = false;
//	}
}
*/


static void readDefaultFile()
{
        char buf[1024]={0};
        struct file *fp;
	int ret = 0;
	int i = 0;
	int k = 0;
	int last_number = 0;
	int file_data_number = 0;
	initKernelEnv();
	bool data_error = false;
       

	fp=openFile(Default_Name,O_RDONLY,0);

	//printk(KERN_ALERT"fp=  %x ----",fp);
       	if (fp!= NULL)
        {
					memset(buf,0,1024);
					if ((ret=readFile(fp,buf,1024))>0)//get file data
					{    
					//	printk(KERN_ALERT"DefaultFile buf:%s\n",buf);
						file_data_number=0;
						for( i = 0 ; i < 1024 ; i++ )				//check default_file contents
						{
							if(buf[i]==',' ) file_data_number++;
						}		
						if(file_data_number<10)	//Avoid the APK writing default_file error
						{
								data_error=true;
								printk(KERN_ALERT"default file contents ERROR: buf:%s\n",buf);
						}
					}    
					else
					{
						data_error=true;
						printk(KERN_ALERT"read default file error %d\n",ret);
					}
					closeFile(fp);
				}
				else
				{
					data_error=true;
					printk(KERN_INFO "=no default file or open defaule file error=");
				}
				
				if(data_error==true)
				{
						strcpy(buf,"400,81,92,300,709,300,400,517,800,600,");
						printk(KERN_INFO "Driver using the default value : %s\n",buf);
						data_error=false;
				}
				
		set_fs(oldfs);
		
    file_data_number=0;
		for( i = 0 ; i < 1024 ; i++ )				
		{
			if( buf[i]==',' )				//Calibrate_data[10]
			{
                        	for( k = last_number ; k < i ; k++)
                        	{
                                	if( k == last_number )
                                        	Calibrate_data[file_data_number] = buf[k] -48 ;
                                	else
                                        	Calibrate_data[file_data_number] = Calibrate_data[file_data_number]*10 + buf[k] - 48;
                        	}
                        	last_number=i+1;
				file_data_number++;			
			}
		}

		for( i = 0 ; i < 10 ; i++)			
		{
			if ( (i % 2) == 0 )
			{
				if( Calibrate_data[i] > Calibrate_data[8])
                               	Calibrate_data[i]=Calibrate_data[8];
					//data_error = true;	
			}
			else if( (i % 2) == 1 )
			{
		        	if( Calibrate_data[i] > Calibrate_data[9]) 
				Calibrate_data[i]=Calibrate_data[9];     
                                      	//data_error = true;
			}
			//printk(KERN_INFO "%d\n",Calibrate_data[i] );
		} 
//		if( data_error == false )				
//		{
        		TOUCH_MAX_X_D= Calibrate_data[4] * (MAX_12BIT + 1) /Calibrate_data[8];
        		TOUCH_MAX_Y_D = Calibrate_data[7] * (MAX_12BIT + 1) /Calibrate_data[9];
        		TOUCH_MIN_X_D = Calibrate_data[2] * (MAX_12BIT + 1) /Calibrate_data[8];
        		TOUCH_MIN_Y_D = Calibrate_data[1] * (MAX_12BIT + 1) /Calibrate_data[9];
        		MUL_X_PARAMETER_D = MAX_12BIT * 1000 / (TOUCH_MAX_X_D - TOUCH_MIN_X_D);
        		DIV_X_PARAMETER_D = 1000;             
        		MUL_Y_PARAMETER_D = MAX_12BIT * 1000 / (TOUCH_MAX_Y_D - TOUCH_MIN_Y_D); 
        		DIV_Y_PARAMETER_D = 1000;
        		Default_file = true;            
//		}
//		else
//			Default_file = false;
//	}
//	else
//	{
//		printk(KERN_INFO "=no default file=");
//		Default_file = false;
//	}
}


static void readCalibrateFile()
{
	char buf[1024];
	struct file *fp;
	int ret = 0;
	int i = 0;
	int k = 0;
	int last_number = 0;
	int file_data_number = 0;
	initKernelEnv();
	bool data_error = false;

	fp=openFile(Calibrate_Name,O_RDONLY,0);

	//printk(KERN_ALERT"fp=  %x ----",fp);
		if (fp!= NULL)
		{
      memset(buf,0,1024);
      if ((ret=readFile(fp,buf,1024))>0) //get file data
      {    
		//		printk(KERN_ALERT"CalibrateFile buf:%s\n",buf);
				file_data_number=0;
				for( i = 0 ; i < 1024 ; i++ )				//check calibration_file contents
				{
					if(buf[i]==',' ) file_data_number++;
				}		
				if(file_data_number<10)	//Avoid the APK writing calibration_file error
				{
						data_error=true;
						printk(KERN_ALERT"calibration file contents ERROR: buf:%s\n",buf);
				}
			}     
      else
			{
				data_error=true;
				printk(KERN_ALERT"read Calibration file error %d\n",ret);
			}
			closeFile(fp);
		}
		else
		{
			data_error=true;
		//	printk(KERN_INFO "=no Calibration file or open Calibration file error=");
		}

		set_fs(oldfs);

		if( data_error == false )				
		{
			file_data_number=0;    
			for( i = 0 ; i < 1024 ; i++ )				
			{
				if( buf[i]==',' )				//Calibrate_data[10]
				{
					for( k = last_number ; k < i ; k++)
					{
						if( k == last_number )
							Calibrate_data[file_data_number] = buf[k] -48 ;
						else
							Calibrate_data[file_data_number] = Calibrate_data[file_data_number]*10 + buf[k] - 48;
					}
					last_number=i+1;
					file_data_number++;			
				}
			}

		for( i = 0 ; i < 10 ; i++)			
		{
			if ( (i % 2) == 0 )
			{
				if( Calibrate_data[i] > Calibrate_data[8])
					Calibrate_data[i]=Calibrate_data[8];
					//data_error = true;	
			}
			else if( (i % 2) == 1 )
			{
      	if( Calibrate_data[i] > Calibrate_data[9])   
      		Calibrate_data[i]=Calibrate_data[9];   
                                      	//data_error = true;
			}
			//printk(KERN_INFO "%d\n",Calibrate_data[i] );
		} 
		
		TOUCH_MAX_X_C = Calibrate_data[4] * (MAX_12BIT + 1) /Calibrate_data[8];
		TOUCH_MAX_Y_C = Calibrate_data[7] * (MAX_12BIT + 1) /Calibrate_data[9];
		TOUCH_MIN_X_C = Calibrate_data[2] * (MAX_12BIT + 1) /Calibrate_data[8];
		TOUCH_MIN_Y_C = Calibrate_data[1] * (MAX_12BIT + 1) /Calibrate_data[9];
		MUL_X_PARAMETER_C = MAX_12BIT * 1000 / (TOUCH_MAX_X_C - TOUCH_MIN_X_C);
		DIV_X_PARAMETER_C = 1000;             
		MUL_Y_PARAMETER_C = MAX_12BIT * 1000 / (TOUCH_MAX_Y_C - TOUCH_MIN_Y_C); 
		DIV_Y_PARAMETER_C = 1000;
		Calibrate_file = true;            
//		}
//		else
//			Calibrate_file = false;
	}
	else
	{
//		printk(KERN_INFO "=no calibrate file=");
		Calibrate_file = false;
	}
}
// read and check parameter file, end
//#################################################################################################

struct uc6511_point {
	char			count;
	char			number;
	u16			x;
	u16			y;
};

struct uc6511 {
	bus_device		*bus;
	struct input_dev	*input;
	struct work_struct	work;
	struct timer_list	timer;

	struct mutex		mutex;
	unsigned		disabled:1;	/* P: mutex */

	char			phys[32];
};
static struct uc6511 *ts;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend es;
static void uc6511_early_suspend(struct early_suspend *h);
static void uc6511_late_resume(struct early_suspend *h);
#endif

static void uc6511_work(struct work_struct *work)
{	struct uc6511_point pt;
	char point_count;
	char point_no;
	int x, y;

	struct uc6511 *ts = container_of(work, struct uc6511, work);
	struct input_dev *input_dev = ts->input;

	Irq_enable = true;
	i2c_master_recv(ts->bus, &pt, 6);
	point_count = pt.count;
	point_no    = pt.number;
	// calculate x/y sampling values
	x = (MAX_12BIT - swab16(pt.y));
	y = swab16(pt.x);

//#######################################################
//calibration of x,y coordinates, begin
	if (Default_file == false)
	{
//		readDefaultValue();
		readDefaultFile();	//JamesLin, 20110905
		readCalibrateFile();
	}
        else if ((Calibrate_file == false)&&(Calibrateing == false))
        {
          readCalibrateFile();
        }
      

	if( point_count == 0x51)
	{
		Calibrateing = true;
		Calibrate_file = false; 
                readDefaultFile();
	//	printk(KERN_INFO " Run Calibration!!\n");
	}
	else if( point_count == 0x52)
	{	
		Calibrateing = false;	//JamesLin, 20110905
	//	Calibrate_file = true;
	//	printk(KERN_INFO " Calibration OK!!\n");
    readCalibrateFile();
	}

	if ( Calibrate_file == true )
	{
			//printk(KERN_INFO " Calibration with Calibrate_File\n");
			int TmpX = 0, TmpY = 0, TmpX2 = 0, TmpY2 = 0; 
      int px = 0, py =0;
 
    	px=x;
     	py=y; 
     	TmpX2 = (px >= TOUCH_MAX_X_C) ? (TOUCH_MAX_X_C) : px; 
     	TmpY2 = (py >= TOUCH_MAX_Y_C) ? (TOUCH_MAX_Y_C) : py; 

     	TmpX = (TmpX2 < TOUCH_MIN_X_C) ? (TOUCH_MIN_X_C) : TmpX2;
     	TmpY = (TmpY2 < TOUCH_MIN_Y_C) ? (TOUCH_MIN_Y_C) : TmpY2;
 
     	TmpX -= TOUCH_MIN_X_C; 
     	TmpY -= TOUCH_MIN_Y_C; 
 
     	TmpX = (TmpX) ? TmpX : 0; 
     	TmpY = (TmpY) ? TmpY : 0; 
     	px = TmpX * MUL_X_PARAMETER_C / DIV_X_PARAMETER_C; 
     	py = TmpY * MUL_Y_PARAMETER_C / DIV_Y_PARAMETER_C; 
 
     	if(px>4095) px=4095;   
     	if(py>4095) py=4095;
     
     	x = px;  //For TopTouch PDC2 TP
     	y = py;  //For TopTouch PDC2 TP 
	}
	else if (Default_file == true)
	{
			//printk(KERN_INFO " Calibration with Default_File\n");
			int TmpX = 0, TmpY = 0, TmpX2 = 0, TmpY2 = 0; 
      int px = 0, py =0;
 
    	px=x;
     	py=y; 
     	TmpX2 = (px >= TOUCH_MAX_X_D) ? (TOUCH_MAX_X_D) : px; 
     	TmpY2 = (py >= TOUCH_MAX_Y_D) ? (TOUCH_MAX_Y_D) : py; 

     	TmpX = (TmpX2 < TOUCH_MIN_X_D) ? (TOUCH_MIN_X_D) : TmpX2;
     	TmpY = (TmpY2 < TOUCH_MIN_Y_D) ? (TOUCH_MIN_Y_D) : TmpY2;
 
     	TmpX -= TOUCH_MIN_X_D; 
     	TmpY -= TOUCH_MIN_Y_D; 
 
     	TmpX = (TmpX) ? TmpX : 0; 
     	TmpY = (TmpY) ? TmpY : 0; 
     	px = TmpX * MUL_X_PARAMETER_D / DIV_X_PARAMETER_D; 
     	py = TmpY * MUL_Y_PARAMETER_D / DIV_Y_PARAMETER_D; 
 
     	if(px>4095) px=4095;   
     	if(py>4095) py=4095;
     
     	x = px;  //For TopTouch PDC2 TP
     	y = py;  //For TopTouch PDC2 TP 
	}	
//calibration of x,y coordinates, end
//#######################################################

       fingers_x[point_no - 1] = x;
       fingers_y[point_no - 1] = y;

        
#if PRINT_XY
	//printk(KERN_INFO "uc6511_work%d / %d +\n", point_count, point_no);
#endif

	if (point_count == point_no) {
#if PRINT_XY
	//	printk(KERN_INFO "touch_uc6511 %d / %d", point_count, point_no);
#endif
		for (point_no = 0; point_no < point_count; point_no ++) {
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 128);
			input_report_abs(input_dev, ABS_MT_POSITION_X, fingers_x[point_no]);
			input_report_abs(input_dev, ABS_MT_POSITION_Y, fingers_y[point_no]);
			input_mt_sync(input_dev);
#if PRINT_XY
	printk(KERN_INFO "touch_uc6511 (%d, %d)", fingers_x[point_no],fingers_y[point_no]);
#endif
		}
		input_sync(input_dev);
	}
	else if (point_count == 0x80) {
		input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(input_dev);
		input_sync(input_dev);
	}
 
	enable_irq(ts->bus->irq);
}

static irqreturn_t uc6511_irq(int irq, void *handle)
{
	struct uc6511 *ts = handle;

	/* The repeated conversion sequencer controlled by TMR kicked off too fast.
	 * We ignore the last and process the sample sequence currently in the queue.
	 * It can't be older than 9.4ms
	 */
	disable_irq_nosync(irq);

	if (!work_pending(&ts->work))
		schedule_work(&ts->work);

	return IRQ_HANDLED;

}

//static void uc6511_disable(struct uc6511 *ts)
static void uc6511_disable(void)
{
 //printk(KERN_INFO"touch_uc6511 %s,%d\n",__func__,__LINE__);
	mutex_lock(&ts->mutex);

	if (!ts->disabled) {
		ts->disabled = 1;
		disable_irq(ts->bus->irq);
		cancel_work_sync(&ts->work);
	}
	Irq_enable = false;
	mutex_unlock(&ts->mutex);
}

//static void uc6511_enable(struct uc6511 *ts)
static void uc6511_enable(void)
{
	int err;
	mutex_lock(&ts->mutex);

	if (ts->disabled) {
		ts->disabled = 0;
		enable_irq(ts->bus->irq);
	}
	if (Irq_enable == false){
                printk("##################Irq_enable == false################\n");
       //       	uc6511_disable();
		ts->disabled = 1;
		disable_irq(ts->bus->irq);
		 cancel_work_sync(&ts->work);

		mdelay(50);
		free_irq(ts->bus->irq, ts);
		mdelay(50);
	err = request_irq(ts->bus->irq, uc6511_irq,
			  IRQF_TRIGGER_FALLING, ts->bus->dev.driver->name, ts);
	if (err) {
		printk("err requeset_irq");		     
	}
	ts->disabled = 0;
	}
	mutex_unlock(&ts->mutex);
}

static ssize_t uc6511_pen_down_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return 0;

}

static DEVICE_ATTR(pen_down, 0644, uc6511_pen_down_show, NULL);

static ssize_t uc6511_disable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct uc6511 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ts->disabled);
}

static ssize_t uc6511_disable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct uc6511 *ts = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;

	if (val)
		uc6511_disable();
	else
		uc6511_enable();

	return count;
}

/*********************************Proc-filesystem-TOP**************************************/
static int uc6511_fwinfo(void)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	uint8_t Rdbuf[6]={0};
	uint8_t Wrbuf[2]={0};
	Wrbuf[0] = 0x07;
	Wrbuf[1] = 0x50;
	int ret;
	
	ret = i2c_master_send(ts->bus,Wrbuf,sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		printk("%s:Unable to write touch ic buffer!\n",__FUNCTION__);
		goto out;
	}
	ret = i2c_master_recv(ts->bus,Rdbuf,sizeof(Rdbuf));
	if (ret != sizeof(Rdbuf))
	{
		printk("%s:Unable to read touch ic buffer!\n",__FUNCTION__);
		goto out;
	}
	fwversion[0] = 10;
	if (Rdbuf[0] > fwversion[0]){       
	      fwversion[0] = Rdbuf[0];
	      fwversion[1] = Rdbuf[1];
	      fwversion[2] = Rdbuf[3];
	      fwversion[3] = Rdbuf[4];
	      fwversion[4] = Rdbuf[5];
	}else {
	      printk("read default uc6511 fwversion\n");
	      fwversion[0] = 19;
	      fwversion[1] = 1;
	      fwversion[2] = 11;
	      fwversion[3] = 12;
	      fwversion[4] = 8; 
	}
	printk("UC6511 touch firmware version: 20%02d%02d%02d-%02x%02x",fwversion[2],fwversion[3],fwversion[4],fwversion[0],fwversion[1]);
	return 0;
out:
	return -1;
}

unsigned int uc6511_get_version(char *buf)
{
	int ret;	
	ret = sprintf(buf, "20%02d%02d%02d-%02x%02x",fwversion[2],fwversion[3],fwversion[4],fwversion[0],fwversion[1]);
	return ret;
}


unsigned int uc6511_set_version(unsigned int version)
{
//	printk("%s:Blank function\n", __FUNCTION__);
	return 0;
}


struct uc6511_proc {
	char *module_name;
	unsigned int (*uc6511_get) (char *);
	unsigned int (*uc6511_set) (unsigned int);
};

static const struct uc6511_proc uc6511_modules[] = {
	{"version", uc6511_get_version, uc6511_set_version},
};


ssize_t uc6511_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	ssize_t len;
	char *p = page;
	const struct uc6511_proc *proc = data;

	p += proc->uc6511_get(p);

	len = p - page;
	
	*eof = 1;
	
	//printk("len = %d\n", len);
	
	return len;
}

size_t uc6511_proc_write(struct file * filp, const char __user * buf, unsigned long len, void *data)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	char command[PROC_CMD_LEN];
	const struct uc6511_proc *proc = data;

	if (!buf || len > PAGE_SIZE - 1)
		return -EINVAL;

	if (len > PROC_CMD_LEN) {
		printk("Command to long\n");
		return -ENOSPC;
	}
	if (copy_from_user(command, buf, len)) {
		return -EFAULT;
	}

	if (len < 1)
		return -EINVAL;

	if (strnicmp(command, "on", 2) == 0 || strnicmp(command, "1", 1) == 0) {
		proc->uc6511_set(1);
	} else if (strnicmp(command, "off", 3) == 0 || strnicmp(command, "0", 1) == 0) {
		proc->uc6511_set(0);
	} else {
		return -EINVAL;
	}

	return len;
}

//proc file_operations
struct file_operations uc6511r_fops = {
	.owner = THIS_MODULE,
};

static int proc_node_num = (sizeof(uc6511_modules) / sizeof(*uc6511_modules));
static struct proc_dir_entry *update_proc_root;
static struct proc_dir_entry *proc[5];

static int uc6511_proc_init(void)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	unsigned int i;
	//struct proc_dir_entry *proc;

	update_proc_root = proc_mkdir("touch", NULL);

	if (!update_proc_root) 
	{
		printk("create uc6511_touch directory failed\n");
		return -1;
	}

	for (i = 0; i < proc_node_num; i++) 
	{
		mode_t mode;

		mode = 0;
		if (uc6511_modules[i].uc6511_get)
		{
			mode |= S_IRUGO;
		}
		if (uc6511_modules[i].uc6511_set)
		{
			mode |= S_IWUGO;
		}

		proc[i] = create_proc_entry(uc6511_modules[i].module_name, mode, update_proc_root);
		if (proc[i]) 
		{
			proc[i]->data = (void *)(&uc6511_modules[i]);
			proc[i]->read_proc = uc6511_proc_read;
			proc[i]->write_proc = uc6511_proc_write;
		}
	}

	return 0;
}

static void uc6511_proc_remove(void)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	remove_proc_entry("version", update_proc_root);
	remove_proc_entry("touch", NULL);
}

/*********************************Proc-filesystem-BOTTOM**************************************/

static DEVICE_ATTR(disable, 0664, uc6511_disable_show, uc6511_disable_store);

static struct attribute *uc6511_attributes[] = {
	&dev_attr_disable.attr,
	&dev_attr_pen_down.attr,
	NULL
};

static const struct attribute_group uc6511_attr_group = {
	.attrs = uc6511_attributes,
};

static int __devinit uc6511_construct(bus_device *bus, struct uc6511 *ts)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	struct input_dev *input_dev;
	struct uc6511_platform_data *pdata = bus->dev.platform_data;
	int err;
	u16 revid;

//	printk(KERN_INFO "uc6511_construct +\n");
	if (!bus->irq) {
		dev_err(&bus->dev, "no IRQ?\n");
		return -ENODEV;
	}

	if (!pdata) {
		dev_err(&bus->dev, "no platform data?\n");
		return -ENODEV;
	}

	input_dev = input_allocate_device();
	if (!input_dev)
		return -ENOMEM;

	ts->input = input_dev;

	INIT_WORK(&ts->work, uc6511_work);
	mutex_init(&ts->mutex);

	snprintf(ts->phys, sizeof(ts->phys), "%s/input0", dev_name(&bus->dev));

	input_dev->name = "UC6511 Touchscreen";
	input_dev->phys = ts->phys;
	input_dev->dev.parent = &bus->dev;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(ABS_X, input_dev->absbit);
	__set_bit(ABS_Y, input_dev->absbit);
	__set_bit(ABS_PRESSURE, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev, ABS_X, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

	//err = uc6511_write(bus, AD7879_REG_CTRL2, AD7879_RESET);

	if (err < 0) {
		dev_err(&bus->dev, "Failed to write %s\n", input_dev->name);
		goto err_free_mem;
	}

	err = request_irq(bus->irq, uc6511_irq,
			  IRQF_TRIGGER_FALLING, bus->dev.driver->name, ts);

	if (err) {
		dev_err(&bus->dev, "irq %d busy?\n", bus->irq);
		goto err_free_mem;
	}

	err = sysfs_create_group(&bus->dev.kobj, &uc6511_attr_group);
	if (err)
		goto err_free_irq;

	err = input_register_device(input_dev);
	if (err)
		goto err_remove_attr;

	dev_info(&bus->dev, "Rev.%d touchscreen, irq %d\n",
		 revid >> 8, bus->irq);

	//printk(KERN_INFO "uc6511_construct -\n");
	return 0;

err_remove_attr:
	sysfs_remove_group(&bus->dev.kobj, &uc6511_attr_group);
err_free_irq:
	free_irq(bus->irq, ts);
err_free_mem:
	input_free_device(input_dev);

	printk(KERN_ERR "uc6511_construct error\n");
	return err;
}

static int __devexit uc6511_destroy(bus_device *bus, struct uc6511 *ts)
{
//printk(KERN_INFO"touch_uc6511 %s,%d\n",__func__,__LINE__);
	uc6511_disable();
	sysfs_remove_group(&ts->bus->dev.kobj, &uc6511_attr_group);
	free_irq(ts->bus->irq, ts);
	input_unregister_device(ts->input);
	dev_dbg(&bus->dev, "unregistered touchscreen\n");

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void uc6511_early_suspend(struct early_suspend *h)
{
	printk("%s\n",__FUNCTION__);
	uc6511_disable();
	release_power(VDD_TOUCH_POWER);
	printk("uc6511_touch early suspend done!\n");
	
}

static void uc6511_late_resume(struct early_suspend *h)
{
	printk("%s\n",__FUNCTION__);
	request_power(VDD_TOUCH_POWER);
	mdelay(100);
	uc6511_enable();
	mdelay(50);
	printk("uc6511_touch late resume done!\n");
}

#endif
#if 0	//CONFIG_PM
static int uc6511_suspend(bus_device *bus, pm_message_t message)
{
	printk("%s\n",__FUNCTION__);
	struct uc6511 *ts = dev_get_drvdata(&bus->dev);
	
	uc6511_disable(ts);
	mdelay(100);
	release_power(VDD_TOUCH_POWER);
	mdelay(50);
	return 0;
}

static int uc6511_resume(bus_device *bus)
{
	printk("%s\n",__FUNCTION__);
	struct uc6511 *ts = dev_get_drvdata(&bus->dev);
	//mdelay(100);
	request_power(VDD_TOUCH_POWER);
	mdelay(100);
	uc6511_enable(ts);
	mdelay(50);
	return 0;
}
#else
#define uc6511_suspend NULL
#define uc6511_resume  NULL
#endif


/* All registers are word-sized.
 * AD7879 uses a high-byte first convention.
 */

static int __devinit uc6511_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	printk(KERN_INFO"%s\n",__FUNCTION__);
//	struct uc6511 *ts;
	int error;
	request_power(VDD_TOUCH_POWER);
	ts = kzalloc(sizeof(struct uc6511), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	i2c_set_clientdata(client, ts);
	ts->bus = client;
//	printk("==========\n%d\n========\n",ts->bus->irq);
	error = uc6511_construct(client, ts);
	if (error) {
		i2c_set_clientdata(client, NULL);
		kfree(ts);
	}
	error = uc6511_proc_init();
	error = uc6511_fwinfo();
	#ifdef CONFIG_HAS_EARLYSUSPEND
	es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	es.suspend = uc6511_early_suspend;
	es.resume = uc6511_late_resume;
	register_early_suspend(&es);
	#endif
	return error;
}

static int __devexit uc6511_remove(struct i2c_client *client)
{
	struct uc6511 *ts = dev_get_drvdata(&client->dev);

	uc6511_destroy(client, ts);
	i2c_set_clientdata(client, NULL);
	kfree(ts);
	#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&es);
	#endif
	release_power(VDD_TOUCH_POWER);
	return 0;
}

static const struct i2c_device_id uc6511_id[] = {
	{ "uc6511", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, uc6511_id);

static struct i2c_driver uc6511_driver = {
	.driver = {
		.name	= "uc6511",
		.owner	= THIS_MODULE,
	},
	.id_table	= uc6511_id,
	.probe		= uc6511_probe,
	.remove		= __devexit_p(uc6511_remove),
	//.suspend	= uc6511_suspend,
	//.resume		= uc6511_resume,
};

static int __init uc6511_init(void)
{
	printk(KERN_INFO "uc6511_init +\n");
	return i2c_add_driver(&uc6511_driver);
}
module_init(uc6511_init);

static void __exit uc6511_exit(void)
{
	i2c_del_driver(&uc6511_driver);
	printk(KERN_INFO "uc6511_exit +\n");
}
module_exit(uc6511_exit);

MODULE_AUTHOR("Lex Yang <lex.yang@taiwan-sf.com>");
MODULE_DESCRIPTION("UC6511(-1) touchscreen Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:uc6511");
