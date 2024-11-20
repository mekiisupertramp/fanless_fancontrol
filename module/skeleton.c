/* skeleton.c */
#include <linux/module.h>			/* needed by all modules */
#include <linux/init.h>				/* needed for macros */
#include <linux/kernel.h>			/* needed for debugging */
#include <linux/device.h>
#include <linux/platform_device.h>	
#include <linux/thermal.h>			/* needed to read CPU temperature */
#include <linux/gpio.h>				/* needed fto access GPIOs */
#include <linux/timer.h>			/* needed to use timers */

#define GPIO_LED_STATUS 10
#define MANUAL_MS_LED_INCREMENT 200
#define MINI_MANUAL_MS_LED_INCREMENT 50
#define TIME_TO_CHECK_TEMP 1000

static char sysfs_buf[100];
uint32_t temp;
uint32_t ms_led;
bool led_on;
bool mode_auto;

static struct timer_list timer_status_led;
static struct timer_list timer_read_temp;

/*
* Convert 
*/
uint32_t temp_to_ms(uint32_t temp) {
	if (temp < 35000)
		return 500;
	else if (temp < 40000)
		return 200;
	else if (temp < 45000)
		return 100;
	else
		return 50;
}

/*
* Switch the state of the status led when the time is over
*/ 
void timer_status_led_callback(struct timer_list  *timer) {
	gpio_set_value(GPIO_LED_STATUS, !led_on);
	led_on = !led_on;
	mod_timer(&timer_status_led, jiffies + msecs_to_jiffies(ms_led));
}

/*
* Read the CPU temparature every TIME_TO_CHECK_TEMP ms
* if the current mode is auto
*/ 
void timer_read_temp_callback(struct timer_list  *timer) {
	if (mode_auto) {
		thermal_zone_get_temp(thermal_zone_get_zone_by_name("cpu_thermal"), &temp);
		ms_led = temp_to_ms(temp);
	}
	mod_timer(&timer_read_temp, jiffies + msecs_to_jiffies(TIME_TO_CHECK_TEMP));
}

/*
* Show if auto or manual mode is currently used: 1=auto, 0=manual
*/ 
static ssize_t skeleton_show_mode (struct device* dev, struct device_attribute *attr, char *buf) {
    strcpy (buf, sysfs_buf);
    return strlen (sysfs_buf);
}

/*
* Store mode: 1=auto, 0=manual
*/ 
static ssize_t skeleton_store_mode (struct device* dev, struct device_attribute *attr, const char *buf, size_t count) {
	int len = sizeof(sysfs_buf) - 1;
    if (len > count) len = count;
    strncpy (sysfs_buf, buf, len);
    sysfs_buf[len] = 0;

	len = strlen(sysfs_buf);
	if(sysfs_buf[len-1] == '\n')
		sysfs_buf[len-1] = '\0';
	if (!strcmp(sysfs_buf, "1")) {
		mode_auto = true;
	} else {
		mode_auto = false;
		ms_led = 1000;
	}
	
	pr_info("mode auto: %d\n", mode_auto);
    return len;
}

/*
* Show last manual operation on the led period: 1 = longer period, 0 = shorter period
*/ 
static ssize_t skeleton_show_op (struct device* dev, struct device_attribute *attr, char *buf) {
    strcpy (buf, sysfs_buf);
    return strlen (sysfs_buf);
}

