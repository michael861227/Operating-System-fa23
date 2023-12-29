 #include <linux/atomic.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/kernel.h> /* for sprintf() */ 
#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/types.h> 
#include <linux/uaccess.h> /* for get_user and put_user */ 
#include <linux/version.h> 
#include <asm/errno.h> 
#include "kfetch.h"
#include <linux/utsname.h>
#include <linux/sysinfo.h>
#include <linux/mm.h>
#include <asm/processor.h>
#include <linux/jiffies.h>
 
/*  Prototypes - this would normally go in a .h file */ 
static int device_open(struct inode *, struct file *); 
static int device_release(struct inode *, struct file *); 
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *); 
 
#define SUCCESS 0 
#define DEVICE_NAME "kfetch" /* Dev name as it appears in /proc/devices   */ 

#define KERNEL_INDEX 0
#define CPUS_INDEX 1
#define CPU_INDEX 2
#define MEM_INDEX 3
#define UPTIME_INDEX 4
#define PROCS_INDEX 5

/* Global variables are declared as static, so are global within the file. */ 
 
static int major; /* major number assigned to our device driver */ 
 
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
}; 
 
/* Is device open? Used to prevent multiple access to device */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
 
static struct class *cls; 
 
static struct file_operations kfetch_ops = { 
    .read = device_read, 
    .write = device_write, 
    .open = device_open, 
    .release = device_release, 
}; 

int kfetch_mask[KFETCH_NUM_INFO] = {1, 1, 1, 1, 1, 1};



static int __init kfetch_init(void) 
{ 
    major = register_chrdev(0, DEVICE_NAME, &kfetch_ops); 
 
    if (major < 0) { 
        pr_alert("Registering char device failed with %d\n", major); 
        return major; 
    } 
 
    pr_info("I was assigned major number %d.\n", major); 
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
    cls = class_create(DEVICE_NAME); 
#else 
    cls = class_create(THIS_MODULE, DEVICE_NAME); 
#endif 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 
 
    pr_info("Device created on /dev/%s\n", DEVICE_NAME); 
 
    return SUCCESS; 
} 
 
static void __exit kfetch_exit(void) 
{ 
    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls); 
 
    /* Unregister the device */ 
    unregister_chrdev(major, DEVICE_NAME); 
} 
 
/* Methods */ 
 
/* Called when a process tries to open the device file, like 
 * "sudo cat /dev/chardev" 
 */ 
static int device_open(struct inode *inode, struct file *file) 
{ 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 
 
    try_module_get(THIS_MODULE); 
 
    return SUCCESS; 
} 
 
/* Called when a process closes the device file. */ 
static int device_release(struct inode *inode, struct file *file) 
{ 
    /* We're now ready for our next caller */ 
    atomic_set(&already_open, CDEV_NOT_USED); 
 
    /* Decrement the usage count, or else once you opened the file, you will 
     * never get rid of the module. 
     */ 
    module_put(THIS_MODULE); 
 
    return SUCCESS; 
}

char* get_Mem(void) {
    struct sysinfo info;
    si_meminfo(&info);

    char *mem_total = (char *) kmalloc(20, GFP_KERNEL);
    char *mem_free = (char *) kmalloc(20, GFP_KERNEL);
    char *mem = (char *) kmalloc(20, GFP_KERNEL);


    memset(mem_total, '\0', 20);
    memset(mem_free, '\0', 20);
    memset(mem, '\0', 20);
    

    sprintf(mem_total, "%lu", (info.totalram << (PAGE_SHIFT - 10)) / 1024);
    sprintf(mem_free, "%lu", (info.freeram << (PAGE_SHIFT - 10)) / 1024);
    sprintf(mem, "%s MB / %s MB", mem_free, mem_total);


    kfree(mem_total);
    kfree(mem_free);
    return mem;
}

char *get_CPU(void) {
    struct cpuinfo_x86 *c = &cpu_data(0);
    return c->x86_model_id;
}

char *get_CPUs(void) {
    unsigned int online_cpus, total_cpus;
    char *CPUs = (char *) kmalloc(20, GFP_KERNEL);
    memset(CPUs, '\0', 20);

    // Get the online CPUs
    online_cpus = num_online_cpus();

    // Get the total CPUs
    total_cpus = num_possible_cpus();

    sprintf(CPUs, "%u / %u", online_cpus, total_cpus);

    return CPUs;
}

char *get_thread(void) {
    int count = 0;
    struct task_struct *p;
    
    char *thread_num = (char *) kmalloc(20, GFP_KERNEL);
    memset(thread_num, '\0', 20);

    // Iterate through the task list
    for_each_process(p) {
        // Count each thread
        count += get_nr_threads(p);
    }

    sprintf(thread_num, "%d", count);

    return thread_num;
}

char *get_uptime(void) {
    unsigned long uptime_jiffies;
    unsigned long minutes;
    
    char *uptime = (char *) kmalloc(20, GFP_KERNEL);
    memset(uptime, '\0', 20);

    // Get the current value of jiffies
    uptime_jiffies = jiffies;

    // Convert jiffies to minutes
    minutes = jiffies_to_msecs(uptime_jiffies) / (1000 * 60);

    sprintf(uptime, "%lu mins", minutes);

    return uptime;
}

void setup_hostname(char *kfetch_buf) {
    char *dash = (char *) kmalloc(strlen(utsname()->nodename) + 1, GFP_KERNEL);
    memset(dash, '-', strlen(utsname()->nodename));
    dash[strlen(utsname()->nodename)] = '\0';

    memset(kfetch_buf, ' ', 19);
    strcat(kfetch_buf, utsname()->nodename);
    strcat(kfetch_buf, "\n");
    strcat(kfetch_buf, "        .-.        ");
    strcat(kfetch_buf, dash);
    strcat(kfetch_buf, "\n");

    kfree(dash);
}


