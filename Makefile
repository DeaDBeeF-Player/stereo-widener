all:
	gcc -I/usr/local/include  -std=c99 -shared -O2 -o stereo_widener.so stereo_widener.c -fPIC
