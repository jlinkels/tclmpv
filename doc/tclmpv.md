% tclmpv 0.12 | Interface to libmpv for TCL scripts 
% Original code by Brad Lanam 
% 0.12


# NAME

tclmpv - Interface to libmpv library for Tcl Scripts


# SYNOPSIS

**package require libmpv** ?0.12?

**::tclmpv::close**

**::tclmpv::duration**

**::tclmpv::eofinfo**

**::tclmpv::gettime**

**::tclmpv::isplay**

**::tclmpv::init**

**::tclmpv::loadfile** *filename* ?flags? ?*option=value* ...?

**::tclmpv::media** *filename* 

**::tclmpv::pause**

**::tclmpv::play**

**::tclmpv::quit**

**::tclmpv::rate** *factor*

**::tclmpv::seek** *position*

**::tclmpv::state**

**::tclmpv::stop**

**::tclmpv::version**


# DESCRIPTION

Provides and interface to the libmpv library for Tcl/Tk scripts. Only a small subset
of all available commands and options are currently implemented. This extension only 
plays audio files.

The main goal if this package is to provide capabilities for Tcl/Tk scripts to
play audio files, load audio files with an offset in start time, jump to a position
in  ithe audio file, and query the status of the player.

Playlists are not supported. The exception is that a file can be appended to the current
queue. Once appended it cannot be manipulated.

# COMMANDS

**::tclmpv::close**
:	Stops the player and releases the mpv instance. This stops all event handling and
	releases all memory and resources. It does not unload the Tcl library. After execution
	of this command, further calls to tclmpv functions yield an error with the exception
	of calling ::tclmpv::init.

**::tclmpv::duration**
:	Returns the duration of the currently playing file in seconds.

**::tclmpv::eofinfo**
:	Returns a list with the reason and error resulting from the last end-of-file
	event. The mpvlib *loadfile* command does not return an error when a non-existing file
	or stream is attempted to be loaded. Only by observing EOF and the associated
	reason this error can be retrieved. The list consists of 2 strings. The first
	givng the reason for EOF, the second the error causing the EOF if any.

**::tclmpv::gettime**
:	Returns the playback position in seconds of the currently playing.

**::tclmpv::isplay**
:	Returns TRUE if a file is currently playing, FALSE otherwise.

**::tclmpv::init**
:	Creates and initializes a new instance of mpv player. Once this player is created, all
	other tclmpv functions can be called. To remove the mpv instance, use ::tclmpv::close
	TODO: Check if a proper error message if generated if a second instance is created while
	the first one is not closed yet.

**::tclmpv::loadfile** *filename* ?flags? ?*option=value* ...?
:	Loads a file *filename* in the player and by default replaces the current file and start
	playing immediately.

These flags are recognized:  
	**replace (default)**  
	Stop playback of the current file, and play the new file immediately  
	**append**  
	Append the file to the playlist.  
	**append-play**  
	Append the file, and if nothing is currently playing, start playback. (Always starts with   
	the added file, even if the playlist was not empty before running this command.)  

*Options* passed to the loadfile command are specified in https://mpv.io/manual/stable/#options
	and are valid as far as applicable for this extension.  
	There is no check whether this is a valid file name. Even if it
	does not exist the mpvlib loadfile command does not return an error
	but play state returns to stopped as result of end-of-file.
	This is not unlike the command line clients of mpv.  

**Note** According to this documentation time can be specified as [hh:[mm:]]ss[.mmm]. However, the
	implementation of libmpv **only** allows time in the format ss[.mmm].
	
**::tclmpv::media** *filename* 
:	Loads a file *filename* in the player, replaces the current file and start
	playing immediately.
	This function can only be used to load media files which can be found on the
	file system. No http streams etc. When the file does not exist the function returns an error.

**::tclmpv::pause**
:	Puts the player in pause, provided it is playing. It had not effect when not in playing state.

