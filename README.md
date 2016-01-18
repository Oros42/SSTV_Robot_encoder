# SSTV_Robot_encoder
Simple and fast SSTV encoder for Robot8BW and Robot24BW.


Beacause pySSTV* is realy to slow on a Raspberry-Pi B, I rewrite it in C :-D  
* pySSTV : https://github.com/dnet/pySSTV  
  

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
apt-get install gcc libgd-dev  
gcc SSTV_Robot_encoder.c -o SSTV_Robot_encoder -lgd -lm  
```
  
Who to run
----------

```
./SSTV_Robot_encoder INPUT_IMAGE [OUTPUT_WAVE [MODE]]  
```