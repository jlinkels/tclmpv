# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

# This file serves both as an example as to how to use the TclMpv extension
# as well as testing the extension for correct functionality.
# Testing should have been done with the TclTest package, but
# I found it quite tedious to set up a complete sequence of actions
# to test for every situation to test. And some return values
# are not exact and cannot be flagged as pass or fail. If implemented
# with TclTest I would have had to construct workaround for that anyway
# so writing this example was easier.

# Note that if the smaple mp3 files are not in the same folder and
# considerable time is needed for buffering, expected player states
# may not match. This Tcl extension only plays audio files, no video.

# Querying the player state after initialization (before a file
# is loaded might yield inaccurate results. The implementation
# of those states is not quite correct yet.

# If run in Tclsh, a TCL event loop must be initiated calling
# vwait. Only then the player will behave as expected because
# in the library internals functionality depends on a running
# event loop.

# Scheduling various events while a blocking call is made to
# vwait can be implemented by creating a main_loop function
# with is called periodically by itself. This is NOT a recursive
# call, just a rescheduling jist before exiting the function.

# There are other methods, like using multiple "after nn" {action}
# statements, but this becomes messy rather quickly.

# My application does not have any keyboard interaction, therefor
# the setup with the main loop. Obviously the extension can also
# be used in an event-driven environment.


package require tclmpv

proc main_loop {} {
	switch $::loopcnt {
		0 {
			# don't do a thing, just leave the proc and schedule it
			# to run again in 100 ms. This executes the statement
			# after the call to main_loop and starts the event loop
			incr ::loopcnt
			after 100 main_loop
			return
		}

		1 {
			puts "time: $::loopcnt seconds"
			puts "initialize the mpv player"
			::tclmpv::init
			puts "tclmpv state: expected: None, actual value: [::tclmpv::state]"
			puts "mpv library version: [::tclmpv::version]"
		}

		2 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Idle, actual value: [::tclmpv::state]"
			puts "load a file and start to play"
			::tclmpv::loadfile soul_bossa_nova_30s.mp3
		}

		3 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Playing, actual value: [::tclmpv::state] "
			puts "tclmpv duration: expected: 30, actual value: [::tclmpv::duration] "
			puts "tclmpv time: expected: > 0, actual value: [::tclmpv::gettime] "
		}

		8 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv time: expected: 5, actual value: [::tclmpv::gettime] "
		}

		12 {
			puts "time: $::loopcnt seconds"
			puts "pause the playback"
			::tclmpv::pause
		}

		13 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Pause, actual value: [::tclmpv::state] "
		}

		15 {
			puts "time: $::loopcnt seconds"
			puts "resume the playback"
			::tclmpv::play
		}

		16 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Playing, actual value: [::tclmpv::state] "
		}

		20 {
			puts "time: $::loopcnt seconds"
			# Time must be a number of seconds. So other formats
			# must be converted to seconds before calling seek
			puts "tclmpv time: actual value time before seek: [::tclmpv::gettime] "
			puts "seek to 19.6 seconds (absolute)"
			::tclmpv::seek 19.6
		}

		21 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Playing, actual value: [::tclmpv::state] "
			puts "tclmpv time: expected: 20, actual value: [::tclmpv::gettime] "
		}


		25 {
			puts "time: $::loopcnt seconds"
			puts "stop the playback"
			::tclmpv::stop
		}

		26 {
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Idle, actual value: [::tclmpv::state] "
		}

		27 {
			puts "time: $::loopcnt seconds"
			puts "load new file with offset of 10 seconds, length 5 seconds"
			# Values have to be in seconds. This is different from the 
			# documentation https://mpv.io/manual/stable/#options-start
			# But this is a property of libmpv, not the Tcl interface to libmpv.
			::tclmpv::loadfile winchester_cathedral_30s.mp3 start=10,length=5
			puts "tclmpv state: expected: Idle, actual value: [::tclmpv::state] "
		}

		28 { 
			puts "time: $::loopcnt seconds"
			puts "tclmpv state: expected: Playing, actual value: [::tclmpv::state] "
			puts "tclmpv time: expected: 11, actual value: [::tclmpv::gettime] "
		}

		33 { 
			puts "time: $::loopcnt seconds"
			puts "file must have played for the specified time"
			puts "tclmpv state: expected: Idle, actual value: [::tclmpv::state] "
		}

		35 {
			# See if we can discard the player instance and then re-initialize it
			# successfully
			puts "time: $::loopcnt seconds"
			puts "discarding the player instance"
			::tclmpv::close
		}

		37 {
			puts "time: $::loopcnt seconds"
			puts "query state of the player. Expected: error message"
			if { [catch {::tclmpv::state} errmsg ] } {
				puts "::tclmpv::state returned an error: $errmsg"
			} else {
				puts "player instance deleted, but no error"
			}
		}

		38 {
			puts "time: $::loopcnt seconds"
			puts "re-create the player instance"
			::tclmpv::init
			puts "after 100 ms, ask for the player status"
			after 100
			puts "tclmpv state: expected: Idle, actual value: [::tclmpv::state]"
			puts "mpv library version: [::tclmpv::version]"
		}

		39 {
			puts "time: $::loopcnt seconds"
			puts "load a file and start to play"
			::tclmpv::loadfile winchester_cathedral_30s.mp3
		}

		41 {
			# check that the event handler is running correctly
			# by querying the duration and the actual time
			puts "time: $::loopcnt seconds"
			puts "tclmpv duration: expected: 30, actual value: [::tclmpv::duration] "
			puts "tclmpv time: expected: 2, actual value: [::tclmpv::gettime] "
			}

		43 {
			puts "time: $::loopcnt seconds"
			puts "stop the playback"
			::tclmpv::stop
		}

		default {
			puts -nonewline "."
			flush stdout
		}

	}
	incr ::loopcnt
	if { $::loopcnt > 45 } { set ::forever 1}
	after 1000 main_loop
}

set ::loopcnt 0

# Interaction with the mpv library is only possible when an
# event loop is running.
# So first main_loop is called to return without any action
# then main_loop is called to carry out various functions
# after some time
main_loop
vwait ::forever 

puts "time: $::loopcnt seconds"
puts "discard the player and exit the script"
::tclmpv::close
exit 0