**::tclmpv::play**
:	Resumes from pause.

**::tclmpv::quit**
:	Quits the player, that is it stops playing the current file and any queued filei, but the
	playlist is not cleared.  After this
	command the player is in idle state. The player instance is *not* released. In this state
	another file can be loaded and played.

**::tclmpv::rate** *factor*
:	Adjusts the playback speed by *factor*. The value of *factor* must be 0.01 - 100.

**::tclmpv::seek** *position*
:	Positions the current playback position to *position* seconds. When this value is negative
	it positions the player at *position* seconds from the end. *position is expressed as ss[.mmm].
	**Note** According to the documentation time can be specified as [hh:[mm:]]ss[.mmm]. However, the
	implementation of libmpv **only** allows time in the format ss[.mmm].

**::tclmpv::state**
:	Returns the current state of the player. See **States** below for a description.

**::tclmpv::stop**
:	Essentially the same as *quit*, but the playlist is not cleared.

**::tclmpv::version**
:	Returns the current version of mpv (not the Tcl library).
	
# PLAYER STATES

**TODO** Implementation of state handling in the extension could be improved. The
mpv player generates events asynchronously. These events are catched by periodically
calling an event handler in the extension. The event handler changed the internal
state of the extension. The TCL application in its turn polls
the internal state of the extension. Even if no events are missed by the extension
there is no guarantee the TCL application polls sufficiently often so that no
state transitions or intermediate states are missed. So currently only states pertaining
for a sufficiently long time, like Idle, Playing or Paused, are reliably reflected.  

**idle**
:	In this mode, no file is played, and the playback core waits for new commands. 
	This is the normal state the player is in after calling ::tclmpv::init or
	after the playback of the current file has ended.  

**opening**
:	A file is being opened.  

**buffering**
:	The player waits until a sufficient part of the file is loaded.  

**none**
:	The library is loaded, but the player is not initialized yet.  

**playing**
:	The player is playing a file.  

**paused**
:	The playback is paused by the *pause* command.  

**stopped**
:	The playback has ended because and end-of-file was encountered, or any other reason
	including an error or a request to load a new file. If and end-of-file condition was
	encountered, the player proceeds to the idle state shortly. After and end-of-file it
	is not guaranteed the stopped state is observed in the tcl application before the idke
	state is entered. 


# EXAMPLES

Simplest example. A more detailed example in examples/mpv_example.tcl

	#! /usr/bin/tclsh

	package require tclmpv

	proc check_stopped {} {
		if {!  [string compare -nocase [::tclmpv::state] idle] } {
			set ::exit_loop 1
		}
		after 500 check_stopped
	}

	::tclmpv::init
	# This check is called periodically to see if the playback has
	# stopped and quit the event loop. Any other event could be
	# used as well to exit the program.
	after 1000 check_stopped
	::tclmpv::media audiofile.mp3
	# The event loop *must* be started to ensure correct event handling
	# in the extension library.
	vwait exit_loop
	::tclmpv::close
	exit 0

# SEE ALSO

mpv(1) 

# NOTES

1. https://github.com/mpv-player/mpv/blob/master/libmpv/client.h
2. https://mpv.io/manual/master/#options
3. https://mpv.io/manual/master/#list-of-input-commands
4. https://mpv.io/manual/master/#properties
5. https://github.com/mpv-player/mpv-examples/tree/master/libmpv
6. https://wiki.tcl-lang.org/
7. https://wiki.tcl-lang.org/page/MPV+Tcl+Extension


# LICENSE

Copyright 2022 Johannes Linkels. 

Acknowledgement:
This interface to MPV  was originally written by Brad Lanam Walnut Grove CA US and published
on https://wiki.tcl-lang.org/page/MPV+Tcl+Extension under the ZLIB/LIBPNG
licence.  

This package is published under the ZLIB/LIBPNG license as derived work.

More information: https://wiki.tcl-lang.org/page/bll





