#define DEBUG
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/kfifo.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <linux/interrupt.h>
#include "uart_driver.h"


static struct kset *uart_dev_kset = NULL;


/* Custom attribute structure for UART_DEV_T */
struct uart_attr
{
	struct attribute attr;
	ssize_t (*show)(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf);
	ssize_t (*store)(UART_DEV_T *pdev, struct uart_attr *pattr, const char *buf, size_t count);
};


/* store() method implementation for the reset attribute */
static ssize_t reset_store(UART_DEV_T *pdev, struct uart_attr *pattr, const char *buf, size_t count)
{
	int i = 0;
	
        dump_stack();
	sscanf(buf, "%d", &i);

	if (i == 1)
		reset_dev(pdev); /* Reset the device */

        return count;
}


/* show() method implementation for the ntxbytes attribute */
static ssize_t ntxbytes_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of data bytes available for transmission in the device */
	return sprintf(buf, "%d\n", kfifo_len(&pdev->tx_fifo));
}


/* show() method implementation for the nrxbytes attribute */
static ssize_t nrxbytes_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of data bytes available for reading in the device */
	return sprintf(buf, "%d\n", kfifo_len(&pdev->rx_fifo));
}


/* show() method implementation for the ndrbytes attribute */
static ssize_t ndrbytes_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of bytes dropped for the device */
	return sprintf(buf, "%lu\n", pdev->dbytes);
}


/* show() method implementation for the ovrerrs attribute */
static ssize_t novrerrs_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of overrun errors for the device */
	return sprintf(buf, "%lu\n", pdev->ovr_err);
}


/* show() method implementation for the parerrs attribute */
static ssize_t nparerrs_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of parity errors for the device */
	return sprintf(buf, "%lu\n", pdev->par_err);
}


/* show() method implementation for the frmerrs attribute */
static ssize_t nfrmerrs_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of framing errors for the device */
	return sprintf(buf, "%lu\n", pdev->frm_err);
}


/* show() method implementation for the brkerrs attribute */
static ssize_t nbrkerrs_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the number of break interrupt errors for the device */
	return sprintf(buf, "%lu\n", pdev->brk_err);
}


/* show() method implementation for the devid attribute */
static ssize_t devid_show(UART_DEV_T *pdev, struct uart_attr *pattr, char *buf)
{
        dump_stack();

	/* Print the major and minor number for the device */
	return sprintf(buf, "%d,%d\n", MAJOR(pdev->devid), MINOR(pdev->devid));
}


/* Attributes for the pseudo character device */
static struct uart_attr reset_attr = __ATTR(reset, 0222, NULL, reset_store);
static struct uart_attr ntxbytes_attr = __ATTR(ntxbytes, 0444, ntxbytes_show, NULL);
static struct uart_attr nrxbytes_attr = __ATTR(nrxbytes, 0444, nrxbytes_show, NULL);
static struct uart_attr ndrbytes_attr = __ATTR(ndrbytes, 0444, ndrbytes_show, NULL);
static struct uart_attr novrerrs_attr = __ATTR(novrerrs, 0444, novrerrs_show, NULL);
static struct uart_attr nparerrs_attr = __ATTR(nparerrs, 0444, nparerrs_show, NULL);
static struct uart_attr nfrmerrs_attr = __ATTR(nfrmerrs, 0444, nfrmerrs_show, NULL);
static struct uart_attr nbrkerrs_attr = __ATTR(nbrkerrs, 0444, nbrkerrs_show, NULL);
static struct uart_attr devid_attr = __ATTR(devid, 0444, devid_show, NULL);


/* Default show() function to be passed to sysfs */
static ssize_t uart_attr_show(struct kobject *kobj, struct attribute *pattr, char *buf)
{
	struct uart_attr *uart_attr;
	UART_DEV_T *pdev;

	dump_stack();

	/* Retrieve pointers to the attribute and device objects and pass them to the corresponding show() function */
	uart_attr = container_of(pattr, struct uart_attr, attr);
	pdev = container_of(kobj, UART_DEV_T, kobj);

	if (!uart_attr->show)
		return -EIO;

	return uart_attr->show(pdev, uart_attr, buf);
}


/* Default store() function to be passed to sysfs */
static ssize_t uart_attr_store(struct kobject *kobj, struct attribute *pattr, const char *buf, size_t len)
{
	struct uart_attr *uart_attr;
	UART_DEV_T *pdev;

	dump_stack();

	/* Retrieve pointers to the attribute and device objects and pass them to the corresponding store() function */
	uart_attr = container_of(pattr, struct uart_attr, attr);
	pdev = container_of(kobj, UART_DEV_T, kobj);

	if (!uart_attr->store)
		return -EIO;

	return uart_attr->store(pdev, uart_attr, buf, len);
}


/* release() method implementation for the kobject */
static void uart_release(struct kobject *kobj)
{
}


/* Default methods for kobject attributes */
static struct sysfs_ops uart_sysfs_ops = {
	.show = uart_attr_show,
	.store = uart_attr_store,
};


/* Set of default attributes for the kobject */
static struct attribute *uart_def_attrs[] = {
	&reset_attr.attr,
	&ntxbytes_attr.attr,
	&nrxbytes_attr.attr,
	&ndrbytes_attr.attr,
	&novrerrs_attr.attr,
	&nparerrs_attr.attr,
	&nfrmerrs_attr.attr,
	&nbrkerrs_attr.attr,
	&devid_attr.attr,
	NULL,
};


/* ktype for the kobject */
static struct kobj_type uart_ktype = {
	.sysfs_ops = &uart_sysfs_ops,
	.release = uart_release,
	.default_attrs = uart_def_attrs,
};


/* Initialize the kobject instance */
int init_uart_kobj(struct kobject *kobj, const int n)
{
	int ret;

	dump_stack();

	kobj->kset = uart_dev_kset;

	if ((ret =  kobject_init_and_add(kobj, &uart_ktype, NULL, "uart_device%d", n)) == 0)
		kobject_uevent(kobj, KOBJ_ADD);

	return ret;
}


/* Decrement the kobject instance reference count */
void destroy_uart_kobj(struct kobject *kobj)
{
	kobject_put(kobj);
}


/* Function to create a kset for the character device and add it to sysfs */
int create_uart_kset(void)
{
	dump_stack();

	/* Create and add the kset to /sys/kernel */
	if ((uart_dev_kset = kset_create_and_add("uart_devs", NULL, kernel_kobj)) == NULL)
		return -ENOMEM;

	return 0;
}


/* Remove the kset from sysfs */
void destroy_uart_kset(void)
{
	kset_unregister(uart_dev_kset);
}
