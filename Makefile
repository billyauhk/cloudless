default:cloudless

cloudless:cloudless.cpp
	g++ cloudless.cpp -o cloudless -I/usr/local/include \
		`pkg-config opencv --cflags --libs` \
		-fopenmp -lgomp
	export OMP_NUM_THREADS=2
fixpath:
	sudo ldconfig -v
	echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH"
