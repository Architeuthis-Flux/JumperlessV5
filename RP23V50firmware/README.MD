# Writing Apps

Until I make this into a proper operating system, what you're doing when you write an App is just writing a function in the main firmware. There's really no guard rails, and the API is just any function in the firmware. 

So fork the firmware here: https://github.com/Architeuthis-Flux/JumperlessV5

I'm using PlatformIO in VSCode. And it *should* just work to open the RP23V50firmware folder in that (you'll probably need to comment out `upload_port = /dev/cu.usbmodem101` in `Platformio.ini` so it'll just automatically find it)

You should probably try to just load the firmware just ot make sure everything works.

**To write an App**
- Go to [`menuTree.h`](https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/RP23V50firmware/src/menuTree.h) and add the name of your app under `Apps\n\` (shown as `-Custom App\n\` here, it needs to fit in 7x2 chars to show on the breadboard)

- Go to `Apps.h` and declare your function where you'll write your app

- Go to `Apps.cpp` and add a struct in the `struct app apps[30]` for your app 
`{"Name", index, ??idk, name of the function (unused)}`

- Go to `Apps.cpp > runApp()` and add a `case` for your app's index (this is so you can also find it by index rather than exact matching the name `"Custom App"`

- Make a function that's the entirety of your app, I just pushed a demo function called `customApp(void)` with some (non exhaustive) examples of things you can do from an app.

- Run your app with the clickwheel, `Apps > Custom App` if that's annoying, you can find an unused menu character and add a 
```
case'2':
{
runApp(2);
break;
}
``` 
in the big main menu `switch` statement in `main.cpp`.











omfg thanks for pointing me in this direction, I think I *finally* just wrote a useful example/docs app. I'll keep working on it, but I think it cleanly covers most of the API pretty much anyone would want to use. 

You can do literally anything the Jumperless can in an app, so if there's a specific thing, lmk and I'll write an example.

Here's the code for that function and a UF2 file for V5 rev 3 and 4 ( <@259138024556855296> use Rev 4, everyone else use Rev 3)

The quick way run `"Custom App"` is to just enter `2` in the main menu, or just use the clickwheel and go Apps > Custom App. You should probably be looking at [the code](https://github.com/Architeuthis-Flux/JumperlessV5/blob/6fd4fcba572c4b524435ec36c8901adcedbf52c6/RP23V50firmware/src/Apps.cpp#L141) while you run it because it's not super self-explanatory.


