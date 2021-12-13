#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#define BUFF_SIZE 20

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

DECLARE_WAIT_QUEUE_HEAD(readQ);
DECLARE_WAIT_QUEUE_HEAD(writeQ);
struct semaphore sem;


int alu[6];
int pos1 = 0;
int pos2 = 0;
#define num_of_minors 6
int storage[num_of_minors];
int endRead = 0;
int form=1;

int alu_open(struct inode *pinode, struct file *pfile);
int alu_close(struct inode *pinode, struct file *pfile);
ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t alu_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = alu_open,
	.read = alu_read,
	.write = alu_write,
	.release = alu_close,
};


int alu_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened file\n");
		return 0;
}

int alu_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed file\n");
		return 0;
}

ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len=0;
	int minor = MINOR(pfile->f_inode->i_rdev);
	/*if(pos1<4)
	{
		printk(KERN_INFO "Potrebno je izvrsiti operaciju pre sledeceg citanja\n");
	}	
	
	if(down_interruptible(&sem))
		return -ERESTARTSYS;
	
	while(pos1 < 4)
	{
		up(&sem);
		if(wait_event_interruptible(readQ,(pos1>=4)))
			return -ERESTARTSYS;
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
	}*/
	if(pos1>=4 && pos1<6)
	{	
		if(form==1)
		{
			len = scnprintf(buff,BUFF_SIZE , "%d ", alu[pos1]);
		}
		else if (form==2)
		{
			int i;
			int a[10];
			int n=alu[pos1];
			int var1=0;
			int cnt;
			int exp=1;
			for(i=0;n>0;i++)    
			{    
				a[i]=n%2;    
				n=n/2;
				cnt=i;
			}  
			for(i=0;i<=cnt;i++)
			{
				var1+=a[i]*exp;
				exp=exp*10;
			}
			alu[pos1]=var1;
			len = scnprintf(buff,BUFF_SIZE , "%d ", alu[pos1]);
			
		}
		else if (form==3)
		{
			len = scnprintf(buff,BUFF_SIZE , "%x ", alu[pos1]);
		}	
		ret = copy_to_user(buffer, buff, len);
		if(ret)
			return -EFAULT;
		pos1 ++;
	}
	else if (pos1 == 6) {
		endRead = 1;
	}
	
	/*up(&sem);
	wake_up_interruptible(&writeQ);*/
	
	if (endRead){
		endRead = 0;
		printk(KERN_INFO "Succesfully read from file\n");
		pos1=0;
		return 0;
	}
	
	return len;
}

