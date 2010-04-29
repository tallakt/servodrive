/*
 * Copyright (C) Chris Desjardins 2008 - code@chrisd.info
 * Copyright (C) Tallak Tveide 2010 - tallak@tveide.net
 */
/* http://www.mjmwired.net/kernel/Documentation/gpio.txt */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/device.h>

MODULE_LICENSE("Dual BSD/GPL");

#define SERVODRIVE_NAME             "servodrive"
#define SERVODRIVE_PERIOD           20000

int id = 0;
static struct class *servodrive_class;

#define FIRST_GPIO_PIN 3
#define LAST_GPIO_PIN 24
#define GPIO_TOTAL_COUNT (LAST_GPIO_PIN - FIRST_GPIO_PIN + 1)


/* Any GPIO pin on the beagleboard expansionheader may be used as PWM output, */
/* but without any particular modifications on my behalf, the followig pins */
/* on the expansion header work: */
/* 3 - 5 - 7 - 9 - 11 - 12 - ... - 23 - 24 */
int pin_gpio[] =   
  {  
    139, /* PIN 3 */
    144, /* PIN 4 */
    138, /* PIN 5 */
    146, /* PIN 6 */
    137, /* PIN 7 */
    143, /* PIN 8 */
    136, /* PIN 9 */
    145, /* PIN 10 */
    135, /* PIN 11 */
    158, /* PIN 12 */
    134, /* PIN 13 */
    162, /* PIN 14 */
    133, /* PIN 15 */
    161, /* PIN 16 */
    132, /* PIN 17 */
    159, /* PIN 18 */
    131, /* PIN 19 */
    156, /* PIN 20 */
    130, /* PIN 21 */
    157, /* PIN 22 */
    183, /* PIN 23 */
    168  /* PIN 24 */
  };

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


int g_nServoDrvMajor = 0;
int g_nServoDrvOpen = 0;
struct timer_list g_tlServoDrvPwm;
unsigned long pulse_time_usec[GPIO_TOTAL_COUNT];  /* disabled: 0, min: 1100, neutral: 1500, max: 1900 */
unsigned long pulse_time_usec_frozen[GPIO_TOTAL_COUNT];
int sorted_by_pulse_time[GPIO_TOTAL_COUNT];
int sorted_by_pulse_time_frozen[GPIO_TOTAL_COUNT];
int dirty = 0;
unsigned long pulse_count = 0;


void servodrive_pwm(unsigned long arg);


void print_sorted_list(void) {
  int i;
  for (i = 0; i < GPIO_TOTAL_COUNT; ++i) {
    printk("<2>Servodrive: No %02d is pin %02d at %lu usec\n", i + 1, sorted_by_pulse_time[i], pulse_time_usec[sorted_by_pulse_time[i] - FIRST_GPIO_PIN]);
  }
}


static void servodrive_init_data(void) {
    int i;
    for (i = 0; i < GPIO_TOTAL_COUNT; i++) { 
      pulse_time_usec[i] = 0;
      sorted_by_pulse_time[i] = i + FIRST_GPIO_PIN;
    }
    dirty = -1;
}

static int servodrive_init(void) {
    printk("<1>Starting servo drive %s %s\n", __DATE__, __TIME__);

    servodrive_init_data();
    /*printk("<2>Initialized list:\n"); */
    /* print_sorted_list();*/

    g_nServoDrvMajor = register_chrdev(g_nServoDrvMajor, SERVODRIVE_NAME, &servodrive_funcs);
    if (g_nServoDrvMajor < 0)
    {
        printk("<l>Servodrive error: %i\n", g_nServoDrvMajor);
        return g_nServoDrvMajor;
    }
    printk("<1>major=%i\n", g_nServoDrvMajor);
    servodrive_class = class_create(THIS_MODULE, SERVODRIVE_NAME);
    device_create(servodrive_class, NULL, MKDEV(g_nServoDrvMajor, id), NULL, SERVODRIVE_NAME "%d", id);

    g_nServoDrvOpen = 1;
    init_timer(&g_tlServoDrvPwm);
    g_tlServoDrvPwm.function = servodrive_pwm;
    g_tlServoDrvPwm.expires = jiffies + usecs_to_jiffies(SERVODRIVE_PERIOD);
    add_timer(&g_tlServoDrvPwm);

    return 0;
}

int servodrive_open(struct inode *inode, struct file *filp) {
    printk("<1>Servodrive open\n");
    return 0;
}

ssize_t servodrive_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    return 0;
}

