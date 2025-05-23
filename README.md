# WinHider

[![Build App](https://github.com/aamitn/winhider/actions/workflows/build.yml/badge.svg)](https://github.com/aamitn/winhider/actions/workflows/build.yml)

WinHider (short for _Window Hider_) is an application that allows you to hide user defined windows from screensharing (zoom, ms-teams, gmeet etc.) and also from taskbar / taskswitcher (Alt-Tab)

Original Fork From : [`https://github.com/radiantly/invisiwind`](https://github.com/radiantly/invisiwind)

## So .. what does it do exactly?

I think this is best explained with a couple of screenshots I took during a Zoom meeting:

<p float="left">
  <img src="./Misc/ss1.png" width="400" alt="What I see" />
  <img src="./Misc/ss2.png" width="400" alt="What they see" />
</p>

The screenshot on the left is what I see. The one on the right is what everyone else sees.

Using this tool, firefox and slack have been hidden so anyone watching the screenshare is unable to see those windows. However, I can continue to use them as usual on my side.

_Note: this tool works with any app (MS Teams, Discord, OBS, etc) and not just Zoom._

### What goes under the hood? 

The tool performs dll injection with dlls containg targets for :
- [SetWindowDisplayAffinity](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity) to `WDA_EXCLUDEFROMCAPTURE`. (For hiding from screenshare)
- Modifying the window’s [extended styles](https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles) (For hiding from taskbar & taskswitcher) : 
  1. It removes the [`WS_EX_APPWINDOW`](https://learn.microsoft.com/en-us/previous-versions/dd425531(v=vs.100)) style, which normally causes a window to appear in the taskbar and Alt-Tab.
  2. It adds the [`WS_EX_TOOLWINDOW`](https://learn.microsoft.com/en-us/previous-versions/dd410943(v%3Dvs.100)) style, which hides the window from the taskbar and Alt-Tab.

## How do I install it?

To use this application, you can either use the installer or the portable version.

>>> :
Binaries Legend :  
`Winhider.exe` -> 64-bit CLI  :  
`Winhider_32bit.exe` -> 32-bit CLI  :   
`WinhiderGui.exe` -> 64-bit GUI :  
`WinhiderGui_32bit.exe` -> 32-bit GUI : 
`hide_hotkey.exe` -> Auto Hotkey Handler(32-bit only)
 >>>

### Use the binary installer (recommended)

 - Download and run [WinhiderInstaller.exe](https://github.com/radiantly/Winhider/releases/download/latest/WinhiderInstaller.exe).
 - Once the installation is complete, you will be able to run `Winhider` from the Start Menu.

### Download the portable zip with prebuilt binaries

- Download and extract the generated zip bundle from [here](https://github.com/aamitn/Winhider/releases/download/latest/Winhider.zip).
- Run `Winhider.exe`. You will now be dropped into a terminal.

![usage](./Misc/illustration.gif)

Running it directly drops you into interactive mode. You can type `help` for more information.

You can also directly invoke it with commandline arguments. Type `Winhider --help` for argument specification.

### Build The Project 

- Download and extract the source from [here](https://github.com/aamitn/Winhider).
  ```bash
  git clone https://github.com/aamitn/Winhider
  cd Winhider
  ```
- If you have Visual Studio Installed, open the Winhider.sln and build as per provided configs.

- Alternatively you can run `build.ps1` in powershell to buiild from CLI without IDE
> To build without IDE form CLI using powershell script, make sure you have [Visual Studio Build Tools](https://aka.ms/vs/17/release/vs_BuildTools.exe) installed , you may skip this if you have entire Visual Studio Installation at system. 


## FAQ

#### What OSes are supported?

Microsoft Windows 10 v2004 or above. On previous versions of windows, a black screen shows up instead of hiding the window.

#### Do future instances of the application get automatically hidden?

No

#### Is it possible to see a preview of the screen on my side?
- You can simply use [`OBS Studio`](obsproject.com/download) with Windowed Projectors.
- Open OBS and do first-time setup.
- Then Right-Click under Sources-> Add-> Display Capture->OK-> Select Monitor under `Display` Dropdown-> OK. Now you will see infinity mirror if you have single diplay.
- Right click on the newly creted display under sources -> Click Windowed Projector.
- Minimize OBS and check for window hide status in projector window
- Multi-Monitor Systems will not require additional projector
<details>
<summary>Expand for Screenshot</summary>

![Windowed Projector](./Misc/win-projector.png)

_Tip: you can hide the Projector window from view too._

</details>

#### Could I automatically hide windows using a hotkey?

Yes! with the installer and zip-bundles we provide 2 ways to achieve this : 
- An Autohotkey(.ahk) script named `hide_hotkey.ahk` which could be run using Autohotkey v2+
- If you dont have Autohotkey installed in your system, you could also use the precompiled `hide_hotkey.exe` to use hotkey functions
>>> :
Hotkey Legend :  
Ctrl+H -> Hide from Screenshare :  
Ctrl+H -> Unhide from Screenshare :  
Ctrl+K -> Hide from Screenshare :  
Ctrl+L -> Hide from Screenshare :  
Ctrl+Q -> Exit/Quit Hotkey Script
 >>>

## Contributing

Feel free to open an issue/PR if you find a bug or would like to contribute!
