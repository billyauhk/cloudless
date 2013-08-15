CC=g++
CFLAGS=-O2

default:cloudless

cloudless:cloudless.cpp
	$(CC) $(CFLAGS) cloudless.cpp -o cloudless -I/usr/local/include \
		`pkg-config opencv --cflags --libs` \
		-fopenmp -lgomp
	export OMP_NUM_THREADS=4
charlie:cloudless.cpp
	$(CC) $(CFLAGS) cloudless.cpp -o charlie -I/usr/local/include \
		`pkg-config opencv --cflags --libs` \
		-fopenmp -lgomp -DORIGINAL
	export OMP_NUM_THREADS=4
tiff:tiff.cpp
	$(CC) $(CFLAGS) tiff.cpp -o tiff -I/usr/local/include \
		`pkg-config opencv --cflags --libs`
version:version.cpp
	$(CC) $(CFLAGS) version.cpp -o tiff -I/usr/local/include \
		`pkg-config opencv --cflags --libs`
renderMODIS:renderMODIS.cpp
	$(CC) $(CFLAGS) renderMODIS.cpp -o renderMODIS
fixpath:
	sudo ldconfig -v
	echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH"
