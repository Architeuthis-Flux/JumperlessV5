
Hey everyone,

I originally made Jumperless V5 because it was something *I* wished I had. So when I make any engineering/UI decisions, I just ask myself "would I find this useful?" and "does this feel nice to use?". I think it's a solid strategy. But one of the coolest things about developing in the open with an active community is that they can cover some major blind spots in the ways I think people will want to use their Jumperless. 

As more of a hardware/firmware person, I don't spend a ton of time playing with full-blown-Linux-running single board computers like Raspberry Pis (of course I still have a drawer full of them because that's just what happens.) So when someone on the [Jumperless Discord server](https://discord.gg/CKP2chvcUt) asked how they could use their V5 with the 40-pin GPIO header on a Raspberry Pi, it was immediately clear what I had to do:

{SBCadapters-2.jpg}

Yes, design an adapter board in an hour and then spend eight more hours screwing around in Blender to get this cool rendering.

This SBC adapter board plugs into the Arduino Nano header and maps it to the GPIO pins on any Raspberry Pi. You can even have the Pi control the Jumperless V5 over the UART lines for fully remote Jumperless-ing.

{SBCadapters-3.jpg}

If Raspberry Pis aren't really your thing, this little adapter board has a couple more tricks up its sleeve:

{SBCadapters-1.jpg}

It includes a 0.91" 32x128 OLED module as a secondary display. One of the alpha testers said they had trouble reading text on the breadboard LEDs, so this will copy anything displayed there and give you a much roomier place to play DOOM or show whatever you like.

It friction-fits onto the board with wiggly offset holes so you can always remove it to read the pin mappings for the Pi. I also threw in some universal-ish SMD footprints on the back and am considering adding a Sparkfun Quiic connector, because why not?

{SBCadapters-4.jpg}

The design isn't completely finalized, so there's still time to ask me for more stuff to cram onto this little board. 

You might be thinking, "Wow, I that looks useful! How much extra is that going to cost?" The answer is nothing, these will be included in every box for free!

If you want daily-ish updates on my quest to make Jumperless V5 the single most useful and fun thing you have on your workbench, I'm always posting fun little tidbits on [Twitter/X](https://x.com/arabidsquid) and [Mastodon](https://hackaday.social/@ArchiteuthisFlux). 

Or come chat with the community on the [Jumperless Discord server](https://discord.gg/CKP2chvcUt) or the [Jumperless Forums](https://forum.jumperless.org).

Sincerely,  
Kevin


P.S. Extra huge thanks to these awesome contributors who helped form this board into what you see here:
* madbodger
* shallax
* DerelBims
* Archit3ch
* pluster
