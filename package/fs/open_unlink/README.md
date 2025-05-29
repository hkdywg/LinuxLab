**NOTE!**

# 应用软件中，文件在open后进行read/write操作，如果在没有close文件之前进行delete操作，这样操作系统会删除file对应的dentry信息，但inode没有删除，已经打开的文件依旧可以
# 访问









