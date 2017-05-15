Based on the Izhikevich model, we have
v' = 0.04v**2 + 5v + 140 - u + I
u' = a(bv - u)

if v >= 30, then {v = c, u = u + d}

which amounts to, every ms:

if (v >= 30) {
  v = c;
  u += d;
} else {
  v = (0.04 * vprev * vprev + 5 * vprev + 140 - uprev + I);
  u = uprev + a * (b * vprev - uprev);
}

Pretty simple to work out in python, I would figure. And my code seems to have shown as much.

So just to make things a challenge, let's see if I can get something fairly fast going in integers. int16_t means -32K -> 32K for range. We'll map the -100 -> 100 range onto that.

v' = 0.04 * v * v + 5 * v + (140 * 320) - u + (I * 320)
u' = (a)(b*v - u)
if v >= (30 * 320), then {v = c * 320, u = u + d * 320}

I threw these together quickly in python, and things look like I expect them to, so I'll move along to the *real* programming. C.

First step will be to re-implement the floating-point version in C with some way to visualize the output. [aade0e6:izhi.c] contains this straightforward implementation, the output can be generated like so:
```
gcc izhi.c -o izhi
./izhi > rs.dat
```
I use `gnuplot` to display this quickly, by starting it up and using the command `plot 'rs.dat' with lines`. The output looks more or less like the outputs from the paper, at least to my eye.
![baseline](https://github.com/patrickyeon/izhi-neurons/blob/master/data/baseline.png?raw/true)

Next I'll implement the easiest fixed-point version I can think of, and we'll see how well its output aligns with the floating-point version. I've done this in [05b939d:izhi.c] and I'd like to highlight a few habits that usually make my life easier:
```
typedef int16_t fixed_t;
#define FSCALE 320
```
I took an initial guess that I'd be using 16-bit ints (this eventually will target a 32bit processor, but Zach's earlier work was happily using 16 bits, so I'm starting there), but if I change my mind later I don't want to track down every place that I declared an int16_t and change it to an int32_t. That's setting myself up for a mistake and a frustrating bug, so I use the `typedef` to "create" a new type, aliased to `int16_t`, that I can use all over and only need to change one line to switch it all to a different size.

Similarly, I `#define`'d the scaling factor right there as well. This number is equivalent to 1.0 in the floating point implementation, and again if I find my scaling is wrong, I can just change it in this one place instead of tracking down every usage. I chose 320 because a 16bit signed int goes from -2^15 to +2^15, or roughly speaking -32000 to +32000, and the neuron potential and recovery values stay comfortably within the -100 to +100 range. There's definitely value in analyzing this decision more thoroughly; if I'm too cautious then I'm losing precision by never exercising the full range, but if I'm not cautious enough I risk overflowing the variable size and "wrapping around" from larger than the largest possible positive value to a very large negative value, or vice-versa.

Lastly, I kept the floating point implementation around so that I can run my new fixed point version against the same inputs and compare the two side-by-side to make it easier to spot any differnces in behaviour. And it's a good thing I did, because if I build and run this version and examine the output,
`gcc izhi.c -o izhi && ./izhi >rs.dat && gnuplot`
```
gnuplot> plot 'rs.dat' using 1:2 title 'floating' with lines, \
              'rs.dat' using 1:3 title 'fixed' with lines
```
I get this:
![fixed wrong](https://github.com/patrickyeon/izhi-neurons/blob/master/data/first_fixed.png?raw=true)

If I weren't comparing the two on the same plot, I'd be tempted to say they're basically the same. Side-by-side though, we can immediately see that one is running slightly faster than the other. If we count the spikes starting with the nearly-coincident ones near t=200 until the next nearly-coincident ones (roughly t = 375), there are 10 floating point spikes vs. 9 fixed point spikes. The fixed point implementation is rising approximately 90% as fast as the floating point implementation I'm considering my reference.

This is why I started with the simplest attempt I thought would work. If I saw this mis-alignment with a more clever attempt, I'd start asking myself where my cleverness let me down and I'd have many more dark corners to go looking in. As it stands, it's fairly simple to look through the code and see if I missed something. There's maybe a half-dozen places to make sure I did the (a*x -> x/(1/a)) conversion properly, and I can convince myself that I did it right.

Investigating this problem led me down a deep rabbit hole, read about it in [my digression on the matter]https://github.com/patrickyeon/izhi-neurons/blob/master/digression.md), or just read on without worrying too much about it. The short story is that I'm now on [0cc92b5:izhi.c], using 32bit ints and a modified (but stable) version of the neuron potential calculation.

My next task will be to check this code for accuracy and speed on the actual hardware it will eventually run on. To do this, split the main loop out of `izhi.c` and also created some support code for my target. Finally I added a Makefile because keeping track of the build details got suddenly complicated when I wanted to build for *real* hardware. By this point, my local tree looks like [73bc981].
