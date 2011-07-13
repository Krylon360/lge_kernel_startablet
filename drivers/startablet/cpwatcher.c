/*
 * drivers/startablet/cpwatcher.c
 *
 * Star Tablet CP watcher driver
 *
 * Copyright (c) 2011, LG Electronics Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/types.h>

#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/semaphore.h>

static struct tty_driver *cpwatcher_driver;

struct watcher_struct {
    struct tty_struct	*tty;
};
static struct watcher_struct* watcher;

int open_count;
static DECLARE_MUTEX(watcher_sema);

static int cwc_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
    return 0;
}

static int cwc_write_room(struct tty_struct *tty)
{
    return 0;
}

static int cwc_open(struct tty_struct *tty, struct file *filp)
{
    printk(KERN_INFO "%s", __FUNCTION__);

    tty->driver_data = NULL;
    if(tty->index > 0) return -ENODEV;

    if (watcher == NULL) {
        watcher = kmalloc(sizeof(struct watcher_struct), GFP_KERNEL);
        if (!watcher) return -ENOMEM;
    }
    down(&watcher_sema);
	open_count++;

    /* save our structure within the tty structure */
    tty->driver_data = watcher;
    watcher->tty = tty;

    printk(KERN_INFO "%s returns", __FUNCTION__);
	return 0;

}

static void cwc_close(struct tty_struct *tty, struct file *filp)
{
    printk(KERN_INFO "%s", __FUNCTION__);

    if((watcher == NULL) || (tty == NULL) || (filp == NULL)) return;

    printk(KERN_INFO "%s 1", __FUNCTION__);
    if(tty->driver_data != watcher) return;

    printk(KERN_INFO "%s 2", __FUNCTION__);

    open_count--;
	watcher->tty = NULL;
	up(&watcher_sema);

    printk(KERN_INFO "%s returns", __FUNCTION__);
    return;
}

void cwc_notify_reset(void)
{
    const char* cpresetted = "+RESTART\r\n";
    int data_size = strlen(cpresetted);

    printk(KERN_INFO "%s", __FUNCTION__);

	if(watcher &&(watcher->tty) && (watcher->tty->ldisc)) {
	    watcher->tty->ldisc->ops->receive_buf(watcher->tty, cpresetted, NULL, data_size);
        tty_flip_buffer_push(watcher->tty);
	}
}
EXPORT_SYMBOL_GPL(cwc_notify_reset);

struct tty_operations cpwatcher_ops = {
     .open = cwc_open,
     .close = cwc_close,
     .write = cwc_write,
     .write_room = cwc_write_room,
};

static int __init cwc_init(void)
{
    int result;

    printk(KERN_INFO "%s", __FUNCTION__);

    cpwatcher_driver = alloc_tty_driver(1);
    if (!cpwatcher_driver)
        return -ENOMEM;

    cpwatcher_driver->owner = THIS_MODULE;
    cpwatcher_driver->driver_name = "cpwatcher";

    cpwatcher_driver->name = "cwc";
    cpwatcher_driver->major = 154;
    cpwatcher_driver->minor_start = 0;
    cpwatcher_driver->type = TTY_DRIVER_TYPE_SERIAL;
    cpwatcher_driver->subtype = SERIAL_TYPE_NORMAL;
    cpwatcher_driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;

    cpwatcher_driver->init_termios = tty_std_termios;
    cpwatcher_driver->init_termios.c_cflag = B38400 | CS8 | CREAD | HUPCL | CLOCAL;

    tty_set_operations(cpwatcher_driver, &cpwatcher_ops);

    result = tty_register_driver(cpwatcher_driver);
    if (result) {
        printk(KERN_ERR "failed to register cp watcher driver");
        put_tty_driver(cpwatcher_driver);
        return result;
    }

    tty_register_device(cpwatcher_driver, 0, NULL);

    return 0;
}

static void __exit cwc_exit(void)
{
    printk(KERN_INFO "%s", __FUNCTION__);

    tty_unregister_device(cpwatcher_driver, 0);
    tty_unregister_driver(cpwatcher_driver);

}

module_init(cwc_init);
module_exit(cwc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CP Watcher");
