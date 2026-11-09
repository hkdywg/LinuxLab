##
## benchmark/stress-ng/make.inc
##
## History:
##    2024/04/16 - [yinwg] Created file
##
## Copyright (C) 2016, JiangSu New Vision Automotive CO.,LTD.
## 
## All rights reserved. This software is New Vision property. Duplication
## or disclosure without New Vision written authorization is prohibited.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= config.h
STRESS_PATH     := $(LOCAL_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): 
	cd $(STRESS_PATH) && make CC="$(CC)" CXX="$(CXX)" STATIC=1 -f Makefile.config && \
    $(CC) core-io-uring.c  | grep IORING_OP | sed 's/,//' | \
        sed 's/.*\(IORING_OP_.*\)/#define HAVE_\1/' > io-uring.h && \
    $(CC) core-personality.c | grep -e "PER_[A-Z0-9]* =.*," | cut -d "=" -f 1 | sed "s/.$$/,/" > personality.h && \
    echo "#define STRESS_GIT_COMMIT_ID \"\"" > git-commit-id.h && \
    echo "const char stress_config[] = " > core-config.c && sed 's/.*/"&\\n"/' config.h >> core-config.c && echo ";" >> core-config.c && \
    $(CC) -E core-perf-event.c | grep "PERF_COUNT" | sed 's/,/ /' | sed s/'^ *//' | awk {'print "#define STRESS_" $$1 " (1)"'} > core-perf-event.h
	@echo "Generate stress-ng $@ Done."


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= stress-ng
LOCAL_SRCS      := $(LOCAL_PATH)/core-affinity.c \
	$(LOCAL_PATH)/core-asm-ret.c \
	$(LOCAL_PATH)/core-cpu.c \
	$(LOCAL_PATH)/core-cpu-cache.c \
	$(LOCAL_PATH)/core-cpuidle.c \
	$(LOCAL_PATH)/core-clocksource.c \
	$(LOCAL_PATH)/core-config-check.c \
	$(LOCAL_PATH)/core-hash.c \
	$(LOCAL_PATH)/core-helper.c \
	$(LOCAL_PATH)/core-ignite-cpu.c \
	$(LOCAL_PATH)/core-interrupts.c \
	$(LOCAL_PATH)/core-io-uring.c \
	$(LOCAL_PATH)/core-io-priority.c \
	$(LOCAL_PATH)/core-job.c \
	$(LOCAL_PATH)/core-killpid.c \
	$(LOCAL_PATH)/core-klog.c \
	$(LOCAL_PATH)/core-limit.c \
	$(LOCAL_PATH)/core-lock.c \
	$(LOCAL_PATH)/core-log.c \
	$(LOCAL_PATH)/core-madvise.c \
	$(LOCAL_PATH)/core-mincore.c \
	$(LOCAL_PATH)/core-mlock.c \
	$(LOCAL_PATH)/core-mmap.c \
	$(LOCAL_PATH)/core-module.c \
	$(LOCAL_PATH)/core-mounts.c \
	$(LOCAL_PATH)/core-mwc.c \
	$(LOCAL_PATH)/core-net.c \
	$(LOCAL_PATH)/core-numa.c \
	$(LOCAL_PATH)/core-opts.c \
	$(LOCAL_PATH)/core-out-of-memory.c \
	$(LOCAL_PATH)/core-parse-opts.c \
	$(LOCAL_PATH)/core-perf.c \
	$(LOCAL_PATH)/core-processes.c \
	$(LOCAL_PATH)/core-resources.c \
	$(LOCAL_PATH)/core-sched.c \
	$(LOCAL_PATH)/core-setting.c \
	$(LOCAL_PATH)/core-shared-heap.c \
	$(LOCAL_PATH)/core-shim.c \
	$(LOCAL_PATH)/core-smart.c \
	$(LOCAL_PATH)/core-sort.c \
	$(LOCAL_PATH)/core-thermal-zone.c \
	$(LOCAL_PATH)/core-time.c \
	$(LOCAL_PATH)/core-thrash.c \
	$(LOCAL_PATH)/core-ftrace.c \
	$(LOCAL_PATH)/core-try-open.c \
	$(LOCAL_PATH)/core-vmstat.c \
	$(LOCAL_PATH)/core-config.c \
	$(LOCAL_PATH)/stress-ng.c \
    $(LOCAL_PATH)/stress-access.c \
	$(LOCAL_PATH)/stress-acl.c \
	$(LOCAL_PATH)/stress-affinity.c \
	$(LOCAL_PATH)/stress-af-alg.c \
	$(LOCAL_PATH)/stress-aio.c \
	$(LOCAL_PATH)/stress-aiol.c \
	$(LOCAL_PATH)/stress-alarm.c \
	$(LOCAL_PATH)/stress-apparmor.c \
	$(LOCAL_PATH)/stress-atomic.c \
	$(LOCAL_PATH)/stress-bad-altstack.c \
	$(LOCAL_PATH)/stress-bad-ioctl.c \
	$(LOCAL_PATH)/stress-bigheap.c \
	$(LOCAL_PATH)/stress-bind-mount.c \
	$(LOCAL_PATH)/stress-binderfs.c \
	$(LOCAL_PATH)/stress-bitonicsort.c \
	$(LOCAL_PATH)/stress-branch.c \
	$(LOCAL_PATH)/stress-brk.c \
	$(LOCAL_PATH)/stress-bsearch.c \
	$(LOCAL_PATH)/stress-cache.c \
	$(LOCAL_PATH)/stress-cacheline.c \
	$(LOCAL_PATH)/stress-cap.c \
	$(LOCAL_PATH)/stress-cgroup.c \
	$(LOCAL_PATH)/stress-chattr.c \
	$(LOCAL_PATH)/stress-chdir.c \
	$(LOCAL_PATH)/stress-chmod.c \
	$(LOCAL_PATH)/stress-chown.c \
	$(LOCAL_PATH)/stress-chroot.c \
	$(LOCAL_PATH)/stress-clock.c \
	$(LOCAL_PATH)/stress-clone.c \
	$(LOCAL_PATH)/stress-close.c \
	$(LOCAL_PATH)/stress-context.c \
	$(LOCAL_PATH)/stress-copy-file.c \
	$(LOCAL_PATH)/stress-cpu.c \
	$(LOCAL_PATH)/stress-cpu-online.c \
	$(LOCAL_PATH)/stress-crypt.c \
	$(LOCAL_PATH)/stress-cyclic.c \
	$(LOCAL_PATH)/stress-daemon.c \
	$(LOCAL_PATH)/stress-dccp.c \
	$(LOCAL_PATH)/stress-dekker.c \
	$(LOCAL_PATH)/stress-dentry.c \
	$(LOCAL_PATH)/stress-dev.c \
	$(LOCAL_PATH)/stress-dev-shm.c \
	$(LOCAL_PATH)/stress-dir.c \
	$(LOCAL_PATH)/stress-dirdeep.c \
	$(LOCAL_PATH)/stress-dirmany.c \
	$(LOCAL_PATH)/stress-dnotify.c \
	$(LOCAL_PATH)/stress-dup.c \
	$(LOCAL_PATH)/stress-dynlib.c \
	$(LOCAL_PATH)/stress-eigen.c \
	$(LOCAL_PATH)/stress-efivar.c \
	$(LOCAL_PATH)/stress-enosys.c \
	$(LOCAL_PATH)/stress-env.c \
	$(LOCAL_PATH)/stress-epoll.c \
	$(LOCAL_PATH)/stress-eventfd.c \
	$(LOCAL_PATH)/stress-exec.c \
	$(LOCAL_PATH)/stress-exit-group.c \
	$(LOCAL_PATH)/stress-expmath.c \
	$(LOCAL_PATH)/stress-factor.c \
	$(LOCAL_PATH)/stress-fallocate.c \
	$(LOCAL_PATH)/stress-fanotify.c \
	$(LOCAL_PATH)/stress-far-branch.c \
	$(LOCAL_PATH)/stress-fault.c \
	$(LOCAL_PATH)/stress-fcntl.c \
	$(LOCAL_PATH)/stress-fd-fork.c \
	$(LOCAL_PATH)/stress-file-ioctl.c \
	$(LOCAL_PATH)/stress-fiemap.c \
	$(LOCAL_PATH)/stress-fifo.c \
	$(LOCAL_PATH)/stress-filename.c \
	$(LOCAL_PATH)/stress-flock.c \
	$(LOCAL_PATH)/stress-flushcache.c \
	$(LOCAL_PATH)/stress-fma.c \
	$(LOCAL_PATH)/stress-fork.c \
	$(LOCAL_PATH)/stress-forkheavy.c \
	$(LOCAL_PATH)/stress-fp.c \
	$(LOCAL_PATH)/stress-fp-error.c \
	$(LOCAL_PATH)/stress-fpunch.c \
	$(LOCAL_PATH)/stress-fsize.c \
	$(LOCAL_PATH)/stress-fstat.c \
	$(LOCAL_PATH)/stress-full.c \
	$(LOCAL_PATH)/stress-funccall.c \
	$(LOCAL_PATH)/stress-funcret.c \
	$(LOCAL_PATH)/stress-futex.c \
	$(LOCAL_PATH)/stress-get.c \
	$(LOCAL_PATH)/stress-getrandom.c \
	$(LOCAL_PATH)/stress-getdent.c \
	$(LOCAL_PATH)/stress-goto.c \
	$(LOCAL_PATH)/stress-gpu.c \
	$(LOCAL_PATH)/stress-handle.c \
	$(LOCAL_PATH)/stress-hash.c \
	$(LOCAL_PATH)/stress-hdd.c \
	$(LOCAL_PATH)/stress-heapsort.c \
	$(LOCAL_PATH)/stress-hrtimers.c \
	$(LOCAL_PATH)/stress-hsearch.c \
	$(LOCAL_PATH)/stress-icache.c \
	$(LOCAL_PATH)/stress-icmp-flood.c \
	$(LOCAL_PATH)/stress-idle-page.c \
	$(LOCAL_PATH)/stress-inode-flags.c \
	$(LOCAL_PATH)/stress-inotify.c \
	$(LOCAL_PATH)/stress-io.c \
	$(LOCAL_PATH)/stress-iomix.c \
	$(LOCAL_PATH)/stress-ioport.c \
	$(LOCAL_PATH)/stress-ioprio.c \
	$(LOCAL_PATH)/stress-io-uring.c \
	$(LOCAL_PATH)/stress-ipsec-mb.c \
	$(LOCAL_PATH)/stress-itimer.c \
	$(LOCAL_PATH)/stress-jpeg.c \
	$(LOCAL_PATH)/stress-judy.c \
	$(LOCAL_PATH)/stress-kcmp.c \
	$(LOCAL_PATH)/stress-key.c \
	$(LOCAL_PATH)/stress-kill.c \
	$(LOCAL_PATH)/stress-klog.c \
	$(LOCAL_PATH)/stress-kvm.c \
	$(LOCAL_PATH)/stress-l1cache.c \
	$(LOCAL_PATH)/stress-landlock.c \
	$(LOCAL_PATH)/stress-lease.c \
	$(LOCAL_PATH)/stress-led.c \
	$(LOCAL_PATH)/stress-link.c \
	$(LOCAL_PATH)/stress-list.c \
	$(LOCAL_PATH)/stress-llc-affinity.c \
	$(LOCAL_PATH)/stress-loadavg.c \
	$(LOCAL_PATH)/stress-lockbus.c \
	$(LOCAL_PATH)/stress-locka.c \
	$(LOCAL_PATH)/stress-lockf.c \
	$(LOCAL_PATH)/stress-lockofd.c \
	$(LOCAL_PATH)/stress-logmath.c \
	$(LOCAL_PATH)/stress-longjmp.c \
	$(LOCAL_PATH)/stress-loop.c \
	$(LOCAL_PATH)/stress-lsearch.c \
	$(LOCAL_PATH)/stress-lsm.c \
	$(LOCAL_PATH)/stress-madvise.c \
	$(LOCAL_PATH)/stress-malloc.c \
	$(LOCAL_PATH)/stress-matrix.c \
	$(LOCAL_PATH)/stress-matrix-3d.c \
	$(LOCAL_PATH)/stress-mcontend.c \
	$(LOCAL_PATH)/stress-membarrier.c \
	$(LOCAL_PATH)/stress-memcpy.c \
	$(LOCAL_PATH)/stress-memfd.c \
	$(LOCAL_PATH)/stress-memhotplug.c \
	$(LOCAL_PATH)/stress-memrate.c \
	$(LOCAL_PATH)/stress-memthrash.c \
	$(LOCAL_PATH)/stress-mergesort.c \
	$(LOCAL_PATH)/stress-metamix.c \
	$(LOCAL_PATH)/stress-mincore.c \
	$(LOCAL_PATH)/stress-misaligned.c \
	$(LOCAL_PATH)/stress-mknod.c \
	$(LOCAL_PATH)/stress-mlock.c \
	$(LOCAL_PATH)/stress-mlockmany.c \
	$(LOCAL_PATH)/stress-mmap.c \
	$(LOCAL_PATH)/stress-mmapaddr.c \
	$(LOCAL_PATH)/stress-mmapfiles.c \
	$(LOCAL_PATH)/stress-mmapfixed.c \
	$(LOCAL_PATH)/stress-mmapfork.c \
	$(LOCAL_PATH)/stress-mmaphuge.c \
	$(LOCAL_PATH)/stress-mmapmany.c \
	$(LOCAL_PATH)/stress-module.c \
	$(LOCAL_PATH)/stress-monte-carlo.c \
	$(LOCAL_PATH)/stress-mprotect.c \
	$(LOCAL_PATH)/stress-mpfr.c \
	$(LOCAL_PATH)/stress-mq.c \
	$(LOCAL_PATH)/stress-mremap.c \
	$(LOCAL_PATH)/stress-msg.c \
	$(LOCAL_PATH)/stress-msync.c \
	$(LOCAL_PATH)/stress-msyncmany.c \
	$(LOCAL_PATH)/stress-munmap.c \
	$(LOCAL_PATH)/stress-mutex.c \
	$(LOCAL_PATH)/stress-nanosleep.c \
	$(LOCAL_PATH)/stress-netdev.c \
	$(LOCAL_PATH)/stress-netlink-proc.c \
	$(LOCAL_PATH)/stress-netlink-task.c \
	$(LOCAL_PATH)/stress-nice.c \
	$(LOCAL_PATH)/stress-nop.c \
	$(LOCAL_PATH)/stress-null.c \
	$(LOCAL_PATH)/stress-numa.c \
	$(LOCAL_PATH)/stress-oom-pipe.c \
	$(LOCAL_PATH)/stress-opcode.c \
	$(LOCAL_PATH)/stress-open.c \
	$(LOCAL_PATH)/stress-pagemove.c \
	$(LOCAL_PATH)/stress-pageswap.c \
	$(LOCAL_PATH)/stress-pci.c \
	$(LOCAL_PATH)/stress-personality.c \
	$(LOCAL_PATH)/stress-peterson.c \
	$(LOCAL_PATH)/stress-physpage.c \
	$(LOCAL_PATH)/stress-pidfd.c \
	$(LOCAL_PATH)/stress-ping-sock.c \
	$(LOCAL_PATH)/stress-pipe.c \
	$(LOCAL_PATH)/stress-pipeherd.c \
	$(LOCAL_PATH)/stress-pkey.c \
	$(LOCAL_PATH)/stress-plugin.c \
	$(LOCAL_PATH)/stress-poll.c \
	$(LOCAL_PATH)/stress-powmath.c \
	$(LOCAL_PATH)/stress-prctl.c \
	$(LOCAL_PATH)/stress-prefetch.c \
	$(LOCAL_PATH)/stress-prio-inv.c \
	$(LOCAL_PATH)/stress-priv-instr.c \
	$(LOCAL_PATH)/stress-procfs.c \
	$(LOCAL_PATH)/stress-pthread.c \
	$(LOCAL_PATH)/stress-ptrace.c \
	$(LOCAL_PATH)/stress-pty.c \
	$(LOCAL_PATH)/stress-quota.c \
	$(LOCAL_PATH)/stress-qsort.c \
	$(LOCAL_PATH)/stress-race-sched.c \
	$(LOCAL_PATH)/stress-radixsort.c \
	$(LOCAL_PATH)/stress-randlist.c \
	$(LOCAL_PATH)/stress-ramfs.c \
	$(LOCAL_PATH)/stress-rawdev.c \
	$(LOCAL_PATH)/stress-rawpkt.c \
	$(LOCAL_PATH)/stress-rawsock.c \
	$(LOCAL_PATH)/stress-rawudp.c \
	$(LOCAL_PATH)/stress-rdrand.c \
	$(LOCAL_PATH)/stress-readahead.c \
	$(LOCAL_PATH)/stress-reboot.c \
	$(LOCAL_PATH)/stress-regs.c \
	$(LOCAL_PATH)/stress-remap.c \
	$(LOCAL_PATH)/stress-rename.c \
	$(LOCAL_PATH)/stress-resched.c \
	$(LOCAL_PATH)/stress-resources.c \
	$(LOCAL_PATH)/stress-revio.c \
	$(LOCAL_PATH)/stress-ring-pipe.c \
	$(LOCAL_PATH)/stress-rlimit.c \
	$(LOCAL_PATH)/stress-rmap.c \
	$(LOCAL_PATH)/stress-rotate.c \
	$(LOCAL_PATH)/stress-rseq.c \
	$(LOCAL_PATH)/stress-rtc.c \
	$(LOCAL_PATH)/stress-sctp.c \
	$(LOCAL_PATH)/stress-schedmix.c \
	$(LOCAL_PATH)/stress-schedpolicy.c \
	$(LOCAL_PATH)/stress-seal.c \
	$(LOCAL_PATH)/stress-seccomp.c \
	$(LOCAL_PATH)/stress-secretmem.c \
	$(LOCAL_PATH)/stress-seek.c \
	$(LOCAL_PATH)/stress-sem.c \
	$(LOCAL_PATH)/stress-sem-sysv.c \
	$(LOCAL_PATH)/stress-sendfile.c \
	$(LOCAL_PATH)/stress-session.c \
	$(LOCAL_PATH)/stress-set.c \
	$(LOCAL_PATH)/stress-shellsort.c \
	$(LOCAL_PATH)/stress-shm.c \
	$(LOCAL_PATH)/stress-shm-sysv.c \
	$(LOCAL_PATH)/stress-sigabrt.c \
	$(LOCAL_PATH)/stress-sigbus.c \
	$(LOCAL_PATH)/stress-sigchld.c \
	$(LOCAL_PATH)/stress-sigfd.c \
	$(LOCAL_PATH)/stress-sigfpe.c \
	$(LOCAL_PATH)/stress-sigio.c \
	$(LOCAL_PATH)/stress-signal.c \
	$(LOCAL_PATH)/stress-signest.c \
	$(LOCAL_PATH)/stress-sigpending.c \
	$(LOCAL_PATH)/stress-sigpipe.c \
	$(LOCAL_PATH)/stress-sigq.c \
	$(LOCAL_PATH)/stress-sigrt.c \
	$(LOCAL_PATH)/stress-sigsegv.c \
	$(LOCAL_PATH)/stress-sigsuspend.c \
	$(LOCAL_PATH)/stress-sigtrap.c \
	$(LOCAL_PATH)/stress-sigxcpu.c \
	$(LOCAL_PATH)/stress-sigxfsz.c \
	$(LOCAL_PATH)/stress-skiplist.c \
	$(LOCAL_PATH)/stress-sleep.c \
	$(LOCAL_PATH)/stress-smi.c \
	$(LOCAL_PATH)/stress-sock.c \
	$(LOCAL_PATH)/stress-sockabuse.c \
	$(LOCAL_PATH)/stress-sockdiag.c \
	$(LOCAL_PATH)/stress-sockfd.c \
	$(LOCAL_PATH)/stress-sockpair.c \
	$(LOCAL_PATH)/stress-sockmany.c \
	$(LOCAL_PATH)/stress-softlockup.c \
	$(LOCAL_PATH)/stress-spawn.c \
	$(LOCAL_PATH)/stress-sparsematrix.c \
	$(LOCAL_PATH)/stress-splice.c \
	$(LOCAL_PATH)/stress-stack.c \
	$(LOCAL_PATH)/stress-stackmmap.c \
	$(LOCAL_PATH)/stress-statmount.c \
	$(LOCAL_PATH)/stress-str.c \
	$(LOCAL_PATH)/stress-stream.c \
	$(LOCAL_PATH)/stress-swap.c \
	$(LOCAL_PATH)/stress-switch.c \
	$(LOCAL_PATH)/stress-sync-file.c \
	$(LOCAL_PATH)/stress-syncload.c \
	$(LOCAL_PATH)/stress-sysbadaddr.c \
	$(LOCAL_PATH)/stress-syscall.c \
	$(LOCAL_PATH)/stress-sysinfo.c \
	$(LOCAL_PATH)/stress-sysinval.c \
	$(LOCAL_PATH)/stress-sysfs.c \
	$(LOCAL_PATH)/stress-tee.c \
	$(LOCAL_PATH)/stress-timer.c \
	$(LOCAL_PATH)/stress-timerfd.c \
	$(LOCAL_PATH)/stress-time-warp.c \
	$(LOCAL_PATH)/stress-tlb-shootdown.c \
	$(LOCAL_PATH)/stress-tmpfs.c \
	$(LOCAL_PATH)/stress-touch.c \
	$(LOCAL_PATH)/stress-tree.c \
	$(LOCAL_PATH)/stress-trig.c \
	$(LOCAL_PATH)/stress-tsc.c \
	$(LOCAL_PATH)/stress-tsearch.c \
	$(LOCAL_PATH)/stress-tun.c \
	$(LOCAL_PATH)/stress-udp.c \
	$(LOCAL_PATH)/stress-udp-flood.c \
	$(LOCAL_PATH)/stress-umount.c \
	$(LOCAL_PATH)/stress-unshare.c \
	$(LOCAL_PATH)/stress-uprobe.c \
	$(LOCAL_PATH)/stress-urandom.c \
	$(LOCAL_PATH)/stress-userfaultfd.c \
	$(LOCAL_PATH)/stress-usersyscall.c \
	$(LOCAL_PATH)/stress-utime.c \
	$(LOCAL_PATH)/stress-vdso.c \
	$(LOCAL_PATH)/stress-vecfp.c \
	$(LOCAL_PATH)/stress-vecmath.c \
	$(LOCAL_PATH)/stress-vecshuf.c \
	$(LOCAL_PATH)/stress-vecwide.c \
	$(LOCAL_PATH)/stress-verity.c \
	$(LOCAL_PATH)/stress-vforkmany.c \
	$(LOCAL_PATH)/stress-vm.c \
	$(LOCAL_PATH)/stress-vm-addr.c \
	$(LOCAL_PATH)/stress-vm-rw.c \
	$(LOCAL_PATH)/stress-vm-segv.c \
	$(LOCAL_PATH)/stress-vm-splice.c \
	$(LOCAL_PATH)/stress-vma.c \
	$(LOCAL_PATH)/stress-vnni.c \
	$(LOCAL_PATH)/stress-wait.c \
	$(LOCAL_PATH)/stress-waitcpu.c \
	$(LOCAL_PATH)/stress-watchdog.c \
	$(LOCAL_PATH)/stress-wcs.c \
	$(LOCAL_PATH)/stress-workload.c \
	$(LOCAL_PATH)/stress-x86cpuid.c \
	$(LOCAL_PATH)/stress-x86syscall.c \
	$(LOCAL_PATH)/stress-xattr.c \
	$(LOCAL_PATH)/stress-yield.c \
	$(LOCAL_PATH)/stress-zero.c \
	$(LOCAL_PATH)/stress-zlib.c \
	$(LOCAL_PATH)/stress-zombie.c \

LOCAL_CFLAGS	:= -Wall -Wextra -DVERSION='"0.17.07"' -DHAVE_PEDANTIC -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
LOCAL_LDFLAGS	:= -latomic  -lacl -laio -lcrypt -ldl  -lz  -lrt  -lpthread  -lm  -lc -lkmod -ljpeg -lgmp -lgbm -lmpfr
LOCAL_OBJS_DEPEND := stress-eigen-ops.o

$(LOCAL_OBJS_DEPEND): config.h
	cd $(STRESS_PATH) && $(CXX) -c -o $()


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): config.h $(LOCAL_OBJS_DEPEND) $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

