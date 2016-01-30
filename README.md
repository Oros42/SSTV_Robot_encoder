# SSTV_Robot_encoder
Simple and fast SSTV encoder for Robot8BW and Robot24BW.

Beacause pySSTV* is realy to slow on a Raspberry-Pi B, I rewrite it in C :-D  
\* pySSTV : https://github.com/dnet/pySSTV  
  
This program convert an image (jpeg, png, gif or bmp) to an SSTV wav file.  
You can decode the wav file with QSSTV (http://users.telenet.be/on4qz/).  


Benchmark on a Raspberry-Pi B
-----------------------------
With pySSTV :  
```
$ time python -m pysstv --mode Robot8BW photo.jpg output.wav  
  
real	0m19.954s  
user	0m19.360s  
sys	0m0.170s  
```

With this SSTV_Robot_encoder :  
```
$ time ./SSTV_Robot_encoder photo.jpg output.wav Robot8BW  
  
real	0m1.039s  
user	0m0.980s  
sys	0m0.050s  
```

How to buid
-----------
  
```
sudo apt-get install gcc libgd-dev  
gcc SSTV_Robot_encoder.c -o SSTV_Robot_encoder -lgd -lm  
chmod u+x SSTV_Robot_encoder
```
  
Who to run
----------

```
./SSTV_Robot_encoder INPUT_IMAGE [OUTPUT_WAVE [MODE]]  
```

Examples
--------

```
./SSTV_Robot_encoder photo_160x120.jpg
./SSTV_Robot_encoder photo_160x120.png
./SSTV_Robot_encoder photo_160x120.gif
./SSTV_Robot_encoder photo_160x120.bmp
./SSTV_Robot_encoder photo_160x120.jpg sstv_robot_8BW.wav
./SSTV_Robot_encoder photo_160x120.jpg sstv_robot_8BW.wav Robot8BW
./SSTV_Robot_encoder photo_320x240.png sstv_robot_24BW.wav Robot24BW
```
