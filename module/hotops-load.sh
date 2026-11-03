#!/system/bin/sh
# Load or swap the optional lkm/hotops.ko overlay onto the running base module.
# No file = base table only. Kernel-side guards (magic/abi_rev/EBUSY) keep a
# bad overlay inert; the base keeps serving dispatches through its own table.
# Re-run at any time to swap in a freshly staged lkm/hotops.ko without reboot.

MODDIR=${0%/*}
. "$MODDIR/common.sh"

HOTOPS_KO="$MODDIR/lkm/hotops.ko"

if ! "$LOADER" version >/dev/null 2>&1; then
    klogwarn "hotops-load: base module not live, overlay untouched"
    exit 1
fi

[ -f "$HOTOPS_KO" ] || exit 0

# One overlay at a time. Family contract: overlay modules are named nm_hotops_*.
# rmmod waits out in-flight slot calls (module refcount), so an old registration
# drains here before the new file is insmod'ed.
for m in $(cut -d' ' -f1 /proc/modules | grep '^nm_hotops_'); do
    if ! rmmod "$m" 2>/dev/null; then
        klogerr "hotops-load: rmmod $m refused; keeping the active overlay"
        exit 1
    fi
done

# Loader precedence mirrors load_ko in common.sh: the script's SELinux domain
# is often not allowed to load modules, so try the root daemon's insmod first.
if command -v ksud >/dev/null 2>&1 && ksud -h 2>&1 | grep -qE '(^|[[:space:]])insmod([[:space:]]|$)'; then
    ksud insmod "$HOTOPS_KO" >/dev/null 2>&1 && klog "hotops overlay loaded" && exit 0
elif command -v apd >/dev/null 2>&1 && apd -h 2>&1 | grep -qE '(^|[[:space:]])insmod([[:space:]]|$)'; then
    apd insmod "$HOTOPS_KO" >/dev/null 2>&1 && klog "hotops overlay loaded" && exit 0
fi
if [ -x "$MODDIR/lkm/lkmloader" ] && "$MODDIR/lkm/lkmloader" "$HOTOPS_KO" >/dev/null 2>&1; then
    klog "hotops overlay loaded"
else
    klogwarn "hotops overlay load failed; base table stays active"
fi