void handle_input(char* first, char* last) {
  char* p;
  char* a;
  char* b;
  int failed;
  int pin;
  int new_pulse_time_us;
  int percentage;
  char buffer[128];

  failed = 0;

  for (p = first; p != last; ++p) {
    *p = toupper(*p);
  }
  /* valid commands are "1:100" "14:-100" "20:OFF", on separate lines */

  /* first number */
  a = first;
  pin = simple_strtol(first, &a, 10);
  if (a == first) {
    failed = -1;
  };
  if ((FIRST_GPIO_PIN > pin) || (pin > LAST_GPIO_PIN)) {
     failed = -1;
  } 

  /* skip colon */
  if (*a == ':') {
    ++a;
  } else {
    failed = -1;
  }

  /* check for off */
  if (strncmp("OFF", a, last - a) == 0) {
    new_pulse_time_us = 0;
  } else {
    /* check for signed number */
    b = a;
    percentage = simple_strtol(b, &b, 10);
    if (a == b) {
      failed = -1;
    } else {
      if (-100 <= percentage && percentage <= 100) {
        new_pulse_time_us = 1500 + 4 * percentage;
      } else {
        failed = -1;
      }
    }
  }

  if (!failed) {
    if (!pulse_time_usec[pin - FIRST_GPIO_PIN] && new_pulse_time_us) {
      if (gpio_request(pin_gpio[pin - FIRST_GPIO_PIN], "SERVODRIVE")) {
          printk("<1>Servodrive: gpio_request failed for pin %i or GPIO%d\n", pin, pin_gpio[pin - FIRST_GPIO_PIN]);
      }
      gpio_direction_output(pin_gpio[pin - FIRST_GPIO_PIN], 0);
    }
    if (pulse_time_usec[pin - FIRST_GPIO_PIN] && !new_pulse_time_us) {
      gpio_free(pin_gpio[pin - FIRST_GPIO_PIN]);
    }
    pulse_time_usec[pin - FIRST_GPIO_PIN] = new_pulse_time_us;
    printk("<1>Servodrive: output of pin %i set to %i us\n", pin, new_pulse_time_us);
  } else {
    strncpy(buffer, first, last - first);
    printk("<1>Servodrive: could not parse line: %s\n", buffer);
  }
}


void sort_by_pulse_time(void) {
  int i, j, temp;
 
  for (i = (GPIO_TOTAL_COUNT - 1); i >= 0; i--) {
    for (j = 1; j <= i; j++) {
      if (pulse_time_usec[sorted_by_pulse_time[j-1] - FIRST_GPIO_PIN] > pulse_time_usec[sorted_by_pulse_time[j] - FIRST_GPIO_PIN]) {
        temp = sorted_by_pulse_time[j-1];
        sorted_by_pulse_time[j-1] = sorted_by_pulse_time[j];
        sorted_by_pulse_time[j] = temp;
      }
    }
  }
}

ssize_t servodrive_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  int safe_count;
  char buffer[128];
  char* first;
  char* last; 
  unsigned long result;

  dirty = 0; /* dont update while changing values */
  safe_count = count > 127 ? 127 : count;
  result = copy_from_user(buffer, buf, safe_count);
  if (result == 0) {
    buffer[safe_count] = 0;

    /* split into strings */
    first = buffer;
    last = buffer;
    while (*first != 0) {
      while (*last != '\n' && *last != 0) {
        ++last;
      }
      if (first != last) {
        handle_input(first, last);
      }
      first = last;
      if (first != 0) {
        ++first;
        ++last;
      }
    }

    sort_by_pulse_time();

    /* print_sorted_list(); */
    printk("<2>Pulse count: %lu\n", pulse_count);

    /* next timer interrupt will copy newly sorted list and pulse sized across at start */
    dirty = -1; /* dont update while changing values */

    *f_pos += count;
    return count;
  }
  else {
    return -1;
  }
}


int servodrive_release(struct inode *inode, struct file *filp) {
    return 0;
}

void servodrive_pwm(unsigned long arg) {
    /* restart this timer for the next period */
    int i;
    int n;
    unsigned long total_delay = 0;
    unsigned long delay = 0;
    if (g_nServoDrvOpen == 1)
    {
        g_tlServoDrvPwm.expires = jiffies + usecs_to_jiffies(SERVODRIVE_PERIOD);
        add_timer(&g_tlServoDrvPwm);
    }

    /* freeze sorted list if changed since last run */
    if (dirty) {
      for (i = 0; i < GPIO_TOTAL_COUNT; ++i) {
        sorted_by_pulse_time_frozen[i] = sorted_by_pulse_time[i];
        pulse_time_usec_frozen[i] = pulse_time_usec[i];
      }
      dirty = 0;
    }

    /* skip all pins not activated */
    for (n = 0; n < GPIO_TOTAL_COUNT && (pulse_time_usec_frozen[sorted_by_pulse_time_frozen[n] - FIRST_GPIO_PIN] == 0); ++n) {
    }

    /* start all active pulses with high output */
    for (i = n; i < GPIO_TOTAL_COUNT; ++i) {
      gpio_set_value(pin_gpio[sorted_by_pulse_time_frozen[i] - FIRST_GPIO_PIN], 1);
    }

    /* reset pulses in order */
    total_delay = 75; /* measured initial delay */
    for (i = n; i < GPIO_TOTAL_COUNT; ++i) {
      delay = pulse_time_usec_frozen[sorted_by_pulse_time_frozen[i] - FIRST_GPIO_PIN] - total_delay;
      udelay(delay);
      total_delay += delay;
      gpio_set_value(pin_gpio[sorted_by_pulse_time_frozen[i] - FIRST_GPIO_PIN], 0);
    }
    ++pulse_count;
}

static void servodrive_exit(void) {
    int i;
    g_nServoDrvOpen = 0;
    del_timer_sync(&g_tlServoDrvPwm);
    for (i = 0; i < GPIO_TOTAL_COUNT; i++) {
      if (pulse_time_usec[i]) {
        gpio_free(pin_gpio[i]);
      }
    }
    printk("<1>Exiting servodrive\n");
    unregister_chrdev(g_nServoDrvMajor, SERVODRIVE_NAME);
    device_destroy(servodrive_class, MKDEV(g_nServoDrvMajor, id));
}

module_init(servodrive_init);
module_exit(servodrive_exit);
