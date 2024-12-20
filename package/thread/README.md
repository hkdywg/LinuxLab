## load_monitor模块打印

```
# [   15.509453] dump runing task.
[   15.509651] runing task, comm: test_thread, pid 177
[   15.509872]  __switch_to+0xc4/0x220
[   15.509980]  kmalloc_caches+0x0/0xe0

# [   18.685360] dump runing task.
[   18.685508] runing task, comm: test_thread, pid 177
[   18.685638]  __switch_to+0xc4/0x220
[   18.686030]  kernel_test_thread+0x78/0x8c [load_monitor]

#
# [   21.757780] dump runing task.
[   21.757908] runing task, comm: test_thread, pid 177
[   21.758028]  __switch_to+0xc4/0x220
[   21.758229]  0x38
```

## test_thread函数体

```
	while(1) {
		buffer = kzalloc(4 * 1024, GFP_KERNEL);
		kfree(buffer);

		while (time_before(jiffies, end)) {
			cpu_relax(); // 提示编译器不要优化掉这个循环
		}
	}
```


**NOTE!**

按我的理解load_monitor应该打印出test_thread函数的函数调用栈，包括kzalloc及其子函数的调用关系。


