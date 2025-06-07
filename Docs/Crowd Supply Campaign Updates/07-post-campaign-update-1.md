# Good News Everyone! Things Are Happening

First of all, sorry I haven't sent one of these updates in a while. Too many cool things have been happening that writing about it got pushed to the bottom of the list. So now I get to tell you about all the extra bonus stuff you didn't even know you were getting when you backed Jumperless V5. 

## No More Naked Click Wheels!

I was putting in an order for some new breadboard shells at JLC3DP, when I figured I should maybe see if putting a cap over the [SIQ-02FVS3](https://www.lcsc.com/product-detail/Rotary-Encoders_Mitsumi-Electric-SIQ-02FVS3_C2925423.html) rotary encoder / switch would look interesting. I really didn't intend to make it an included thing, just a "here are the files if you want to print one yourself" type deal. So I got them printed in a bunch of different materials to see which ones look the best. 

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-2.jpg)

Meh... No real stand outs (the SLM steel ended up being really cool after polishing though, you'll see it out of focus in a later image.) 

At some point I figured I'd give JLC's [full-color resin WJP printing](https://jlc3dp.com/help/article/full-color-resin) a try, with fairly low expectations. *Surely* something in full color *must* sacrifice some resolution, right?

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-5.jpg)
**Wrong**

It honestly looked ***too*** good, because once I stuck one on a Jumperless V5, they look like unfinished garbage without them. 

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-4.jpg)

There was no going back, I wouldn't be able to live with myself shipping Jumperlesses with ***naked click wheels!*** (gasp) 

#### The Battle

The issue is that these caps are really tiny, and there's a cost floor of (hilariously) $6.66 per WJP print until you get into the regime of paying for the volume of material, okay fine, I'll just stick a bunch together on a frame and separate them myself.

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-6.png)

I'm sure they have a good reason for this, but (seemingly) arbitrary rules getting in the way of me making stuff flips some switch in my brain where I become hyper-fixated on coming up with a workaround. Which led to me submitting a series of silly shapes in the hopes that one of them wouldn't be recognized as multiple parts.

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-7.png)
What? It's a model of a coronavirus (this didn't work).

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-8.png)
SinglePieceMonolithicDieForBoardGame.3mf

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-9.jpg)

This one actually did sneak by their patrols. But they were a bit of pain to separate and the ones printed vertically had some weird artifacts around the black parts. Still, it felt like a win.

#### This Update Is Sponsored By [JLC3DP](https://jlc3dp.com/)

While talking about all this on [Twitter](https://x.com/arabidsquid) (I promise I wasn't complaining), JLC3DP reached out and graciously offered to make an exception to the "no connected parts" rule for me and a sponsorship. In return, I'm supposed to mention them in a post, which is what you're reading now. But I would have probably said all this anyway, but they do *excellent* work. They're the only ones who seem to be able to pull off the ultra thin 0.48mm separations on these SLS nylon breadboard shells. And also are the only people I know of offering WJP full-color resin. Which is kinda crazy how few prints I've seen using it, you should seriously try it out. <\ad>

Anyway, I uploaded them in 7x7 sheets, fully resigned to future me's couple day ordeal removing the frames and cleaning them up. Then I get an email.

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-11.png)
omg yes. And they did a really nice job with it.


![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-13.jpg)
Wake up babe, new vaporwave apocalypse currency just dropped

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-12.jpg)

Fun little bonus pic, I made this render so see if JLC3DP would want me to put their logo on my breadboard shells in exchange for absolutely cranking every possible quality setting on their SLS nylon printers (they already come out nearly perfect, I just wanted to see if it was possible to push it further.) They came back and told me they already do that for these prints in particular because of the super thin features, but they'd be happy to push the quality settings even further without needing to put ads on my stuff. Which is a relief, because it felt kinda gauche. But I liked how the render turned out so I'm showing you here.

![](/images/UpdateImages/07-post-campaign-images/click-wheel-cap-14.jpg)

## The Bus Pirate/QWIIC/analog/whatever Adapter Board

