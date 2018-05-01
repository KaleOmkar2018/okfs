# okfs 
Learn how to create a simple filesystem from a reference filesystem : simplefs. <br />
Kernel Version : 4.13.0.37	<br />
<br />
1.Basics <br />
-Added a source file : okfs.c
-Added a kernel module.<br />
-Added a Makefile<br />
-Screenshot of output.<br />
.
2.Filesystem Basic structure<br />
-Added a filesystem type structure.<br />
-Modified init module to include filesystem registration.<br />
-Modified exit module to include filesystem unregistration.<br />
-Modified makefile to change the errors in Makefile.<br />
 -okfs_output.ko file will be produced<br />
-Screenshot of output.<br />

3.Added an user space application to write data to a file.<br />
-Added support for mount and kill_sb.<br />
-Added a header file for filesystem structures and shared data.<br />
-Added root directory creation support<br />
-Change of file names: fs.c : source file and kernel module will be okfs.ko.<br />
-Screenshot removed<br />
-Device has not been created yet.So, code testing will be done in next commit.<br />
-Comments have been added. Comments will be removed after 2 commits.<br />
