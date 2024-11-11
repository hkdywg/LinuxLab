#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

struct mydev_priv {
	char name[64];
	int i;
};

static int create_oops(struct vm_area_struct *vma, struct mydev_priv *priv)
{
	unsigned long flags;

	flags = vma->vm_flags;
	printk("flags = 0x%lx, name = %s\n", flags, priv->name);

	return 0;
}


int __init my_oops_init(void)
{
	int ret;
	struct vm_area_struct *vma  = NULL;
	struct mydev_priv priv;

	vma = kmalloc(sizeof(*vma), GFP_KERNEL);
	if(!vma)
		return -ENOMEM;

	kfree(vma);
	vma = NULL;

	smp_mb();

	memcpy(priv.name, "test_core_dump", sizeof("test_core_dump"));
	priv.i = 20;

	ret = create_oops(vma, &priv);

	return 0;
}


void __exit my_oops_exit(void)
{
	printk("exit test core dump\n");
}


module_init(my_oops_init);
module_exit(my_oops_exit);
MODULE_LICENSE("GPL");
