Based on the Izhikevich model, we have
```
v' = 0.04v**2 + 5v + 140 - u + I
u' = a(bv - u)
if v >= 30, then {v = c, u = u + d}
```

which amounts to, every ms:
```
if (v >= 30) {
  v = c;
  u += d;
} else {
  v = (0.04 * vprev * vprev + 5 * vprev + 140 - uprev + I);
  u = uprev + a * (b * vprev - uprev);
}
```

Pretty simple to work out in python, I would figure. And my code seems to have shown as much.

So just to make things a challenge, let's see if I can get something fairly fast going in integers. int16_t means -32K -> 32K for range. We'll map the -100 -> 100 range onto that.
```
v' = 0.04 * v * v + 5 * v + (140 * 320) - u + (I * 320)
u' = (a)(b*v - u)
if v >= (30 * 320), then {v = c * 320, u = u + d * 320}
```

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

Investigating this problem led me down a deep rabbit hole, read about it in [my digression on the matter](https://github.com/patrickyeon/izhi-neurons/blob/master/digression.md), or just read on without worrying too much about it. The short story is that I'm now on [0cc92b5:izhi.c], using 32bit ints and a modified (but stable) version of the neuron potential calculation.

My next task will be to check this code for accuracy and speed on the actual hardware it will eventually run on. To do this, I split the main loop out of `izhi.c` and also created some support code for my target. Finally I added a Makefile because keeping track of the build details got suddenly complicated when I wanted to build for *real* hardware. By this point, my local tree looks like [73bc981].

With the code running on the native hardware, I fired up my trusty Saleae Logic to watch pins A15 and B5 to see the timing of my `step_f` and `step_i` loops, respectively.

![step_f](https://github.com/patrickyeon/izhi-neurons/blob/master/data/logic_float.png?raw=true)
![step_i](https://github.com/patrickyeon/izhi-neurons/blob/master/data/logic_int32.png?raw=true)

It looks like the fixed-point implementation runs 4x faster at ~0.4ms per loop, vs. ~1.67ms for the floating-point version. This surprised me, I actually expected the floating-point one to be much much worse! As a quick check, I re-compiled with `typedef int16_t fixed_t;` to see if 16 bit integers gave any improvement (I expected not, but especially with performance issues it's important to test one's assumptions), and the integer runtime went down to about 0.32ms per `step_i` loop. Of course, the behaviour would be completely incorrect because of the integer overflow I was struggling with when trying to develop a 16bit version, but it's good to have the datapoint that it looks like a series of 16bit operations takes about 80% as long as a matching series of 32bit operations.

For completeness, I checked the two most obvious compiler optimizations: `-Os` (optimize for size with speedups that tend not to increase size) and `-O3` (optimize for speed). These led to floating and fixed point loop times of 1.54ms/0.4ms and 1.6ms/0.37ms, so not really huge gains to be had there. This time I'm not surprised; the work being done is pretty straightforward and simple.

So how fast do I need to get going? Well Zak didn't exactly give me a spec to hit, but in [his log on implementing this for v07](https://hackaday.io/project/3339-neurobytes/log/31094-attiny-izhikevichish-dynamics-part-two) he says he needs to update the LEDs every 30us and has the model calculations broken up in to 7 steps of no more than 29us each (there's also steps for reading input dendrites, calculating the value I'm passing as `synapse`, and translating the neuron state into a colour value for the LEDs). To match this I'll aim for less than 200us to run through the neuron model. 

The first improvement is the easiest: when the STM32L0 used on the board I have reboots, it starts up with a 2MHz internal clock, but the chip can run up to 16MHz (actually, 32MHz, but the board I have doesn't have an external oscillator, so I'm limited to 16MHz). When I switched over to that oscillator, the simple `step_i` loop went down to 52us when not including the 0.8us spent turning the pin I'm using for timing on and off. The smart thing to do would be to try to split the code inside `step_i` into two parts of roughly equal runtime and call it a day, or maybe three so that there's a bit of room for slack, but I'm pretty sure there's some easy wins to be had so I'll just go for those.

When Zak had to speed up the math on the AVR, he changed all the multiplications and divisons to use powers of two, so that he could just use shift operations. The STM32 has a one-cycle integer multiplier, so I'll start off by attacking division operations. The easiest one is the `/fracms` that I use twice per loop to step the model by timesteps shorter than a millisecond. I've redefined the function so that it now steps by 2 ** (-fracms) milliseconds and the main loop is now stepping by 1/8th of a ms each loop, the nearest power of two to 1/10th.

Just this change has brought me down to 37us per loop! This is almost down to being able to run the full model every time the LED timing needs to be updated, so I'm going to press harder still. Even though the datasheet claims this chip has a single-cycle multiplier, I'd feel better seeing proof of the fact. Yes, datasheets have lied to me in the past so I will fact-check them when it's easy to do so. Changing the `v * 5` component to `(v + (v << 2))` leads to 37.4us per loop, so I left it as the clearer `v * 5` and I'll not try to eliminate any more multiplies.

The divisions by `neuron->a_inv` and `neuron->b_inv` would be hard to turn into shift operations (or even groups of shifts and adds) because they are elements of the model and I will want to keep the model as general as I can support. This leaves me the calculation of `partial` to work on.

`FSCALE` is completely implementation-dependent, and `1000` is conveniently close to `1024 == (2 ** 10)`. So doing that bought me... exactly no improvement. It doesn't seem right to me that I'd get 15us of speedup by removing two divides, and then no further speedup by removing a third, so I wanted to see what was happening. I added debug listings to the compiler output with the `-g` flag, and then re-compiled the original (two divides to calculate `partial`) and got an assembly listing (`arm-none-eabi-objdump -S stm.hex > stm.list`) to see what the compiler was up to.

```
    fixed_t partial = (v / SQRT_FSCALE) / 5;
 80002f8:   69fa        ldr r2, [r7, #28]
 80002fa:   23a0        movs    r3, #160    ; 0xa0
 80002fc:   0159        lsls    r1, r3, #5
 80002fe:   0010        movs    r0, r2
 8000300:   f000 fb86   bl  8000a10 <__aeabi_idiv>
 8000304:   0003        movs    r3, r0
 8000306:   617b        str r3, [r7, #20]
```

Oh, clever compiler. It recognized that I was dividing by a known constant, and then again by another known constant, and combined them into a single division call to help me out. I didn't get a speedup by eliminating one of the division operations because the compiler had already gotten rid of one for me for free. Oh well, I guess I'll just try harder...

My next gut reaction was to add up order-of-two fractions to approximate 1/5 (eg. 1/8 + 1/16 + 1/128 + 1/256  is 0.19921875, and that's pretty close), but that started to look like a lot of partial fractions to make up the precision. Instead, I figured I could pre-multiply `(v >> LOG_SQRT_FSCALE)` so that I'd have an easy right-shift to approximate a divide-by-5. Not wanting to make it harder to analyze to protect myself from integer overflow again later, I decided to keep this multiplicand below `SQRT_FSCALE`, so:
```
1024 * 5 = 5120
log2(5120) = 12.322 (use 2**12 == 4096)
4096 / 5 = 819.2 (use 819)
```

And now I'll have `partial = ((v >> LOG_SQRT_FSCALE) * 819) >> 12;` The error is about 244 parts per million (actually, much much less, as this partial result is squared in the next step), and I'm okay with that.

Once again, I recompiled and ran the code and it got down to 30.2us per loop. Inspired by seeing the compiler combine divisors into one division call, I pulled `neuron->b_inv` out a level and cut down the work of calculating `neuron->recovery` to a single division, and got a huge improvement, with per-loop timing down to 17.8us! I don't know why that lopped over 12us off the loop timing when previously removing a divide would only shave 7us, but I'll take it.

I'm happy with where that stands for now, removing the final division is not impossible (my first attempt would be a pre-multiply and shift-right strategy like I did when calculating `partial`) but I think it would add too much complexity when setting up the neuron properties. I did a timing run with the remaining division replaced by a hard-coded right shift just to see how fast that would make the operation, and it was down to 7.2us per loop. At least I know that's available if I want it.

The last thing to do is implement constructors for the other neuron types and then compare them to floating-point implementations. The actual calculations should not be affected by the performance improvements, but it'll be good to check to make sure. To do this I added the `plots` target to my `Makefile` and when checking through them it mostly looks alright.
- Most neuron outputs look like they do in the paper, and the fixed and floating point implementations don't differ by much
- The fixed point implementation looks a little slower than the floating point one, but not horribly so.
- My IB implementations don't agree with each other, and neither agrees with what was presented in the paper.
- My RZ implementations agree with each other, but are always firing, unlike the RZ in the paper.

I don't quite know why the IB and RZ neurons aren't playing nice, but I'm going to leave that to another day.
