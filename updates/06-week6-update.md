# *A Farewell To Campaigns* (Soon)

##### They're finally almost over, and I couldn't be happier\* to see them *all* end. 

## What Does This All Mean?

For you, dear reader/potential backer, it actually doesn't make *too* big of a difference whether you order your [Jumperless V5](https://www.crowdsupply.com/architeuthis-flux/jumperless-v5) before or after crowdfunding ends. One of the many awesome things about doing this with Crowd Supply is that they also order a bunch of units to keep in stock. So the moment after the campaign ends on Thursday, November 7th, you'll still be able to order one, the only difference is the price will be $369 instead of the special crowdfunding price of $349. I don't think any of us need an *additional* source of urgency in our lives right now (especially us Americans, given the timing).


For me, that means I get to start ramping up for full-scale production pretty soon. The goal is to get first batch (which should be enough to cover all the campaign backers' orders) assembled and shipped to me before Chinese New Year. That's not some arbitrary goal post, CNY / Spring Festival is a (well-earned) holiday that starts on January 29th next year and effectively means nothing is happening in China for the entire month of February.

It's a fairly tight timeline for the number of different manufacturers involved, so to give me some wiggle room to have a last round of final-production prototypes made, I went to [Elecrow](https://www.elecrow.com/) and said "here is *all* the money I have until the campaign ends, is this enough to source the parts before I send you finalized design?" and they were like (I'm paraphrasing here, in AI Jeff Geerling's voice) "naw dawg, but it's cool homie, we gotchu fam." Elecrow's been an amazing CM for both versions of Jumperless, and it's really nice to know they have my back doing whatever it takes to get them made to my absurdly picky standards *and* shipping on time.

## Move Fast And Fix Things

Thus far I've kind of avoided talking too much about things I haven't fully implemented yet. But this is the last update before the campaign ends so I should probably talk about some of the features that will be finished by the time you get your Jumperless V5, and a few that are planned to come out in a future firmware update. These are listed roughly in the order they'll be worked on, but all but the last one should be ready by the time you get yours.


### La Résistance

Analog crossbar switches have some resistance, roughly totaling to 85Ω after the two hops though a crossbar needed to make a connection. And if all the direct paths are taken, it'll need 4 hops to go through an intermediate chip, so 170Ω. 

For signals, this has a negligible effect (if no current is flowing, the voltage drop is zero), but for connections used to power anything that draws a good deal of current, this sucks, and I wish it were 0Ω.

I had just kinda accepted this as a limitation of physics / economics / not making this thing take up an entire desk. Like, if I wanted to use relays or some other discrete part, I would need (12 x 128 =) 1,536 individual switches.

But wait, unless you're trying to connect every breadboard row to every other, most of the available connections are unused, just chillin'. And adding resistors in parallel *decreases* the total resistance by
$$R_P=\left(\frac{1}{R_1}+\frac{1}{R_2}...+\frac{1}{R_n}\right)^{-1}$$ So putting more connections in parallel gives us 
$$ \left(\frac{1}{85Ω}+\frac{1}{85Ω}\right)^{-1} = 42.5Ω \qquad \qquad \qquad\left(\frac{1}{85Ω}+\frac{1}{85Ω}+\frac{1}{85Ω}\right)^{-1} = 28.3Ω\qquad \qquad \qquad\left(\frac{1}{85Ω}+\frac{1}{85Ω}+\frac{1}{85Ω}+\frac{1}{85Ω}+\frac{1}{85Ω}+\frac{1}{85Ω}\right)^{-1} = 14.1Ω $$   



![](/images/UpdateImages/06-week6-images/parallel.jpeg)


So once the Jumperless finds a path for all the connections it was asked to make, we can have it go through again and fill up all the unused paths with redundant parallel connections to bring the resistance *way* down. You'll be able to set a higher priority for connections that need to carry more current, probably with a long press on the probe button, but in general it'll just do this without you having to think about it.

Aha! Using physics to beat physics!


![](/images/UpdateImages/06-week6-images/you-played-yourself.png) 



Some firmware ideas demand to be written immediately after thinking of them, this one came up on the 4 hour drive home from Supercon, so I turned off the Philip K. Dick audiobook I was listening to and started working out how I was going to pull this off in my head. As soon as I finish writing this campaign update, I'm starting this firmware update.


### A Menu Option We Can Blame On Benjamin Franklin

So far, the INA219 current sensors haven't really gotten the love they deserve, but that's going to change soon. I have been using them to detect the switch position on the probe by determining which of the two power sources are drawing the current needed to drive the LEDs (it swaps power sources when you switch from Select to Measure to save pins to get it down to four connections in a TRRS cable, it's a whole thing), but there's a lot more they can do.

You know those little "ants" they put on wires in simulations to show the flow of current?


https://github.com/user-attachments/assets/a9aa8aad-be34-4f81-8a02-d770dbb77e69


*This is a [transistor-level simulation of a CMOS analog switch](https://tinyurl.com/ym8awywx) if you're curious about what's going on inside a Jumperless*

Now you can have those on your breadboard too! There's just one routeable as a current sensor (the other is for measuring resistance / current draw from the DACs), so you'll be able to poke around with the probe and it'll show you the current between any 2 places.

Another funny thing is that means I should put a menu option for which current flow convention to use. When Benjamin Franklin theorized that electric charge was the presence or absence of *something* he knew there wasn't really a way to tell whether the rabbit fur or the glass rod was the one *gaining* the something, so he just picked one. To the joy of future textbook publishers everywhere, [he guessed wrong](https://electronics.stackexchange.com/a/36119).

![](/images/UpdateImages/06-week6-images/textbook-flow.png)

So there will be a setting that does something like this:


https://github.com/user-attachments/assets/0efb8998-ecd1-4ce5-9279-56501e711c7c




### Probe Fu

Watching people use Jumperless V5 at Supercon gave me some new ideas for cool things you can do with the probe. Having the tip be arbitrarily routeable to anything means it can really do *anything* we can think come up with.

Here's a couple that I think will be super useful and likely added before they ship:

- Using it as a logic probe. It already has 3 LEDs, so it can use them to show Floating, Low, or High. The colors I happened to pick are perfect, white in the middle for floating, pink at the top for high, and cyan at the bottom for low.

- A nicer interface for toggling any of GPIO pins, just tap any net that has a GPIO connected, and the Connect (front) button will drive it low, and Remove will drive it high (I'm imagining the probe pointed downwards at the board so it's top / bottom for high / low.)

![](/images/UpdateImages/06-week6-images/newProbe5.jpg)

I'll definitely think of more, but if you have a good idea, come drop it in the [Jumperless Discord](https://discord.gg/nMyHzJnMEw).


### Unleashing Your Inner User Manual

I'm one of those weirdos that usually at least takes a quick look at the user manuals that come with things, but I don't like when they're *necessary* to use it at all. Jumperless will come with some printed "Getting Started" guide to get you up to speed on using the probe, getting into menus, setting up the app, etc. But for things beyond the basics, development on the Jumperless firmware moves *fast*, I need docs that are as software-configurable and "liquid" as the Jumperless itself. The lazy way is just to be like a [restaurant](https://www.youtube.com/watch?v=dytFj5Iw208) and rely on a QR code that takes you to the online docs. That will definitely be a thing, but I also want something that *feels* more natural.

So there will be step-by-step guides that will display on the breadboard LEDs to walk you through how to do things. As well as help pages for each option over the serial terminal. What's nice is they'll just be part of the firmware, so new versions can always stay in sync with the docs.

![](/images/UpdateImages/06-week6-images/in-the-breadboard.png)

### Getting Told What To Do By A Breadboard

And since I'm making a framework to do basically show a Powerpoint on the breadboard with breaks to let the user do something and then continue with the next step, it's also a perfect thing to let teachers write custom lessons that walk you through building a particular circuit or something. 

They'll be super simple human-read/writeable text files that can be dropped onto the filesystem over USB. They'll be saved into flash and you can run them any time from the Lessons library.

So you'll be able to write lessons for students like this:
```
    text "tap ground"
    flash arrow at net "gnd"
    wait for user to "tap gnd"
    text "tap row 24"
    wait for user to "tap 24"
```

For custom graphics and animations, I'll make a little web-based drawing tool to spit out the correct format. And keep a library of cool ones other people have made to use in your own lessons.


### Companion Square

This one will probably come out a little bit after they ship, but I want to assure people that their OG Jumperlesses won't become obsolete. The crossbar matrix and a lot of the hardware is basically the same, most of the V5's improvements are about making it feel effortless to get your intentions into hardware. So why not use the nicer UI from the Jumperless V5 to also control an OG Jumperless to double the working area? I need to hammer out the details of how to route analog signals between them in a nice-looking, robust way, but we'll figure something out. 



## Doggy Doggy, What Now?

The campaign is almost over, so if you want to save 20 bucks and probably get yours sooner (we're still on target to start shipping in mid-February 2025), this is the right time to order one. Otherwise it's all good, there's no hurry, they'll be available here on Crowd Supply *at least* through April 2026.

I'll keep posting updates on the super interesting world of small-mid scale manufacturing, writing firmware for an exceptionally weird piece of hardware, soliciting ideas and opinions, or whatever else I'm working on at the time.

Come can chat with the community and ask me any questions that come to mind on the [Jumperless Discord](https://discord.gg/CKP2chvcUt), [Forums](https://forum.jumperless.org/), [GitHub](https://github.com/Architeuthis-Flux/JumperlessV5), [Twitter/X](https://x.com/arabidsquid), [Bluesky](https://bsky.app/profile/architeuthisflux.bsky.social), [Mastodon](https://hackaday.social/@ArchiteuthisFlux).


Love,  
Kevin

---
# [Jumperless V5 on Crowd Supply](https://www.crowdsupply.com/architeuthis-flux/jumperless-v5)
---
P.S. I've been beta testing a prototype [Solder Ninja Pen](https://www.crowdsupply.com/sitron-labs/solder-ninja-pen) for 6 months now and it's kind of amazing. It's hard to explain exactly *why*, but it somehow solders better than my other tiny soldering irons in that same form factor. You should probably check it out.  



P.P.S. [Claire](https://linktr.ee/ClaireDanielleCassidy) ([@LaserMistress](https://mstdn.social/@LaserMistress) on Mastodon) made me these super rad laser cut acrylic clip-on earrings you can see me wearing in [Crowd Supply's Live from Supercon stream](https://www.youtube.com/watch?v=DAv4pyYnqyk), I'll see if I can convince her to somehow make these available for other people who want their own. Stickers are passé, *real* hackers put earrings on their laptops.

![](/images/UpdateImages/06-week6-images/earrings.jpg)







