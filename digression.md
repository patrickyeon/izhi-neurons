It looked like I had converted the numbers properly, so I started looking for another explanation, something specific to how I was doing the fixed point math. The fixed point neuron seems to rise a little bit slower than the floating point one, so I thought maybe I had a small error in my math that on any one step would be inconsequential, but over hundreds of steps would add up.

When you divide integers in C, the computer doesn't necessarily round the result up or down to the nearest integer, it will just truncate it and throw away the lost precision. This means that every number between 50/10 = 5, 54/10 = 5, 56/10 = 5, and even 59/10 = 5. This is one way to have a constant bias on your results, and the way I know to deal with this is to add half of your divisor to the numerator before doing the division. (50 + 5)/10 = 5, (54 + 5)/10 = 5, but (55 + 5)/10 = 6.

I tried applying this trick, and it didn't really help. In fact, when I split out `((v * v) / FSCALE / 25 + 5 * v + 140 * FSCALE - u + synapse)` into a partial result, the whole calculation went sideways and `neuron->potential` would just oscillate really quickly in between -20 and 0. I think the math in that equation was all being done in 32 (or 64) bit integers that are the native type on my computer and only being re-cast to 16 bits at the end. Forcing the partial result in to a 16 bit int exposed an integer overflow that wasn't obvious otherwise.

Checking this guess was pretty easy, I changed to `typedef int32_t fixed_t;` and the result was back to well-behaved spikes (but still activating at a slower-then-expected rate). Okay sure, my guess of `#define FSCALE 320` must have been a little too high, so I cut it down a bit and ran with
```
typedef int16_t fixed_t;
#define FSCALE 160
```
Aaaand... oh dear. That hasn't exactly helped.
[data/digression_fscale_160.png]
It seems I was just lucky with my first choice for scaling, and there's actually a lot of choices that would not work (reducing the scale should be safe, so long as the lost precision doesn't destroy the behaviour). It's time to stop shooting from the hip on this one, and go back to proper analysis.

My initial thought on the scaling was that the neuron potential stayed well within the -100..100 range, so I could safely use that as my min and max values. Looking ot the equations as I've written them out, that's clearly wrong for partial results. Just starting with the squaring term and substituting our assumed max for `v`, `v * v => (100 * FSCALE) * (100 * FSCALE) ==> 10000 * FSCALE * FSCALE`. Whoops. Even if we were to restrict `v` to 30, we'd be at `900 * FSCALE**2` (and I can tell you that is a bad representation, because `v` actually resets to -65, so the squared result is over 4 times larger still).

Luckily, we can re-order the operations. The next thing I do with this squared result is divide it, so I can keep everything more in check by distributing that division inside the squaring.
`partial = (v * v) / FSCALE / 25` becomes `partial = ((v / (SQRT_FSCALE * 5)) * (v / (SQRT_FSCALE * 5)))`, where `SQRT_FSCALE` is the square root of `FSCALE`. Checking now to see how likely it is we overflow:
```
((100 * FSCALE) / (SQRT_FSCALE * 5))
 ==> ((100 / 5) * (FSCALE / SQRT_FSCALE)) 
 ==> (20 * SQRT_FSCALE)
```
and we can see that squaring that term will give us a max of `400 * FSCALE`. Much more reasonable.

Of course, the next two terms we add are `5 * v + 140 * FSCALE`, which would max out at `5 * (100 * FSCALE) + 140 * FSCALE == 640 * FSCALE`.

Turning our attention to the neuron recovery variable, `u`, even with ridiculously large values of `v = 100; u = -100`, the step size is 2.2, so I don't see it contributing much (`2.2 * FSCALE` that is) here. Similarily, for now the value of `synapse` I'm using is `10 * FSCALE`, so combined they are on the order of 1% the max contribution of the first three terms.

So I can choose `FSCALE` as roughly `INT_MAX / 1100` and try again. This could be 29.789, but I need to round down (integers!) to 29, and then there's an additional restriction that I need `SQRT_FSCALE` as well, so may as well cut back to `FSCALE = 25; SQRT_FSCALE = 5;`. Trying that, I get:
[data/digression_fscale_25.png]
Okay, not exactly stellar either. Zooming in to the start of this run:
[data/digression_fscale_25_zoom.png]
the fixed point implementation looks "choppy", which seems typical of too little precision to me.

That large partial is later on divided by `fracms`, which looks like another great chance to buy a bit of headroom. If I want to have a significant affect, I need to reduce the size of those first three terms, and the most difficult to do so with will be the squared term. I can't just divide it outright (which would be the easy solution, divide it by `fracms`), because I still need to store the squared value somewhere, so either I apply the divisor to only one of the subpartials (and suffer a loss of precision), or I need a divisor I can break in to two terms and apply to each. For now, I'm going to lock down a `fracms` of 10, and see how that shakes out. This is a hit on the flexibility of the `step_i` function, but we need to get it working correctly first.

```
    fixed_t subpartial = v / (SQRT_FSCALE * 5);
    fixed_t partial = ((subpartial / 5) * (subpartial / 2) + v / 2
                       + 14 * FSCALE) + (synapse - u) / 10;
    neuron->potential = v + partial;
```
Now `partial` will be on the order of `(400 * FSCALE / 10) + 100 * FSCALE / 2 + 14 * FSCALE == 104 * FSCALE`, actually much closer to our original `100 * FSCALE` limit. 32768/104 == 315.077, the nearest perfect square without going over is 289 (17**2). This is in [dda698c:izhi.c], and gave a more promising, but not perfect waveform:
[data/digression_fscale_289.png]
Actually, zooming in I can see distinct steps in the neuron potential which makes me suspect a loss of precision:
[data/digression_fscale_289_zoomin.png]
and plotting out the value of `neuron->recovery` shows I very distinctive break from the floating point behaviour:
[data/digression_fscale_289_recovery.png]

I looked at a few different choices for `FSCALE` and juggled the distribution of divisors around a little bit, and I suspect that 16 bit signed ints are just a little shy of the dynamic range needed to implement this algorithm in a straightforward manner. So I'm going to do something I should've done a long time ago: I'll move to 32 bit ints.

`INT_MAX` on an int32_t is roughly 2 billion, so we'll set a conservative `FSCALE` of 1 million. Yes, that made things much much easier:
[data/digression_fscale_1M.png]

This version of the code is [0cc925b:izhi.c]

PS: If I were doing this work professionally, I would have been expected to try the 32bit integers first. The chip this is eventually targetting is a 32bit core, and can do 32bit multiplication in a single clock cycle. Even if it couldn't, I should be trying the easy thing first, and only spending my time optimizing some code if I can show it actually needs to be faster. It's often the case that code runs much differently than one expects, especially on an architecture one isn't intimately familiar with.

Even so, I've got more ideas for a 16bit version, so I may just come back to it later.
