/*
 * Copyright (C) Chris Desjardins 2008 - code@chrisd.info
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "asm/arch/mux.h"
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include "../../servosoc.h"

MODULE_LICENSE("Dual BSD/GPL");

#define SERVODRIVE_GPIO_BASE        136
#define SERVODRIVE_NAME             "servoctrldrv"
#define SERVODRIVE_PERIOD           20000

int servodrive_open(struct inode *inode, struct file *filp);
int servodrive_release(struct inode *inode, struct file *filp);
ssize_t servodrive_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t servodrive_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

struct file_operations servodrive_funcs = {
    read: servodrive_read,
    write: servodrive_write,
    open: servodrive_open,
    release: servodrive_release
};

typedef struct
{
    int m_nGpio;
    unsigned long m_nDelay;
} TServoDrvData;

int g_nServoDrvMajor = 0;
int g_nServoDrvOpen = 0;
struct timer_list g_tlServoDrvPwm;
TServoDrvData g_ServoDrvList[SERVOSOC_MAX_SERVOS];


void servodrive_pwm(unsigned long arg);

static int servodrive_init_mux(int mux_start_index, int mux_end_index)
{
    int i;
    int ret = 0;
    for (i = mux_start_index; i <= mux_end_index; i++)
    {
        ret = omap_cfg_reg(i);
        if (ret != 0)
        {
            printk("<1>Servodrive: omap_cfg_reg failed\n");
            break;
        }
    }
    return ret;
}

static int servodrive_init_data(void)
{
    int i;
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++)
    {
        gpio_direction_output(i + SERVODRIVE_GPIO_BASE, 0);
        g_ServoDrvList[i].m_nGpio   = -1;
        g_ServoDrvList[i].m_nDelay  = 0;
    }
    return 0;
}

static int servodrive_init(void) 
{
    int ret;
    printk("<1>Starting servo drive %s %s\n", __DATE__, __TIME__);

    ret = servodrive_init_mux(XXXX_3430_GPIO_136, XXXX_3430_GPIO_143);
    if (ret != 0)
    {
        return ret;
    }
    
    ret = servodrive_init_data();
    if (ret != 0)
    {
        return ret;
    }
    g_nServoDrvMajor = register_chrdev(g_nServoDrvMajor, SERVODRIVE_NAME, &servodrive_funcs);
    if (g_nServoDrvMajor < 0)
    {
        printk("<l>Servodrive error: %i\n", g_nServoDrvMajor);
        return g_nServoDrvMajor;
    }
    printk("<1>major=%i\n", g_nServoDrvMajor);

    return 0;
}

int servodrive_open(struct inode *inode, struct file *filp)
{
    printk("<1>Servoctrldrv open\n");
    g_nServoDrvOpen = 1;
    init_timer(&g_tlServoDrvPwm);
    g_tlServoDrvPwm.function = servodrive_pwm;
    g_tlServoDrvPwm.expires = jiffies + usecs_to_jiffies(SERVODRIVE_PERIOD);
    add_timer(&g_tlServoDrvPwm);

    return 0;
}

ssize_t servodrive_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

ssize_t servodrive_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int nServoIndex;
    TServoData sServoList[SERVOSOC_MAX_SERVOS];

    memset(sServoList, 0, sizeof(sServoList));
    if (copy_from_user(sServoList, buf, count))
    {
        printk("<1>Servodrive: write error\n");
        return -EFAULT;
    }
    for (nServoIndex = 0; nServoIndex < SERVOSOC_MAX_SERVOS; nServoIndex++)
    {
        switch (sServoList[nServoIndex].m_nServoNumber)
        {
        case SERVOSOC_SERVO_A:
            g_ServoDrvList[nServoIndex].m_nGpio = 139;
            break;
        case SERVOSOC_SERVO_B:
            g_ServoDrvList[nServoIndex].m_nGpio = 137;
            break;
        case SERVOSOC_SERVO_INVALID:
            g_ServoDrvList[nServoIndex].m_nGpio = -1;
            break;
        default:
            printk("<1> I dont even have this line hooked up to hardware, give me a break!\n");
            break;
        }
        if (g_ServoDrvList[nServoIndex].m_nGpio != -1)
        {
            g_ServoDrvList[nServoIndex].m_nDelay = sServoList[nServoIndex].m_nJoyValue;
        }
    }
    *f_pos += count;
    return count;
}

int servodrive_release(struct inode *inode, struct file *filp)
{
    g_nServoDrvOpen = 0;
    del_timer_sync(&g_tlServoDrvPwm);
    return 0;
}

void servodrive_pwm(unsigned long arg)
{
    /* restart this timer for the next period */
    int i;
    unsigned long nTotalDelay = 0;
    int nServos = 0;
    if (g_nServoDrvOpen == 1)
    {
        g_tlServoDrvPwm.expires = jiffies + usecs_to_jiffies(SERVODRIVE_PERIOD);
        add_timer(&g_tlServoDrvPwm);
    }
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++)
    {
        if (g_ServoDrvList[i].m_nGpio != -1)
        {
            gpio_set_value(g_ServoDrvList[i].m_nGpio, 1);
        }
        else
        {
            break;
        }
        nServos++;
    }
    for (i = 0; i < nServos; i++)
    {
        if (g_ServoDrvList[i].m_nGpio != -1)
        {
            udelay(g_ServoDrvList[i].m_nDelay - nTotalDelay);
            nTotalDelay += g_ServoDrvList[i].m_nDelay;
            gpio_set_value(g_ServoDrvList[i].m_nGpio, 0);
        }
    }
}

static void servodrive_exit(void) 
{
    printk("<1>Exiting servodrive\n");
    unregister_chrdev(g_nServoDrvMajor, SERVODRIVE_NAME);
}

module_init(servodrive_init);
module_exit(servodrive_exit);
