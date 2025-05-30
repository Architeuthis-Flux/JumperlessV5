# Jumperless, Probelessly








Obviously the probe is kind of the defining new feature of the Jumperless V5. But there are some situations where you need something more automatable, and duct taping the probe to the end of a robotic arm is just *one* of the many ways you can accomplish that.

Luckily, the code was forked from the (originally probeless) OG Jumperless, so a lot of thought has gone into making connections with plain text sent over serial. Here's a sample of the netlist format:

```
1-35, 2-gnd, 3-top_rail, 4-d3, 5-adc1, 6-gpio7, uart_tx-d0, d1-uart_rx, 
```
*Yes, it's really that simple*

It's whitespace and case insensitive, and will accept most reasonable alternate spellings (for example; gRoUnD, t_rail, nano_d3, ADC_1, gp_7, Tx, UARTRX would all work just fine.)

To tell the Jumperless that you're about to send it a netlist and it should make those connections and store it in the current slot (I'll get to those in a bit), you just preface the netlist with an ``` f ```

Like this:
![](/Images/UpdateImages/04-week4-images/fconnections.png)

After the connections are sent, the Jumperless will parse them, clean up the whitespace and stuff, store them in nonvolatile flash, sort them so any connections that share a node are combined into a single net, figure out which connections each of the crossbar switches need to make, and then send that data to the crossbars to ultimately make the connections.

Another nice thing is if you send it something that would obviously cause problems, like ```top_rail-gnd,``` it'll just ignore it and move on. You can even set custom "Do Not Intersect" rules if you were so inclined, like "don't let D3 be in the same net as row 35" or whatever.

\<tangent\>    
  
Here's what it shows when you type ```n``` to show them all sorted into their own nets. 


![](/Images/UpdateImages/04-week4-images/Netlist.png)


Just for fun, this is what it shows when you enter ```b``` to show all the 16 X and 8 Y connections made by each of the 12 crossbars to make that happen.


![](/Images/UpdateImages/04-week4-images/ChipStatus.png)


If you're wondering how that table corresponds to the hardware, hopefully this clears it up a bit. Basically it's keeping track of what paths are used so it won't "cross the streams" and connect things that shouldn't be.

![](/Images/UpdateImages/04-week4-images/StatToSchem.png)


\<\/tangent\>

But you don't really need to worry too much about all that backend stuff. Long story short, Jumperless stores connections in its 16MB of onboard flash as *extremely* human-readable text files, and that's all it needs to make a circuit. 
All the steps to get from a netlist to physically connected rows is so quick on an RP2350B running at 150MHz that it doesn't need to store any extra information, it just does it from scratch every time it changes.

### So How Do I Share Circuits?

There are sooo many ways, dear reader, but let's start with the most stupidly simple one.

#### Copy / Paste
From the Jumperless App (or any other serial terminal app), type ```s```

![](/Images/UpdateImages/04-week4-images/SlotsNoHighlight.png)

Then just copy this chunk
![](/Images/UpdateImages/04-week4-images/SlotsHighlight.png)

Text, email, fax, telegram, carrier pigeon, Vulcan mind-meld, or semaphore it to someone else with a Jumperless, and they can just paste it into the main menu and that setup will be connected and saved to the currently active slot.

Or you can type ```o``` and paste the whole list (or any number) of slot files at once.


#### Mounting Jumperless V5 as an External Drive

What are these slots I speak of? Well, each slot is a .txt file on the Jumperless's internal filesystem, written in that text file is simply the text you just copied. 

You can tell the Jumperless V5 to mount as an external drive by entering ```+``` in the menu, or double-tapping the BOOT button (on the underside of the USB-C port) and drag and drop whatever circuits you like.

![](/Images/UpdateImages/04-week4-images/DriveDesktop.png)
![](/Images/UpdateImages/04-week4-images/JumperlessDrive.png)


Then you can cycle between slots either by the onboard menus and click wheel, sending ```x``` (next slot) or ```z```(previous slot) over serial, or something even crazier like setting an ADC to switch slots based on a control voltage or pulses from a button. 

It defaults to setting up 8 slots to give you some room to store different projects you're working on, but the only limit to how many you can add is the 16MB of storage. Which at an average size of ~100 bytes, is 160,000 slots (okay minus a few thousand for the rest of the firmware.)

#### Sharing a [Wokwi](https://wokwi.com/) Link

I haven't really talked about it much (because let's face it, using the probe is so much cooler), but just like the OG Jumperless, you can give the Jumperless App a [Wokwi](https://docs.wokwi.com/) project link and it will update every connection you make in the browser-based breadboard simulator in in hardware, in real time.


![](/Images/UpdateImages/04-week4-images/AppLink.png)


![](/Images/UpdateImages/04-week4-images/Wokwi.png)

![](/Images/UpdateImages/04-week4-images/WokwiNetlist.png)

It'll even automatically flash the Arduino you plug into your Jumperless's Nano header with any code you write in Wokwi

![](/Images/UpdateImages/04-week4-images/WokwiFlash.png)


#### UART Lines

Just one more I'll throw into this non-exhaustive list of ways to tell your Jumperless to do things, it will also accept these netlists over the routeable UART lines. So you can program an Arduino or whatever to wire up it's own circuit, and it's really as simple as:
```
Serial.print("f 1-35, 2-gnd, 3-top_rail, 4-d3, 5-adc1, 6-gpio7, uart_tx-d0, d1-uart_rx, ");
```
In this case the UART lines are properly connected in the netlist, but you can also tell the Jumperless to always inject the proper connections to Rx and Tx so it's always ready for the next command and you don't have to worry about adding them in the Arduino code.



### Why Are There So Many Ways To Do The Same Thing?!

I want Jumperless V5 to feel like you can throw anything at it and it'll just do what you want. I like tools that work *with* the unique way your brain intuitively expects them to, instead of trying to enforce some rigid process that only makes sense to the person who made it. So having a bunch of different ways to control it increases the likelihood you'll find a workflow that feels completely natural, and opens it up for the super nerds to hack it into doing crazy, unexpected nonsense with their Jumperless V5s.

This all may seem like a lot, but really the reason I added most of these things is because an OG Jumperless user asked for it (btw pretty much everything I've laid out here also works on the OG Jumperless.) My general response to feature requests is, "Yes, as long as I can figure out a way to make it not annoying to someone who *isn't* using it in this way." I spend so much time working with this codebase that I basically have it memorized, so adding/fixing things is often easier than you may expect. 

So feel free to ask me for anything you want, either on the [Jumperless Discord](https://discord.gg/CKP2chvcUt), [Forums](https://forum.jumperless.org/), [GitHub](https://github.com/Architeuthis-Flux/JumperlessV5), [Twitter/X](https://x.com/arabidsquid), [Bluesky](https://bsky.app/profile/architeuthisflux.bsky.social), [Mastodon](https://hackaday.social/@ArchiteuthisFlux), whatever.   
All feature requests go to the same place anyway, on a piece of gaffer tape in metallic sharpie stuck to my desk. 

![](/Images/UpdateImages/04-week4-images/FeatureRequests.jpeg)

Cordially,
Kevin