void setup_info(char *kfetch_buf) {
    char *info[KFETCH_NUM_INFO] = {
        "Kernel:   ",
        "CPU:      ",
        "CPUs:     ",
        "Mem:      ",
        "Procs:    ",
        "Uptime:   "
    };

    char *info_func[KFETCH_NUM_INFO] = {
        utsname() -> release,
        get_CPU(),
        get_CPUs(),
        get_Mem(),
        get_thread(),
        get_uptime()
    };

    int index[KFETCH_NUM_INFO];
    int order[KFETCH_NUM_INFO] = {KERNEL_INDEX, CPU_INDEX, CPUS_INDEX, MEM_INDEX, PROCS_INDEX, UPTIME_INDEX};
    int count;
    bool print_info;

    count = 0;
    for (int i = 0; i < KFETCH_NUM_INFO; i++) {
        if (kfetch_mask[order[i]]) {
            index[count++] = i;
        }
    }

    
    print_info = true;
    for (int i = 0; i < KFETCH_NUM_INFO; i++) {
        if (i >= count)
            print_info = false;
        
        switch (i)
        {
            case 0:
                if (print_info) {
                    strcat(kfetch_buf, "       (.. |       ");
                    strcat(kfetch_buf, info[index[i]]);
                    strcat(kfetch_buf, info_func[index[i]]);
                    strcat(kfetch_buf, "\n");
                }
                else {
                    strcat(kfetch_buf, "       (.. |       ");
                    strcat(kfetch_buf, "\n");
                }
                break;
            
            case 1:
                if (print_info) {
                    strcat(kfetch_buf, "       <>  |       ");
                    strcat(kfetch_buf, info[index[i]]);
                    strcat(kfetch_buf, info_func[index[i]]);
                    strcat(kfetch_buf, "\n");
                } else {
                    strcat(kfetch_buf, "       <>  |       ");
                    strcat(kfetch_buf, "\n");
                }
                break;
            
            case 2: 
                if (print_info) {
                    strcat(kfetch_buf, "      / --- \\      ");
                    strcat(kfetch_buf, info[index[i]]);
                    strcat(kfetch_buf, info_func[index[i]]);
                    strcat(kfetch_buf, "\n");
                } else {
                    strcat(kfetch_buf, "      / --- \\      ");
                    strcat(kfetch_buf, "\n");
                }
                break;
            
            case 3: 
                if (print_info) {
                    strcat(kfetch_buf, "     ( |   | |     ");
                    strcat(kfetch_buf, info[index[i]]);
                    strcat(kfetch_buf, info_func[index[i]]);
                    strcat(kfetch_buf, "\n");
                } else {
                    strcat(kfetch_buf, "     ( |   | |     ");
                    strcat(kfetch_buf, "\n");
                }
                break;
            
            case 4: 
                if (print_info) {
                    strcat(kfetch_buf, "   |\\_)___/\\)/\\    ");
                    strcat(kfetch_buf, info[index[i]]);
                    strcat(kfetch_buf, info_func[index[i]]);
                    strcat(kfetch_buf, "\n");
                } else {
                    strcat(kfetch_buf, "   |\\_)___/\\)/\\    ");
                    strcat(kfetch_buf, "\n");
                }
                break;
            
            case 5: 
                if (print_info) {
                    strcat(kfetch_buf, "  <__)------(__/   ");
                    strcat(kfetch_buf, info[index[i]]);
                    strcat(kfetch_buf, info_func[index[i]]);
                    strcat(kfetch_buf, "\n");
                } else {
                    strcat(kfetch_buf, "  <__)------(__/   ");
                    strcat(kfetch_buf, "\n");
                }
                break;

            default:
                break;    
        }

    }

}

/* Called when a process, which already opened the dev file, attempts to 
 * read from it. 
 */ 
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */ 
                           char __user *buffer, /* buffer to fill with data */ 
                           size_t len, /* length of the buffer     */ 
                           loff_t *offset) 
{ 
    /* fetching the information */
    char *kfetch_buf = (char *) kmalloc(KFETCH_BUF_SIZE, GFP_KERNEL);
    int length = 0;

    // Initailize the buffer
    memset(kfetch_buf, '\0', KFETCH_BUF_SIZE);

    // Set the information
    setup_hostname(kfetch_buf);
    setup_info(kfetch_buf);
    

    if (copy_to_user(buffer, kfetch_buf, KFETCH_BUF_SIZE)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    
    /* cleaning up */
    length = strlen(kfetch_buf);
    kfree(kfetch_buf);

    return length;
} 

void write_kfetch_mask(int mask_info) {
    // Initailize the mask
    for (int i = 0; i < KFETCH_NUM_INFO; i++) {
        kfetch_mask[i] = 0;
    }

    // Set the mask
    int bit = 1;

    for (int i = 0; i < KFETCH_NUM_INFO; i++) {
        if (mask_info & bit) {
            kfetch_mask[i] = 1;
        }
        bit <<= 1;
    }
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello */ 
static ssize_t device_write(struct file *filp, const char __user *buffer, size_t len, loff_t *offset) 
{ 
    int mask_info = 0;

    if (copy_from_user(&mask_info, buffer, len)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    /* setting the information mask */
    write_kfetch_mask(mask_info);
    return 0;
} 
 
module_init(kfetch_init); 
module_exit(kfetch_exit); 
 
MODULE_LICENSE("GPL");