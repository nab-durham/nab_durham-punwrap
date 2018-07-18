from __future__ import print_function
import numpy
import sys
from __init__ import unwrap2D

phaseR=lambda x : numpy.arctan2(x.imag,x.real)

radius=numpy.add.outer(
   (numpy.arange(64)-31.5)**2.0,(numpy.arange(64)-31.5)**2.0 ) 

mask=1*(radius<31**2.0)
maskI=numpy.flatnonzero( mask.ravel() )

phaseStart=radius*6*2*numpy.pi/31**2.0
phaseWrapped=(phaseStart+numpy.pi)%(numpy.pi*2)-numpy.pi ;
phaseUnwrapped=unwrap2D(phaseWrapped,mask)

print("<< NOISELESS")
print("Wrapped-start difference: {0:5.3g}".format(
      numpy.var((phaseStart-phaseWrapped).ravel().take(maskI))))
print("Unwrapped-start difference: {0:5.3g}".format(
      numpy.var((phaseStart-phaseUnwrapped).ravel().take(maskI))))
sys.stdout.flush()


# now with noise
print("<< WITH NOISE, UNIFORM PHASE, GAUSSIAN AMPLITUDE")
nIters=100
noiseAmps=numpy.power(10,
   numpy.linspace(numpy.log10(1e-3),numpy.log10(4.0),16).tolist()+[2])
noiseData=numpy.zeros([len(noiseAmps),nIters],numpy.float32)
print("noise  Unwrapped-start  Actual var")
for tna in range(len(noiseAmps)):
   thisnoiseAmp=noiseAmps[tna]
   for ti in range(nIters):
      thisnoise=numpy.random.normal(0,thisnoiseAmp,size=radius.shape)*\
            numpy.exp(1.0j*numpy.random.uniform(0,2*numpy.pi))
      thisphaseWrapped=mask*phaseR(numpy.exp(1.0j*phaseWrapped)+thisnoise)
      thisphaseUnwrapped=unwrap2D(thisphaseWrapped)

      noiseData[tna,ti]=numpy.var(
            (phaseStart-thisphaseUnwrapped).ravel().take(maskI))

   print("{0:7.3f}{1:9.2g}+/-{2:5.2g}{3:10.2f}".format(
         thisnoiseAmp,
         noiseData[tna].mean(),noiseData[tna].var()**0.5,
         numpy.var(phaseStart.ravel().take(maskI))) )
   sys.stdout.flush()