Turns out, the Jumperless community has a ton of overlap with the [Bus Pirate](https://buspirate.com/) community. Which makes sense, we kinda bring a similar vibe with the stuff we make which seems to attract a similar group of cool nerds. 

So when it came up on the [Bus Pirate Forum](https://forum.buspirate.com/t/jumperless-buspirate/769) that there should be a nice way to plug one into a Jumperless, I made this:

![](/images/UpdateImages/07-post-campaign-images/SBCBP-1.jpg)

![](/images/UpdateImages/07-post-campaign-images/SBCBP-2.jpg)

And here are some totally normal adult human diagrams explaining how it connects:

![](/images/UpdateImages/07-post-campaign-images/SBCBP-3.png)

![](/images/UpdateImages/07-post-campaign-images/SBCBP-4.png)

It also has 2 STEMMA QT / QWIIC ports with selectable 3.3V or 5V power. You can hook the I2C lines to any of the routeable GPIO with that weird, wiggly footprint, or even connect each one to a different pair of GPIO by cutting it somewhere in the middle.

You may be wondering where that FPC cable connects to the Jumperless. Here's another new thing, a 20 pin 0.5mm pitch FPC connector that breaks out all 10 routable GPIO, the first 4 ADCs, and the 2 DACs, plus power and ground. GPIO (or RP) 6 and 7 are only connected to the RP2350B for sending commands to/from the Jumperless. 

Elecrow is currently in the final hand-assembly phase of building this latest round of pre-production prototypes, so I don't have a real picture of them. So take this stylized image from the packaging instead (which I've photoshopped to be laid out pretty close to the boards you'll be receiving.)

![](/images/UpdateImages/07-post-campaign-images/SBCBP-5.jpg)


I never really had a good plan for how daisy chaining was going to physically connect until now. *I* would just solder them together, but I know most people wouldn't be as comfortable soldering directly to something like this without a pile of backup tester boards. 

![](/images/UpdateImages/07-post-campaign-images/SBCBP-6.png)


These will also be included in the box, along with anything else I mention here. I'm not really into the "upselling accessories" thing, I'd prefer everyone has everything they might need right off the bat.


>[!NOTE]
>The eagle-eyed among you may have noticed the little "tongue" sticking out from the bottom of the LED wishbone board that wasn't there in previous revisions. That's a consequence of making hardware without an enclosure. If you pick this thing up in such a way that your finger is touching the crystal (the silver rectangle), the added capacitance will cause the RP2350B's clock to stop, so it'll freeze up until you yank the power and plug it back in. Not a huge deal because everything is persistent so it just starts back up where you were, but it was mildly annoying and also really easy to fix.
>![](/images/UpdateImages/07-post-campaign-images/SBCBP-11.png)


## SBCSMDOLED (Single Board Computer / Surface Mount Device / Organic Light Emitting Diode) Adapter Board

I talked about these in [my first update](https://www.crowdsupply.com/architeuthis-flux/jumperless-v5/updates/single-board-computer-fans-rejoice), but now they've arrived so I have actual pictures instead of renders. The gist of that update is since I'm including this board anyway, I tried to make it cover a pretty broad range of things that might be useful. 


![](/images/UpdateImages/07-post-campaign-images/SBCBP-7.jpg)
Yes, the [Raspberry Pi's 40 pin GPIO port](https://www.raspberrypi-spy.co.uk/2012/06/simple-guide-to-the-rpi-gpio-header-and-pins/) has some very silly pin numbering, so I had to kinda follow that. Also note that the SMD pads with the blobby extensions aren't connected to anything, I'm really shoving a square peg in an Arduino Nano shaped hole here, so some weirdness was inevitable. 

![](/images/UpdateImages/07-post-campaign-images/SBCBP-10.jpg)

![](/images/UpdateImages/07-post-campaign-images/SBCBP-9.jpg)

This also lets you friction-fit a little [0.91 inch 32x128 I2C OLED module](https://www.amazon.com/dp/B07D9H83R4/?tag=clomads27-20) onto it, which can be controlled by the Jumperless. Eventually there will be some UI for it as an extension to the breadboard LEDs, but it'll require some time to write that firmware (or better yet, someone in the community taking initiative on), so don't expect that to be a working thing when you get your Jumperless V5. But it's on the list, so support will come eventually.

But hey, the hardware's there, we just need to write the firmware, we're halfway done!1!! (joke)


## Hapax LEGOmenon

I had mused about this in an earlier update, and I finally went for it. I put holes on both sides of the breadboard shell to accept Legos and technic axles. In case you want to make a stand or something with that. Very much a "why not?" feature I hope someone will do something awesome with. Note: both sides are holes, there's nothing sticking out on the other side.

![](/images/UpdateImages/07-post-campaign-images/lego-2.jpg)

![](/images/UpdateImages/07-post-campaign-images/lego-3.jpg)

And they hold pretty snugly (apparently Lego themselves use SLS nylon to prototype bricks, which is good to know)

![](/images/UpdateImages/07-post-campaign-images/lego-1.jpg)


If you're wondering about those other 3 holes on the sides, those are for cooling. 445 addressable RGBs on a small board like this can pump out some heat, so the idea is to use the spring clips as cooling fins and allow air to passively flow in from the side and through the breadboard holes.

Here's a POV of an air molecule flowing through an (old revision) Jumperless V5.

https://www.youtube.com/watch?v=Va9jjmDZ5J8

The heat's not as much of an issue now that the crossbar matrix supply is tuned to a voltage they can better handle (still way outside of spec, but a bit less so), but these things still do get a bit warm in normal use. Nothing to worry about, just letting you know. But if it's getting like, ***hot***, let me know because something's wrong and I'll troubleshoot / fix / replace it for free.

## Shipped Inside A Monolith

Speaking of *2001: A Space Odyssey* references, the packaging isn't totally finalized yet, but they'll be coming in a magnetic closure gift box that's the same dimensions as the monolith, 1² x 2² x 3² (1" x 4" x 9", in inches because aliens use freedom units.) In case you have extremely particular storage needs and care about the dimensions.

![](/images/UpdateImages/07-post-campaign-images/box-1.png)

Yeah, I do *way* too many of these silly edits. 

![](/images/UpdateImages/07-post-campaign-images/box-2.jpg)

This was actually by very first hand-stencilled box design for Jumperless back when it was a totally different shape and I had just changed the name from [breadWare](https://hackaday.io/project/180394-breadware).

How it started:
![](/images/UpdateImages/07-post-campaign-images/box-4.png)

How it's going:
![](/images/UpdateImages/07-post-campaign-images/box-5.png)

If you're wondering why it looks all pixelated like that, it's because this box is meant to be printed in [CMYK(+W) Process Color](https://en.wikipedia.org/wiki/CMYK_color_model). Normally they'd just do the color halftone split for you, but my art school background (and general printmaking snobbery) wouldn't allow me to leave that up to someone else. So I kinda leaned into the look that comes from the limitations of printing this way.


## Schedule (but pronounced like those people who skip the hard 'c' and say shed-yool)

The original plan was to slide in *right* before [Chinese New Year](https://en.wikipedia.org/wiki/Chinese_New_Year) and at least have a good portion of backer orders shipped to me by the time it starts on January 29th. And I probably could've pulled that off, if I hadn't decided to fix a bunch of hardware stuff that I felt could be better:
- adding the FPC connector
- tuning the ±9V power supply to run the crossbars at the point of lowest on resistance and highest safety margin
- splitting the LEDs into 2 separate chains for a higher frame rate
- tightening overcurrent protection on the crossbar matrix
- putting some backpower protection on the Arduino Nano's 5V and 3.3V supply pins
- rerouting the whole thing from scratch to give tracks more clearance / lower noise / better grounding
- adding *even more* 3-way solder jumpers for different hardware configuration options
- more stuff like that I can't remember right now

I was fairly confident I didn't screw anything up *too* badly in the process, I've made a lot of similar tweaks over the years on Jumperless and they usually come out fine, as long as I'm patient and spend an entire day staring at it after I think it's finished, which I did here. So I was kinda willing to YOLO the new revision and have 100 made before CNY without testing beforehand. Worst case, I'd spend a week hunched over bodging them as penance for my hubris. 

So I put it up to a vote in the [Jumperless Discord](https://discord.com/channels/1174442669259378778/1312076012611440701), thinking there'd be *some* impatient people like me willing to risk it to get their Jumperless V5 a month sooner. 

![](/images/UpdateImages/07-post-campaign-images/schedule-1.png)

Nope, it was unanimous in favor of me getting 5 pre-production proofs made before committing to a run. The people have spoken! Honestly it's a huge relief that people don't seem to be nearly as worried about the tight deadline I set for myself.

I'm not really sure about the small run of 50 happening before CNY. There was an issue with the PCB fab needing to scrap 4 out of 5 main boards due to shorts and needing to make them again, which isn't really a knock on them, this is a *tight* 4 layer board, and I'm pushing their board constraints a bit further than they advertise. 

![](/images/UpdateImages/07-post-campaign-images/schedule-2.png)

The good news is when something like this happens, they're usually super extra careful on future runs. This happened on the spring clips for the OG Jumperless, there was a batch made to the wrong tolerance once (which they of course offered to make again for free, but I got them to let me pay for materials so they're not losing money on some random low-volume customer), and ever since they've been noticeably *perfect*, like, unnecessarily so. Pro tip: be nice to the people making your stuff.

![](/images/UpdateImages/07-post-campaign-images/schedule-3.jpg)

### The Part Where I Get To The Point


1. 5 pre-production proofs will be getting to me sometime next week (all the parts for a full run of 500 have already been sourced.)
2. I test them to make sure I didn't screw anything up (and fix anything I did).
3. JumperlessV5_MainBoard_Final_FINAL_ForRealThisTimeActuallyFINAL_OKAYREALLYFINALMAKE500OFTHESE_Corrected.zip goes to [Elecrow](https://www.elecrow.com/).
4. They'll have a little bit of time to look at it and make some preparations for when they get back
5. Chinese New Year happens, nominally it's January 29th - February 17th this year, but in practice it seems to take an extra week or two to fully spin up again, so assume it ends March 1st.
6. While China is taking their well-deserved vacation:
    - Boxes will be made and arrive sometime around the end of February.
    - I will paint and glitter the probe cables (yes I do these by hand one at a time, none of the samples I had made look right.)
7. Elecrow starts making a run of 500 Jumperless V5s.
   - SMD parts are usually assembled in one big pass, but there's quite a bit of hand work: putting spring clips into the shells, mounting the LED wishbone and probe sense boards, soldering the probe needle and screwing those 2 boards together, etc. so I'll have them ship as they finish batches of 50-100. The first batches should start being ready to ship to me around the ides of March.
8. Boxes of assembled Jumperless V5s start showing up at my house (which is more factory than house at this point), I attach the click wheel caps, plug in the probes, thoroughly test each board, and lovingly pack each one into their boxes. Which then go into a bigger box to ship to Mouser, around mid-late March.
9. Mouser gets them, does their Mouser thing, and ships them to you (I have no idea how long this takes)

**TL;DR** 
![](/images/UpdateImages/07-post-campaign-images/april-ludgate.gif)
April

Of course that's a guess, but it *feels* reasonable enough. There haven't been any serious unexpected delays \*\*knock on wood\*\*, just my insistence that I should probably do one more round of shaking out potential hardware bugs *before* these ship. 

But yeah, I'm in a huge hurry for these to get to you, because I can't wait to hear about the looks on all your faces when you open up your shiny new Jumperless V5s and start playing with them. Every time I pick up one of these prototypes after not working with the hardware for a few days, I get reminded how much freaking ***fun*** these things are. Of course I'm totally biased because it's my baby, but I am **so** excited for all of you to experience it for yourselves.


If you have any questions about anything or just want to get a more real-time stream of updates about how it's all coming along, come chat with me and the community on the [Jumperless Discord](https://discord.gg/bvacV7r3FP), [Forums](https://forum.jumperless.org/), [GitHub](https://github.com/Architeuthis-Flux/JumperlessV5), [Twitter/X](https://x.com/arabidsquid), [Bluesky](https://bsky.app/profile/architeuthisflux.bsky.social), or [Mastodon](https://leds.social/@ArchiteuthisFlux).

Love,  
Kevin