ssize_t alu_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
	char buff[BUFF_SIZE];
	char position1[5]={0};
	char position2[5]={0};
	char operation[2]={0};
	char format[4]={0};
	int value;
	int ret;
	int result;
	int minor = MINOR(pfile->f_inode->i_rdev);
	ret = copy_from_user(buff, buffer, length);
	if(ret)
		return -EFAULT;
	buff[length-1] = '\0';
	
	/*if(pos1>=4 || pos2>=4)
	{
		printk(KERN_INFO "Potrebno je izvrsiti citanje pre sledece operacije\n");
	}	
	if(down_interruptible(&sem))
		return -ERESTARTSYS;
	while(pos1 >= 4 || pos2 >=4)
	{
		up(&sem);
		if(wait_event_interruptible(writeQ,(pos1<4 && pos2<4)))
			return -ERESTARTSYS;
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
	}*/
	
	if(pos1<4 && pos2<4)
		{	
		ret = sscanf(buff,"%4s=%d",position1,&value);
		if(ret!=2)
		{
			ret=sscanf(buff,"%4s%*c%1s%*c%4s",position1,operation,position2);
			if(ret!=3)
			{
				ret=sscanf(buff,"format=%3s",format);
			}	
		}	
			
		//VARIJANTA 1 UNOS VREDNOSTI U REGISTAR
		if(ret==2)
		{	
			if(strcmp(position1,"regA")==0)
			{
				pos1=0;
			}
			else if(strcmp(position1,"regB")==0)
			{
				pos1=1;
			}
			else if(strcmp(position1,"regC")==0)
			{
				pos1=2;
			}
			else if(strcmp(position1,"regD")==0)
			{
				pos1=3;
			}
			
			if(value >=0 && value <256)
			{
				if(pos1 >=0 && pos1 <=3)
				{
					alu[pos1] = value; 
					printk(KERN_INFO "Succesfully wrote value in  register\n"); 
				}
				else
				{
					printk(KERN_WARNING "Register should be between A and D\n"); 
				}
			}
		}
		
		//VARIJANTA 2 OPERACIJA SA REGISTRIMA
		
		else if (ret==3)
		{
			//prvi registar
			if(strcmp(position1,"regA")==0)
			{
				pos1=0;
			}
			else if(strcmp(position1,"regB")==0)
			{
				pos1=1;
			}
			else if(strcmp(position1,"regC")==0)
			{
				pos1=2;
			}
			else if(strcmp(position1,"regD")==0)
			{
				pos1=3;
			}
			
			//drugi registar
			if(strcmp(position2,"regA")==0)
			{
				pos2=0;
			}
			else if(strcmp(position2,"regB")==0)
			{
				pos2=1;
			}
			else if(strcmp(position2,"regC")==0)
			{
				pos2=2;
			}
			else if(strcmp(position2,"regD")==0)
			{
				pos2=3;
			}
			
			if(pos1 >=0 && pos1 <=3 && pos2>=0 && pos2<=3)
			{
				//operacija
				if(strcmp(operation,"+")==0)
				{
					result = alu[pos1]+alu[pos2];
				}
				else if(strcmp(operation,"-")==0)
				{
					result = alu[pos1]-alu[pos2];
				}
				else if(strcmp(operation,"x")==0) // * ne radi
				{
					result = alu[pos1]*alu[pos2];
				}
				else if(strcmp(operation,"/")==0)
				{
					result = alu[pos1]/alu[pos2];
				}
				
				else
				{
					printk(KERN_WARNING "Uneti operaciju + - x / \n");
				}
				
				//upis rezultata
				if(result>255 || result<0)
				{
					result=(unsigned char) result;
					alu[4]=result; //alu[4] je pozicija rezultata
					alu[5]=1; //alu[5] je pozicija overflow-a (carry)
				}
				else
				{
					alu[4]=result;
					alu[5]=0;
				}
				//printk(KERN_INFO "Succesfully executed operation %d in register %d\n", value, position1); 	
				pos1=4;
			}
			
		}
		//VARIJANTA 3 Unos formata
		else if (ret==1)
		{
			if(strcmp(format,"dec")==0)
			{
				form=1;
			}
			else if(strcmp(format,"bin")==0)
			{
				form=2;
			}
			else if(strcmp(format,"hex")==0)
			{
				form=3;
			}
			
		}	
	}
	else
	{
		printk(KERN_WARNING "Wrong command format\nexpected: regX=num\n X=A,B,C,D\n\num={0-255}\n");
	}
	
	/*up(&sem);
	wake_up_interruptible(&readQ);*/

	return length;
}

static int __init alu_init(void)
{
    int ret = 0;
	int i=0;
	sema_init(&sem,1);

	//Initialize array
	for (i=0; i<6; i++)
		alu[i] = 0;

   ret = alloc_chrdev_region(&my_dev_id, 0, num_of_minors, "alu");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "alu_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
  printk(KERN_INFO "creating device\n");
  for (i = 0; i < num_of_minors; i++)
  {
    printk(KERN_INFO "created nod %d\n", i);
	if(i==0)
	{	
		scnprintf(buff, 10, "alu_regA");
	}
	else if(i==1)
	{
		scnprintf(buff, 10, "alu_regB");
	}
	else if(i==2)
	{
		scnprintf(buff, 10, "alu_regC");
	}
	else if(i==3)
	{
		scnprintf(buff, 10, "alu_regD");
	}	
	else if(i==4)
	{
		scnprintf(buff, 10, "alu_op");
	}
	else if(i==5)
	{
		scnprintf(buff, 10, "alu_result");
	}
    my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id), i), NULL, buff);
    if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
    }

  }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit alu_exit(void)
{
   cdev_del(my_cdev);
   for (i = 0; i < num_of_minors; i++) // every node made must be destroyed
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id), i));
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(alu_init);
module_exit(alu_exit);
