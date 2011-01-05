#!/usr/bin/python

from PIL import Image
import glob, os, sys

if len(sys.argv) < 2:
	print "NEEDS IMAGE ARGUMENT"
	sys.exit(-1)

inpng_fname=sys.argv[1]

im = Image.open(inpng_fname)
sz = im.size
f = open(inpng_fname + ".out" ,"wb")
f.write(str(sz[0])+"\n"+str(sz[1])+"\n")
for y in range(sz[1]):
	for x in range(sz[0]):
		f.write(str(im.getpixel((x,y))[0])+"\n");

f.close()