/*
* Store manual operation on the led period: 1 = longer period, 0 = shorter period
* Effective only if the manual mode is active
*/ 
static ssize_t skeleton_store_op (struct device* dev, struct device_attribute *attr, const char *buf, size_t count) {
	int len = sizeof(sysfs_buf) - 1;
    if (len > count) len = count;
    strncpy (sysfs_buf, buf, len);
    sysfs_buf[len] = 0;

	if (!mode_auto) {
		len = strlen(sysfs_buf);
		if(sysfs_buf[len-1] == '\n')
			sysfs_buf[len-1] = '\0';
		if (strcmp(sysfs_buf, "1")) {
			ms_led += MANUAL_MS_LED_INCREMENT;
			pr_info("led frequency --\n");
		} else {
			if (ms_led > MANUAL_MS_LED_INCREMENT) {
				ms_led -= MANUAL_MS_LED_INCREMENT;
				pr_info("led frequency ++\n");
			} else if (ms_led == MANUAL_MS_LED_INCREMENT || ms_led > MINI_MANUAL_MS_LED_INCREMENT) {
				ms_led -= MINI_MANUAL_MS_LED_INCREMENT;
				pr_info("led frequency +\n");
			} else if (ms_led == MINI_MANUAL_MS_LED_INCREMENT) {
				pr_info("max led frequency\n");
			}
		}
	}
	
    return len;
}

/*
* Show the current led period
*/ 
static ssize_t skeleton_show_period (struct device* dev, struct device_attribute *attr, char *buf) {
    //strcpy (buf, ms_led);
	snprintf(buf, 100, "%d", ms_led);
    return strlen (buf);
}

/*
* Store the current led period
*/ 
static ssize_t skeleton_store_period (struct device* dev, struct device_attribute *attr, const char *buf, size_t count) {
	pr_info("period not writeable\n");
	return 0;
}

DEVICE_ATTR (mode, 0664, skeleton_show_mode, skeleton_store_mode);
DEVICE_ATTR (op, 0664, skeleton_show_op, skeleton_store_op);
DEVICE_ATTR (period, 0664, skeleton_show_period, skeleton_store_period);

// sysfs handling's methods & structures
static void sysfs_dev_release (struct device *dev) {
    
}

static struct platform_driver sysfs_driver = {
    .driver = {.name = "skeleton",},
};

static struct platform_device sysfs_device = {
    .name = "skeleton",
    .id = -1,
    .dev.release = sysfs_dev_release,
};

static int __init skeleton_init(void) {
    // setup sysfs
    int status = 0;
    if (status == 0)
        status = platform_driver_register (&sysfs_driver);
    if (status == 0)
        status = platform_device_register (&sysfs_device);
    if (status == 0) {
        device_create_file (&sysfs_device.dev, &dev_attr_mode);
		device_create_file (&sysfs_device.dev, &dev_attr_op);
		device_create_file (&sysfs_device.dev, &dev_attr_period);
	}

	pr_info("Linux module skeleton loaded\n");

	// start in automatic mode by default
	mode_auto = true;

	// get GPIOs
	gpio_request(GPIO_LED_STATUS, "status_led");
	gpio_direction_output(GPIO_LED_STATUS, 1);

	gpio_set_value(GPIO_LED_STATUS, 0);
	led_on = gpio_get_value(GPIO_LED_STATUS);

	// initial temperature reading and led period setup
	thermal_zone_get_temp(thermal_zone_get_zone_by_name("cpu_thermal"), &temp);
	pr_info("initial cpu_thermal temp: %d", temp);
	ms_led = temp_to_ms(temp);
	
	// setup and start timers
	timer_setup(&timer_status_led, timer_status_led_callback, 0);
	timer_setup(&timer_read_temp, timer_read_temp_callback, 0);
	mod_timer(&timer_status_led, jiffies + msecs_to_jiffies(ms_led));
	mod_timer(&timer_read_temp, jiffies + msecs_to_jiffies(TIME_TO_CHECK_TEMP));

    return status;
}

static void __exit skeleton_exit(void) {
	// free GPIOs
	gpio_set_value(GPIO_LED_STATUS, 0);
	gpio_free(10);

	// delete timers
	del_timer(&timer_status_led);
	del_timer(&timer_read_temp);
    
    // clean sysfs
    device_remove_file (&sysfs_device.dev, &dev_attr_mode);
	device_remove_file (&sysfs_device.dev, &dev_attr_op);
	device_remove_file (&sysfs_device.dev, &dev_attr_period);
    platform_device_unregister (&sysfs_device);
    platform_driver_unregister (&sysfs_driver);
    
    pr_info("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Mehmed Blazevic <mehmed.blazevic@master.hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");