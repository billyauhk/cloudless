CC=g++
CFLAGS=-O2
INC_PATH=-I/usr/include/gdal/ -I/usr/local/include
LD_PATH=-L/usr/lib/

default:cloudless gibs_download

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
gibs_download:gibs_download.cpp
	LD_LIBRARY_PATH=${LD_PATH} g++ gibs_download.cpp -o gibs_download \
		${INC_PATH} ${LD_PATH} -lgdal1.7.0 `pkg-config opencv --cflags --libs` -fopenmp -lgomp
fixpath:
	sudo ldconfig -v
	echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH"
