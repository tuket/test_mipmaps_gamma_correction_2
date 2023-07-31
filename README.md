This is a simple program to test if gamma correction is taken into account by OpenGL when computing the mipmaps

All the code is contained in [main.cpp](https://github.com/tuket/test_mipmaps_gamma_correction_2/blob/master/src/main.cpp)

1) Compile like any standard CMake project.
2) Run the program in the cmd line like: `test.exe input_image.png`

The program will create 4 output files in the working directory:
1) **cpu_no_conversion.png**: box filtering performed on the CPU, without any gamma conversions
2) **cpu_conversion.png**: box filtering performed on the CPU, converting from gamma to linear, and back from linear to gamma
3) **gpu_no_conversion.png**: let OpenGL create the mipmaps, RGB format
4) **gpu_conversion.png**: let OpenGL create the mipmaps, SRGB format

Also the program print a table with the difference between the previous output images. For example:
```
           0     2102348       49224     2121146
     2102348           0     2276882       10106
       49224     2276882           0     2295830
     2121146       10106     2295830           0
```

In the previous example, the difference between `cpu_conversion.png` and `gpu_conversion.png` is `10106`.
