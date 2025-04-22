# Now would be a good time to start getting excited


TL;DR The first batch of Jumperless V5s will be shipping to me this week, they'll be on their way to you shortly after.

But there's a lot of other cool stuff I've been working on while I wait for these to be assembled. So buckle up, this is gonna be a long, action-packed tour through the Jumperverse.


## Joom, it had to be done

First, here's a fun little side quest, [Joom](https://github.com/Architeuthis-Flux/joom).

[![Joom](https://img.youtube.com/vi/xWYWruUO0F4/maxresdefault.jpg)](https://www.youtube.com/watch?v=xWYWruUO0F4)


https://www.youtube.com/watch?v=xWYWruUO0F4

I couldn't really think of a good way to fit it all into the firmware so you could run it from the regular menus, so it's just a .uf2 file that will overwrite the stock firmware. When you're over it, you can just load the [real firmware again](https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest). 

> [!TIP] A quick refresher on loading firmware on an RP2xxx
> Just hold the **boot** button on the back side of the USB port while plugging it in and drop these .uf2 files onto the drive that pops up. 

When you have the regular firmware loaded, the [Jumperless App](https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest) will check for updates at startup and keep you up-to-date, but Joom doesn't initialize USB so you need to do it this way.



## A wedge between your desk and your Jumperless

As I've probably mentioned before, Jumperless V5 probably isn't going to replace your TV anytime soon. Because the LEDs are mounted on the PCB and shine through a small hole ~1/2 inch above them, the viewing angle is terrible. For wires and it's totally fine, the light bounces around and lights up the whole row so you can see it from any angle, but to read text and navigate menus it needs to be pointing right at your eyeballs. So having a stand propping it up at 45° is super handy.

### For people who shoot lasers at stuff

My friend at [Alpenglow Industries](https://www.alpenglowindustries.com/) is an absolute wizard with laser cut acrylic (as well as electronics and yarn stuff), so when she floated the idea of making a laser cut acrylic stand for the Jumperless in a [frankly absurd number of color options](https://www.alpenglowindustries.com/pages/swift-colors), I got to work designing it.

![](/images/UpdateImages/08-post-campaign-images/AcrylicStandCollage.jpg)

Turns out designing acrylic stuff that a human can assemble and stays together is hard and also I suck at it. This will fit together, but the order of operations is super weird and kinda needs to have some hot glue shoved into all the slotted parts to feel solid. But if you have a laser and want to try it, the files are on [Github here](https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/Jumperless23V50/Stands/Acrylic).

Eventually [Carrie](https://bsky.app/profile/alpenglow.bsky.social) may save me from my hubris and design a better acrylic stand than this, and offer them pre cut in a bunch of different mix and match colors.

### For people who shoot melted plastic at stuff

Originally, I was using this [nice 3D printable stand](https://www.printables.com/model/684090-stand-for-the-jumperless-breadboard) designed by someone in the community. It works great, but after designing the acrylic stand, I felt the 3D printed version needed some *pizzazz* to match the overall vibe.

![](/images/UpdateImages/08-post-campaign-images/3DStandCollage.jpg)

![](/images/UpdateImages/08-post-campaign-images/draftPrint-6.jpg)

As soon as I showed that to people, they brought up something that seems stupidly obvious in retrospect, why didn't I add a slot to hold the probe? So I did.

![](/images/UpdateImages/08-post-campaign-images/draftPrint-07.jpg)

![](/images/UpdateImages/08-post-campaign-images/draftPrint-09.jpg)

You can get both versions to print yourself [here on Printables](https://www.printables.com/model/1249365-jumperless-stand) (these will work for the OG Jumperless too, I added a notch to accommodate the slightly wider OG probe.)

You can also put some of [these self adhesive 8 x 2.5mm rubber feet](https://www.amazon.com/Self-Adhesive-Semicircle-Furniture-Reduction-Transparent/dp/B08R41K7M6/) on the bottom, they're the exact same ones on the Jumperless itself so you'll have extras around if you feel like doing any soldering on your Jumperless V5 and want to take them off.

![](/images/UpdateImages/08-post-campaign-images/draftPrint-1.jpg)


### Finally tested with an actual Raspberry Pi

I've had these SBCSMDOLED adapter boards sitting around for a while and it hadn't crossed my mind to actually test them with a Raspberry Pi. And yep, it works.

![](/images/UpdateImages/08-post-campaign-images/RaspberryJ-2.jpg)

![](/images/UpdateImages/08-post-campaign-images/RaspberryJ-1.jpg)

I hooked the Jumperless's internal UART up to the Tx Rx pins on the Pi, so what you're seeing here is me sending typos through the second serial port that pops up when you plug in a Jumperless and being received on the Raspberry Pi. You can also do this in the other direction and have the Pi send commands to make connections or whatever.

What's cool is there's a little solder jumper on these adapters that you can bridge to connect the 5V busses together and power your Jumperless from the Raspberry Pi. This is kinda why I didn't want to mess with USB C Power Delivery or anything, it's kinda cool to just be able to shove in 5V from wherever, as long as it can provide 500-1000mA.

![](/images/UpdateImages/08-post-campaign-images/SMDSBCOLED.jpg)

### DIY shapes

If you want to take a swing at designing your own stand or something, here are the dimensions (in mm):

![](/images/UpdateImages/08-post-campaign-images/JumperlessV5dimensions.png)

Plus a bonus exploded view:

![](/images/UpdateImages/08-post-campaign-images/Explodraft.jpeg)

Or let your computer handle the measuring and just use [the 3D model](https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/Jumperless23V50/Footprints%20and%20Symbols/3Dmodels/JumperlessAndProbe.3mf).

## Outerwear

### For Jumperless

For the first 100 or so boxes for the very first run of the OG Jumperless, I went through some US-based website that didn't offer custom sizes, took six weeks, printed in low resolution on the jankiest cardboard imaginable, didn't crease them enough so I had to fold them with a ruler, and all for the low, low price of ***$7.50 per box!*** To get them to my standard of presentable, I ended up cutting a stencil and painting over the pink parts with glitter, much to the chagrin of some customers who don't love glitter as much as I do (genuinely sorry about your hot chocolate!)

![](/images/UpdateImages/08-post-campaign-images/glitterbomb.png)


So then I decided to have [Zhibang Packaging](https://www.zhibangpackaging.com/) make the boxes for the OG Jumperless, and they were **amazing**, custom sized, ***lush*** cardboard, pink foil printing (instead of glitter), microscopically high resolution, and like 1/5th the price. So much that I got the sense that I was asking too little of them. So of course now that I know what they were capable of the last time around, I *have* to push it further and make a cooler box. So, inspired by the [Solder Ninja](https://www.crowdsupply.com/sitron-labs/solder-ninja-pen)'s excellent packaging, I had them make a magnetically clasped gift box with an EVA foam insert.

![](/images/UpdateImages/08-post-campaign-images/BoxCollage.jpg)

Fun fact: that OG Jumperless, white cable, and probe PCB you see in that bottom left photo, I didn't arrange that. A really cool benefit to having solid relationships with your suppliers is stuff like that, they went out of their way to coordinate with Elecrow and picked up samples from them to check the fit on the foam insert I sent them (okay their factories are like 3 blocks away from each other but still, that was super nice of them.)

[![Video](https://img.youtube.com/vi/O2n4sLUqOEU/maxresdefault.jpg)](https://www.youtube.com/watch?v=O2n4sLUqOEU)
https://www.youtube.com/watch?v=O2n4sLUqOEU

(Sound on to hear what it sounds like inside a box factory)


What I suspect is happening here is that the US customer-facing websites are effectively drop shipping. Using basically the same factories in China but telling them to use the absolute cheapest options to pay them \$0.50 per box so they can keep ~$7 per box for themselves to run a janky-ass website and be on the first page of Google search results.

There you go, that's as close as I'm gonna get to mentioning anything remotely related to tariffs in this update.


### For humans

One thing they don't tell you about having any sort of merch is that you end up *only* wearing shirts with our own logo on them. It started to feel weird to be wearing the same shirt every day for a year and a half, so I decided to bump that up to wearing the same 3 shirts for the foreseeable future.

![](/images/UpdateImages/08-post-campaign-images/ShirtModelCollage.jpg)

The crop tops and 3/4 sleeve shirts were a special request and I only had a few made. But I made a *ton* in men's V neck, men's crew, women's V neck, and some running shorts. 

*In the bottom right: my mom's a painter and needed to do a random panel for something and asked me what she should paint, so I jokingly said she should do my logo. Now I have this awesome glittery painting of [People's Park Complex](https://maps.app.goo.gl/abW7xNt2gUUMDLbp8) on my wall*

Electronics pro tip: have a [friend from high school](https://www.instagram.com/chrispowers_chrispowers/) open a printing business who will close their shop for two days while you hang out and watch them make your stuff.

[![Video](https://img.youtube.com/vi/dvKw246zul0/0.jpg)](https://www.youtube.com/watch?v=dvKw246zul0)

https://www.youtube.com/watch?v=dvKw246zul0

[![Video](https://img.youtube.com/vi/h5wWYyeBhdM/0.jpg)](https://www.youtube.com/watch?v=h5wWYyeBhdM)

https://www.youtube.com/watch?v=h5wWYyeBhdM


We also did some R&D and figured out you can mix red, green, and blue glow in the dark pigment to get a white glow and mix it into clear plastisol ink, then do a top layer with that. So yeah, these also glow in roughly full color.

![](/images/UpdateImages/08-post-campaign-images/ShirtLight.jpg)

I always thought it was lame to sell stuff that's also an ad, so these shirts are free. How you get one is by going to [Teardown 2025](https://www.crowdsupply.com/teardown/portland-2025) where I will be handing them out. So if you didn't have enough reasons to go already, maybe a shirt (or running shorts) will change your mind.


## The lengths I will go to match the design language

The low profile, right angle, TRRS cables you see in all these photos have the ends [hand painted and dipped in glitter](https://hackaday.io/project/191238-jumperless/log/223752-any-sufficiently-extra-technology-is-indistinguishable-from-a-barbie-oppenheimer-crossover-meme-glittering-usb-cables). The prospect of doing that a thousand times wasn't *completely* out of the question, but I knew after the first 100 or so I'd be questioning my life choices. So I put out an RFQ on Alibaba to have them made by someone with better equipment than cans of spray paint and **The Glitter Pile**

[![Video](https://img.youtube.com/vi/bh_8oB4UwfM/0.jpg)](https://www.youtube.com/watch?v=bh_8oB4UwfM)

https://www.youtube.com/watch?v=bh_8oB4UwfM

![](/images/UpdateImages/08-post-campaign-images/TRRRS-RFQ-1.png)

Turns out [the company](https://googfit.en.alibaba.com/company_profile.html?spm=a2700.shop_index.88.64.662b353b33uhI4) I was buying the cables to paint responded, and they told me the molds they used to make these are "used up", but they'd be happy to make a new injection mold and do them in pink plastic for me. 

Here's the hand mixed pink plastic swatch they sent:

![](/images/UpdateImages/08-post-campaign-images/PlasticSample.jpeg)

Hell yeah.

If they're making a custom injection mold for this, it turns out I can make these any shape I want. So I fired up Fusion 360 and made these:

![](/images/UpdateImages/08-post-campaign-images/CableCollage.jpg)

I switched back to TRRS because the third ring wasn't strictly necessary and the back end of the plug is a bit too long to make these as low profile as I would like. This is what they should look like *in vivo*:

![](/images/UpdateImages/08-post-campaign-images/CableRender-6.jpg)


The lead times are a bit long (6-8 weeks, this was like 5 weeks ago), and I wasn't about to hold up shipping on account of the cables, so I had them send me their existing inventory of 100 black ones to hand paint myself. So first 100 or so units will probably ship with the glitter, and then they'll start coming with these pink plastic ones when they're ready.



## Write it yo'self

To save myself from having to write every function anyone would ever want by myself, Jumperless V5 allows you to write your own apps. So I wrote you all a [commented example app](https://github.com/Architeuthis-Flux/JumperlessV5/blob/85db97e93b55011b320fc376792cfa02afb65a18/RP23V50firmware/src/Apps.cpp#L140) that should give you a sense of how to use all the functions you'll likely need to do anything. There's also a [setup guide](https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/RP23V50firmware) in the repo to show you how to add it to the app library so you can run it from the menus.

![](/images/UpdateImages/08-post-campaign-images/customApp-first.png)

As you can see, this isn't *really* an "app", it's just a function in [Apps.cpp](https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/RP23V50firmware/src/Apps.cpp). I did it this way so you can use *literally any function* in the Jumperless V5's firmware, the ones shown are mostly the helper functions I've written to make my life easier, but there's no guard rails here, you have complete control of the hardware.

It's all written in Earle Philhower's [arduino-pico](https://arduino-pico.readthedocs.io/en/latest/index.html), so if you've written code for any Arduino-like thing, the syntax should be familiar.

If you don't feel like looking at the code on Github, I'll go through a few snippets from the example app here to maybe give you some ideas for things you'd like to do with it.

### Measuring current
![](/images/UpdateImages/08-post-campaign-images/customApp-current.png)

### Showing text and images on the breadboard
![](/images/UpdateImages/08-post-campaign-images/customApp-printing.png)
You can also feed it arbitrary images (after running them through [image2cpp](https://javl.github.io/image2cpp/)) and it will work as a 14x30 display.

### Getting row input from the probe
![](/images/UpdateImages/08-post-campaign-images/customApp-probeTap.png)
I might make this whole bit into its own function so you can just tell it to wait until you tap a row with a single line.

### Measuring voltage / current with the probe tip
![](/images/UpdateImages/08-post-campaign-images/customApp-probeMeasure.png)
Make sure you set the switch on the probe to the `measure` position.

### Rawdogging it
![](/images/UpdateImages/08-post-campaign-images/customApp-raw.png)
If you want to bypass all of the routing stuff entirely, you can also just send XY connections to the crossbar switches directly. 

There's also more stuff I didn't include here. If you write an app you're happy with, submit a pull request and I'll add it to the default app library so everyone can use it.

## Apps (and not apps) that actually do something useful

### Scan

[![Video](https://img.youtube.com/vi/BxFaah4Uk7w/maxresdefault.jpg)](https://www.youtube.com/watch?v=BxFaah4Uk7w)
https://www.youtube.com/watch?v=BxFaah4Uk7w

This was kinda the old example app, it scans through each row connecting them to an ADC, if it measures ~0V, it'll connect a GPIO to that row and wiggle the pullup resistors so it can tell the difference between a row that isn't connected to anything and a row tied to GND.

This is the absolute slowest way to do it by writing to the filesystem every time, but it's meant to be an example app showing how to make connections and "properly". If you hard-code the routing, it can scan the whole board ~300 times per second (that's how the probe was sensed on the OG Jumperless.)

### DAC calibration

[![Video](https://img.youtube.com/vi/EthICnDbT1k/maxresdefault.jpg)](https://www.youtube.com/watch?v=EthICnDbT1k)
https://www.youtube.com/watch?v=EthICnDbT1k

Because all the DACs and ADCs are scaled to +-8V with op amps, the precision of the feedback resistors kind of determines how well the readings match the real voltage on those. But as it turns out, the [INA219 current/power monitors](https://www.ti.com/lit/ds/symlink/ina219.pdf?ts=1745204710113) have a pretty good internal voltage reference. So this app sets a bunch of voltages and calibrates the scaling against those. It saves everything to EEPROM so you should only have to run this once.

### Real Time ADC Display (feat. A Cool Encoder I Found On LCSC)

[![Video](https://img.youtube.com/vi/Gtyf7BvZwmc/maxresdefault.jpg)](https://www.youtube.com/watch?v=Gtyf7BvZwmc)
https://www.youtube.com/watch?v=Gtyf7BvZwmc

This is more of a core function that's been around forever, but I have a nice video of it so I'm showing it here. This also works with the DACs and any of the 10 routable GPIO, and it doesn't matter whether they're set as inputs or outputs.

### Path Stacking

[![Video](https://img.youtube.com/vi/1rEojT77RpI/maxresdefault.jpg)](https://www.youtube.com/watch?v=1rEojT77RpI)
https://www.youtube.com/watch?v=1rEojT77RpI

This one is awesome, it's a new setting on Jumperless V5 that automatically fills all unused paths with redundant copies of existing paths to get a lower connection resistance. If you want more control over which paths want low resistance, you can turn it off and just make the same connection multiple times (which does the same thing but makes sure there are more free paths available for the ones you care about.)

## Where the fuck is my Jumperless?!

### Getting a PCB fab to throw away 800 boards

Remember in the [previous update](https://www.crowdsupply.com/architeuthis-flux/jumperless-v5/updates/good-news-everyone-things-are-happening) where the PCB fab Elecrow was using had to scrap the first sample run of 5 and remake them? Well, after my project manager, Chris, and I talked it over, we decided to believe them when they insisted that they could make them. The 5 samples came out great, but apparently they couldn't pull it off at scale and had to throw away ***the entire run of 800 PCBs***, brutal. At least they told us about it pretty early this time so it didn't push back the schedule too much.

A fun bonus from all that is I got them to send me a few of the bad boards so I could do this:
![](/images/UpdateImages/08-post-campaign-images/LayersCollage.jpg)

### New fab, who dis?

So we went to the other fab that Elecrow uses for their high-spec PCBs, and asked them to hurry. 

And they did an excellent job:
![](/images/UpdateImages/08-post-campaign-images/ProductionCollage.jpg)

### Soooooon

As of the time I'm writing this (April 21st), I am happy to report that they have 150 boards fully assembled, flashed, and first-pass tested. As well as another 150 boards with the just SMD stuff populated.

This is the quick testing procedure I'm having them run. I've found that if the click wheel and power supply works on these, 99.9% of the time, everything else is gonna work too (I'll do the rest of the testing myself, this is just to check for major issues in production.)

[![Video](https://img.youtube.com/vi/4XLGgrQFE7A/maxresdefault.jpg)](https://www.youtube.com/watch?v=4XLGgrQFE7A)
https://www.youtube.com/watch?v=4XLGgrQFE7A

So they should be shipping the first batch out to me this week. When they arrive at my house, I will glue on the click wheel caps, put them through some more thorough tests, polish any rough edges, give them their glittery probe cables, read them a bedtime story, then put them all in a box to ship to Mouser's distribution center to be shipped to you.

There's no way to tell how long it will take to move through Mouser's system and get to your doorstep, but it won't be long now. 

___


If you have any questions about anything or just want to get a more real-time stream of updates about how it's all coming along, come chat with me and the community on the [Jumperless Discord](https://discord.gg/bvacV7r3FP), [Forums](https://forum.jumperless.org/), [GitHub](https://github.com/Architeuthis-Flux/JumperlessV5), [Twitter/X](https://x.com/arabidsquid), [Bluesky](https://bsky.app/profile/architeuthisflux.bsky.social), or [Mastodon](https://leds.social/@ArchiteuthisFlux).

Love,  
Kevin



[![Video](https://img.youtube.com/vi/1g_xCEyMQhM/maxresdefault.jpg)](https://www.youtube.com/watch?v=1g_xCEyMQhM)

https://www.youtube.com/watch?v=1g_xCEyMQhM
