#ifndef _LINUX_NOMOUNT_HOTOPS_H
#define _LINUX_NOMOUNT_HOTOPS_H

#include <linux/module.h>
#include <linux/rcupdate.h>
#include "nomount.h"

#define NM_HOTOPS_MAGIC 0x6E6D686FU /* "nmho" */
#define NM_HOTOPS_ABI_REV 1

struct nm_hotops {
	u32 magic;
	u32 abi_rev;
	struct module *owner;
	int (*resolve_rule_from_leaf)(struct nm_leaf *leaf, uid_t fsuid,
		struct nm_rule_info *out);
	int (*rebind_path)(struct nm_rule_info *rule_info);
	struct dentry *(*materialize_rule)(struct inode *dir, struct dentry *dentry,
		struct nm_rule_info *rule_info);
	int (*d_revalidate)(struct inode *parent_inode, const struct qstr *name,
		struct dentry *dentry, unsigned int flags, bool weak);
};

struct nm_hotops_ctl {
	const struct nm_hotops __rcu *ptr;
	const struct nm_hotops *default_ops;
};
extern struct nm_hotops_ctl nm_hotops_ctl;

int nm_base_resolve_rule_from_leaf(struct nm_leaf *leaf, uid_t fsuid, struct nm_rule_info *out);
int nm_base_rebind_path(struct nm_rule_info *rule_info);
struct dentry *nm_base_materialize_rule(struct inode *dir, struct dentry *dentry,
	struct nm_rule_info *rule_info);
int nm_base_d_revalidate_common(struct inode *parent_inode, const struct qstr *name,
	struct dentry *dentry, unsigned int flags, bool weak);

struct nm_rule *nm_select_rule(struct nm_leaf *leaf, unsigned int uid);
struct nm_leaf *nm_find_leaf_in_dir(struct nm_dir_node *dir_node, const char *name, size_t len);
bool nm_is_our_dops(struct nm_dir_ops *iop, const struct dentry_operations *dop);

int nm_hotops_rebind_path(struct nm_rule_info *rule_info);
int nm_hotops_register(struct nm_hotops *ops);
void nm_hotops_unregister(const struct nm_hotops *ops);

static inline void nm_hotops_release(const struct nm_hotops *ops, bool pinned)
{
	if (pinned)
		module_put(ops->owner);
}

static inline const struct nm_hotops *nm_hotops_current(bool *pinned)
{
	const struct nm_hotops *ops;

	rcu_read_lock();
	ops = rcu_dereference(nm_hotops_ctl.ptr);
	*pinned = false;
	if (likely(ops == nm_hotops_ctl.default_ops)) {
		rcu_read_unlock();
		return ops;
	}
	if (ops && try_module_get(ops->owner)) {
		*pinned = true;
	} else {
		ops = nm_hotops_ctl.default_ops;
	}
	rcu_read_unlock();
	return ops;
}

#endif
