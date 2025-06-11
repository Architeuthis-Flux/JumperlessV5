# Writing Apps



### Here's a the example app that should show the calls for most of the things you might want to do

You can do literally anything the Jumperless can in an app, so if there's a specific thing, lmk and I'll write an example. Until I make this into a proper operating system, what you're doing when you write an App is just writing a function in the main firmware. There's really no guard rails, and the API is just any function in the firmware. 


## First get it PlatformIO set up to flash code

So fork the firmware here: https://github.com/Architeuthis-Flux/JumperlessV5

I'm using PlatformIO in VSCode. And it *should* just work to open the RP23V50firmware folder in that (you'll probably need to comment out `upload_port = /dev/cu.usbmodem101` in `Platformio.ini` so it'll just automatically find it)

You should just open the `RP23V50firmware` folder, not the entire `JumperlessV5` repo, in VSCode.

You should probably try to just load the firmware just to make sure everything works.

## To write an App


Before you go writing your app, follow these steps to make it so it's listed in the App library and you can run it from the menus.

- Go to [`menuTree.h`](https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/RP23V50firmware/src/menuTree.h) and add the name of your app under `Apps\n\` (shown as `-Custom App\n\` here, it needs to fit in 7x2 chars to show on the breadboard)
<img width="349" alt="menutree" src="https://github.com/user-attachments/assets/c619f655-de53-4a8c-8bde-24c536956546" />

- Go to `Apps.h` and declare your function where you'll write your app
<img width="758" alt="Appdoth" src="https://github.com/user-attachments/assets/63f7fbab-a63c-4f7c-b454-b4166875d6af" />

- Go to `Apps.cpp` and add a struct in the `struct app apps[30]` for your app 
`{"Name", index, ??idk, name of the function (unused)}`
<img width="510" alt="appStruct" src="https://github.com/user-attachments/assets/f2b02ab6-0a49-46be-bf38-264bc8022417" />

- Go to `Apps.cpp > runApp()` and add a `case` for your app's index (this is so you can also find it by index rather than exact matching the name `"Custom App"`
<img width="587" alt="runApp" src="https://github.com/user-attachments/assets/3285c3d4-3efd-4685-9683-da4cd44cac51" />

- Make a function that's the entirety of your app, I just pushed a demo function called `customApp(void)` with some (non exhaustive) examples of things you can do from an app.
<img width="907" alt="customApp" src="https://github.com/user-attachments/assets/57787127-ac7f-4923-9073-c2eb0611bd6a" />

- Run your app with the clickwheel, `Apps > Custom App`.

The quick way run `"Custom App"` is to just enter `2` in the main menu, or just use the clickwheel and go Apps > Custom App.

<img width="418" alt="mainSwitch" src="https://github.com/user-attachments/assets/8fbc2583-0f4f-4cc1-a245-b6fbad5ac785" />

If you want to add your own shortcut, find an unused menu character and add a 
```
case'3':
{
runApp(3); //the app index you set above
break;
}
``` 
in the big main menu `switch` statement in `main.cpp`.

## To actually write the app

[The code](https://github.com/Architeuthis-Flux/JumperlessV5/blob/6fd4fcba572c4b524435ec36c8901adcedbf52c6/RP23V50firmware/src/Apps.cpp#L141) for `Custom App` is an example of the calls available with comments telling you what's going on. There are tons more, but what's shown there are the higher-level helper functions that should roughly do what they say they're doing.

