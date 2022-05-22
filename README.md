TCLMPV
============

This package adds support for accessing libpmpv from with Tclsh or Wish.
Only audio files are supported and playlists are not used.

Dependencies
-------------

Standard development tools should be installed. If essential packages are not
installed, *configure* generates appropriate messages. In any case, these 
packages mist be installed:  

- gcc, make, autoconf, autotools
- mpv and libmpv-dev   
- tcl and tcl-dev
 
Building
--------

Clone the repository in a development directory. Run  

	./configure  
	make  
	sudo make install  

On Debian, Tcl/Tk libraries are installed in /usr/lib/tcltk/x86_64-linux-gnu.
To have make install in the correct directory, run configure with:  

	./configure --libdir=/usr/lib/tcltk/x86_64-linux-gnu/  

Debugging
---------

Setting **#MPVDEBUG 1** in tclmpv.c generates a mpvdebug.txt file with debug messages 
in the directory where the Tcl script is run.  

Examples
--------

In example/mpv_example.tcl most library functions are called. 

Platforms
---------

This package had been developed and tested on Debian 10. It should run on
Windows and Apple, but it is not tested in any way.  


License
-------

License terms are mentioned in the file licence.terms.

