Advanced OpenGL IMAC3 exercises
===============================

Build on Linux
--------------
<pre>
	premake4 gmake
	make config=debug
	make config=release
</pre>

Tested on Archlinux64 with NVidia drivers 304.60, Ubuntu 12.04 64 with NVidia drivers 310.14.


Build on Mac OSX
--------------
<pre>
	premake4 gmake
	make config=debug
	make config=release
</pre>

Tested on Mac OSX 10.7 with geForce GPU
Note: Using core profile on macosx generates a GL error at context creation.

Build on Windows
----------------
<pre>
	premake4 vs2008
	Use the .sln file as usual in Visual Studio
</pre>

Tested on Windows 7 64 with geForce GPU


Running
--------
Use the command line or in Visual Studio set launch parameters to the following :
<pre>
	bin/debug/td1/td1_corrected td1/1_3_corrected.glsl
	bin/release/td1/td1_corrected td1/1_3_corrected.glsl
</pre>