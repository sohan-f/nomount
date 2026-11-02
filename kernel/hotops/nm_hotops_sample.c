#include <linux/module.h>
#include "../src/hotops.h"

static atomic_t sample_hits = ATOMIC_INIT(0);
static bool sample_registered;

static struct dentry *sample_materialize(struct inode *dir, struct dentry *dentry,
                                         struct nm_rule_info *rule_info)
{
    atomic_inc(&sample_hits);
    return nm_base_materialize_rule(dir, dentry, rule_info);
}

static struct nm_hotops nm_hotops_sample = {
    .magic = NM_HOTOPS_MAGIC,
    .abi_rev = NM_HOTOPS_ABI_REV,
    .owner = THIS_MODULE,
    .resolve_rule_from_leaf = nm_base_resolve_rule_from_leaf,
    .rebind_path = nm_base_rebind_path,
    .materialize_rule = sample_materialize,
    .d_revalidate = nm_base_d_revalidate_common,
};

static void sample_register(struct work_struct *work)
{
    sample_registered = nm_hotops_register(&nm_hotops_sample) == 0;
}
static DECLARE_WORK(sample_work, sample_register);

static int __init nm_hotops_sample_init(void)
{
    schedule_work(&sample_work);
    return 0;
}

static void __exit nm_hotops_sample_exit(void)
{
    flush_work(&sample_work);
    if (sample_registered)
        nm_hotops_unregister(&nm_hotops_sample);
    printk(KERN_INFO "NoMount hotops sample: %u dispatches\n", atomic_read(&sample_hits));
}

module_init(nm_hotops_sample_init);
module_exit(nm_hotops_sample_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NoMount hotops overlay ABI sample");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS("VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver");
MODULE_IMPORT_NS("ANDROID_GKI_VFS_EXPORT_ONLY");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
MODULE_IMPORT_NS(ANDROID_GKI_VFS_EXPORT_ONLY);
#endif
