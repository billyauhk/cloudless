default:cloudless

cloudless:cloudless.cpp
	g++ cloudless.cpp -O2 -o cloudless -I/usr/local/include \
		`pkg-config opencv --cflags --libs` \
		-fopenmp -lgomp
	export OMP_NUM_THREADS=4
charlie:cloudless.cpp
	g++ cloudless.cpp -O2 -o charlie -I/usr/local/include \
		`pkg-config opencv --cflags --libs` \
		-fopenmp -lgomp -DORIGINAL
	export OMP_NUM_THREADS=4
tiff:tiff.cpp
	g++ tiff.cpp -O2 -o tiff -I/usr/local/include \
		`pkg-config opencv --cflags --libs`
version:version.cpp
	g++ version.cpp -O2 -o tiff -I/usr/local/include \
		`pkg-config opencv --cflags --libs`
fixpath:
	sudo ldconfig -v
	echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH"
