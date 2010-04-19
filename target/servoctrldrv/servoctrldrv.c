/*
 * Copyright (C) Chris Desjardins 2008 - code@chrisd.info
 */
// http://www.mjmwired.net/kernel/Documentation/gpio.txt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <plat/mux.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include "../../servosoc.h"

MODULE_LICENSE("Dual BSD/GPL");

#define SERVODRIVE_NAME             "servoctrldrv"
#define SERVODRIVE_PERIOD           20000


/* pin gpio no      servo name */
/* 5   138          A */
/* 7   137          B */
/* 9   136          C */
/* 11  135          D */
/* 12  158          E */
/* 13  134          F */
/* 14  162          G */
/* 15  133          H */

int gpio_bases[] =   {  138,   137,   136,   135,   158,   134,   162,   133};

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
    //for (i = mux_start_index; i <= mux_end_index; i++)
    //{
        //ret = omap_cfg_reg(gpio_bases[i]);
        //if (ret != 0)
        //{
            //printk("<1>Servodrive: omap_cfg_reg failed\n");
            //break;
        //}
    //}
    return ret;
}

static int servodrive_init_data(void)
{
    int i;
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++)
    {
        if (gpio_request(gpio_bases[i], "SERVODRIVE")) {
            printk("<1>Servodrive: gpio_request failed for: %i\n", gpio_bases[i]);
        }

        gpio_direction_output(gpio_bases[i], 0);
        g_ServoDrvList[i].m_nGpio   = -1;
        g_ServoDrvList[i].m_nDelay  = 0;
    }
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++)
    {
        gpio_direction_output(gpio_bases[i], 0);
        g_ServoDrvList[i].m_nGpio   = gpio_bases[i];
        g_ServoDrvList[i].m_nDelay  = 1500;
    }
    return 0;
}

static int servodrive_init(void) 
{
    int ret;
    printk("<1>Starting servo drive %s %s\n", __DATE__, __TIME__);

    ret = servodrive_init_mux(SERVOSOC_SERVO_A, SERVOSOC_SERVO_H);
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
        case SERVOSOC_SERVO_B:
        case SERVOSOC_SERVO_C:
        case SERVOSOC_SERVO_D:
        case SERVOSOC_SERVO_E:
        case SERVOSOC_SERVO_F:
        case SERVOSOC_SERVO_G:
        case SERVOSOC_SERVO_H:
            g_ServoDrvList[nServoIndex].m_nGpio = gpio_bases[sServoList[nServoIndex].m_nServoNumber];
            g_ServoDrvList[nServoIndex].m_nDelay = sServoList[nServoIndex].m_nJoyValue;
            break;
        case SERVOSOC_SERVO_INVALID:
            g_ServoDrvList[nServoIndex].m_nGpio = -1;
            break;
        default:
            printk("<1> I dont even have this line hooked up to hardware, give me a break!\n");
            break;
        }
    }
    *f_pos += count;
    return count;
}

int servodrive_release(struct inode *inode, struct file *filp)
{
    int i;
    g_nServoDrvOpen = 0;
    del_timer_sync(&g_tlServoDrvPwm);
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++) {
        gpio_free(gpio_bases[i]);
    }
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
