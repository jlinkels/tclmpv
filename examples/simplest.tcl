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
::tclmpv::media winchester_cathedral_30s.mp3
#::tclmpv::loadfile http://162.244.80.21:6482
# The event loop *must* be started to ensure correct event handling
# in the extension library.
vwait exit_loop
::tclmpv::close
exit 0

