# CppIEEmbed
Host a IWebBrowser2 control in a c++ program using only win32 (no mfc, wtl, atl etc)

Modified to draw content to screen.

Currently it loads html from bin/canvas.html
Displays HTML content on screen, only drawback is that 'position:absolute' doesn't work with compatible Edge mode.
You need to specity `<meta http-equiv="X-UA-Compatible" content="IE=edge">` otherwise transparency won't work.

Result:
<img src="https://i.imgur.com/Z7nM6jc.png">
