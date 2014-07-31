marcelino
=========

Yet another tiny XCB Window Manager developed in C.

Initially started from Tiny WM XCB Version, then I started to look into other Window Managers like
MCWM and follow its tracks trying to understand the XCB protocol and how to implement a Window Manager.
After that I've started to gather all the info I could about XCB which it seems it is sort of a challenge.

The aim of this new WM is to be used in my own Linux Distro ( Lombix ), so I can create a Desktop integrated 
the way I want. Custom is my motto. Apart from this at this stage I cannot tell much more for the simple fact 
I've not done it yet. 


NOTE: Don't even try to compile it, it does not work yet and it is a very much In Progress work.

References: http://incise.org/tinywm.html ( Tiny WM )
	    https://github.com/rtyler/tinywm-ada/blob/master/tinywm-xcb.c ( Tiny WM XCB version )
	    http://hack.org/mc/hacks/mcwm ( MCWM )
	    http://awesome.naquadah.org ( AWESOME )
	    http://git.pernsteiner.org/xcbcwm.git/tree/xcbcwm/WindowManager.cpp?id=d463c416aacabef7b05648dad3d6220400e78af2 ( A composite c++ WM )
        http://stackoverflow.com/questions/12904844/how-to-get-event-when-new-application-launches-using-xcb
    
Documentation: http://xcb.freedesktop.org/manual/group__XCB____API.html ( XCB API )
               http://xcb.freedesktop.org/tutorial/events/
               http://xcb.freedesktop.org/windowcontextandmanipulation/
