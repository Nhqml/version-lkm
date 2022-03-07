#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#include <linux/module.h>
#include <linux/miscdevice.h>

#include <linux/utsname.h>

#include <linux/uaccess.h>

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/printk.h>

#include "version.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define LEN_BUF 65
static char ORIGINAL_VERSION[LEN_BUF];

static char *version;
static struct mutex version_mutex;

static void restore_version(void)
{
    mutex_lock(&version_mutex);
    strncpy(version, ORIGINAL_VERSION, LEN_BUF);
    mutex_unlock(&version_mutex);
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    static size_t len;

    size_t len_to_copy;

    pr_devel("READ");

    mutex_lock(&version_mutex);

    // First call, we retrieve the data
    if (*offset == 0)
    {
        len = strlen(version) + 1;
        // Add a '\n' at the end
        version[len - 1] = '\n';
    }

    len_to_copy = len - *offset;
    if (copy_to_user(buffer, version + *offset, len_to_copy))
        return -EFAULT;
    *offset += len_to_copy;

    // Everything has been read, we put back the '\0' at the end of the string
    if (*offset == len)
        version[len - 1] = '\0';

    mutex_unlock(&version_mutex);

    return len_to_copy;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    pr_devel("WRITE");

    mutex_lock(&version_mutex);

    copy_from_user(version, buffer, MIN(length, LEN_BUF - 1));
    version[length] = '\0';

    mutex_unlock(&version_mutex);

    return length;
}

static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    switch (ioctl_num)
    {
    case VERSION_MODIFIED:
        pr_devel("IOCTL: VERSION_MODIFIED");
        mutex_lock(&version_mutex);
        put_user((strcmp(ORIGINAL_VERSION, version) != 0), (int __user *)ioctl_param);
        mutex_unlock(&version_mutex);
        break;
    case VERSION_RESET:
        pr_devel("IOCTL: VERSION_RESET");
        restore_version();
        break;
    }

    return 0;
}

static struct file_operations device_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
};

static struct miscdevice device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "version",
    .fops = &device_fops,
};

static __init int version_init(void)
{
    int r;
    pr_devel("INIT");

    r = misc_register(&device);

    if (r < 0)
    {
        pr_err("could not register misc device (%d)\n", r);
        return r;
    }
    else
        pr_devel("Registered misc device (%d)\n", r);

    // Initialize global variables
    version = utsname()->version;
    mutex_init(&version_mutex);

    // Save original version
    strncpy(ORIGINAL_VERSION, version, LEN_BUF);
    pr_info("Saved original version '%s'", ORIGINAL_VERSION);
    return 0;
}

static __exit void version_exit(void)
{
    pr_devel("EXIT");
    misc_deregister(&device);
    restore_version();
    pr_info("Restored original version '%s'", ORIGINAL_VERSION);
}

module_init(version_init);
module_exit(version_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Version module");
MODULE_AUTHOR("Kenji Gaillac");
