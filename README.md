# ROS C++ implementation of Piotr Dollar's pedestrian detector
The C++ code is based on the great MATLAB toolbox by Piotr Dollar [here](https://pdollar.github.io/toolbox/) or on the [github](https://github.com/pdollar/toolbox) (recommended)

# Setting up MATIO (matlab input/output interface)
	- Download it: http://sourceforge.net/projects/matio/
	- Execute the following commands in the directory of the downloaded file.
		$ tar zxf matio-X.Y.Z.tar.gz
                $ cd matio-X.Y.Z
                $ ./configure (you might need to make this file an executable)
                $ make
                $ make check
                $ make install

	- Check the src/pedestriandetectorros/CMakeLists.txt to make it point to the right position of libmatio.so
	- If you get an error like: "undefined reference to H5...".:
		- Delete the installation folder matio-X.Y.Z by "rm -R matio-X.Y.Z"
		- $ tar zxf matio-X.Y.Z.tar.gz
                - $ cd matio-X.Y.Z
                - $ ./configure --without-hdf5
                - $ make
		- $ sudo make install

