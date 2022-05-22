/*
 * Copyright 2022 Johannes Linkels
 *
 * Acknowledgement:
 * This interface to MPV  was originally written by Brad Lanam Walnut Creek CA US
 * and published on https://wiki.tcl-lang.org/page/MPV+Tcl+Extension under the 
 * ZLIB/LIBPNG licence
 *
 * This package is published under the ZLIB/LIBPNG license as derived work.
 *
 * More information: https://wiki.tcl-lang.org/page/bll
 * 
 */

#define MPVDEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <tcl.h>
#include <mpv/client.h>
#include "tclmpv.h"
#include "config.h"

#define RETURN_IF_NOT_INIT(instance_ptr)								\
	if ( instance_ptr == NULL ) {										\
		Tcl_AddErrorInfo (interp, "error: mpv player not initialized");	\
		return TCL_ERROR;												\
	}
		

const char *
mpvStateToStr (
  mpv_event_id      state
  )
{
  int        i;
  const char *tptr;

  tptr = "";
  for (i = 0; i < stateMapMax; ++i) {
    if (state == stateMap[i].state) {
      tptr = stateMap[i].name;
      break;
    }
  }
  return tptr;
}

const char *
stateToStr (
  playstate state
  )
{
  int        i;
  const char *tptr;

  tptr = "";
  for (i = 0; i < playStateMapMax; ++i) {
    if (state == playStateMap[i].state) {
      tptr = playStateMap[i].name;
      break;
    }
  }
  return tptr;
}

/* executed in some arbitrary thread */
void
mpvCallbackHandler (
  void *cd
  )
{
  mpvData_t     *mpvData = (mpvData_t *) cd;

  mpvData->hasEvent = 1;
}

void mpvEventTick (
	ClientData cd
)
{
	struct		timespec curtime;
	mpvData_t   *mpvData = (mpvData_t *) cd;

	clock_gettime (CLOCK_MONOTONIC, &curtime);
	fprintf (mpvData->debugfh, "[%ld.%ld] mpv: mpvEventTick entered \n", curtime.tv_sec, curtime.tv_nsec);
	fflush (mpvData->debugfh); 
	
	mpvData->tickToken = Tcl_CreateTimerHandler (CHKTIMER, &mpvEventTick, mpvData);
}

void
mpvEventHandler (
  ClientData cd
  )
{
	mpvData_t   *mpvData = (mpvData_t *) cd;
	playstate   stateflag;
	struct		timespec curtime;

#if MPVDEBUG
	clock_gettime (CLOCK_MONOTONIC, &curtime);
	fprintf (mpvData->debugfh, "[%ld.%ld] mpv: mpvEventHandler entered \n", curtime.tv_sec, curtime.tv_nsec);
	fflush (mpvData->debugfh); 
#endif
	if (mpvData->inst == NULL) {
		return;
	}

	if (mpvData->hasEvent == 0) {
		mpvData->timerToken = Tcl_CreateTimerHandler (CHKTIMER, &mpvEventHandler, mpvData);
		return;
	}

  mpvData->hasEvent = 0;

	mpv_event *event = mpv_wait_event (mpvData->inst, 0.0);
	stateflag = stateMap[(int) mpvData->stateMapIdx[event->event_id]].stateflag;
	clock_gettime (CLOCK_MONOTONIC, &curtime);
	fprintf (mpvData->debugfh, "[%ld.%ld] mpv_event_name: %s\n", curtime.tv_sec, curtime.tv_nsec, mpv_event_name (event->event_id));

	while (event->event_id != MPV_EVENT_NONE) {

		if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {

			/************ start event == property change  ***************/
			mpv_event_property *prop = (mpv_event_property *) event->data;

#if MPVDEBUG
			if (mpvData->debugfh != NULL) {
				fprintf (mpvData->debugfh, "[%ld.%ld] mpv: ev: prop: %s\n", curtime.tv_sec, curtime.tv_nsec, prop->name);
				fflush (mpvData->debugfh); 
			}
#endif

			if (strcmp (prop->name, "time-pos") == 0) {
				if (prop->format == MPV_FORMAT_DOUBLE) {
					mpvData->tm = * (double *) prop->data;
				}
#if MPVDEBUG
					fprintf (mpvData->debugfh, "formet: %d, new time-pos: %.2f\n", prop->format, mpvData->tm);
					fflush (mpvData->debugfh); 
#endif
			} else if (strcmp (prop->name, "duration") == 0) {
				if (mpvData->state == PS_BUFFERING) {
					mpvData->state = PS_PLAYING;
				}
				if (prop->format == MPV_FORMAT_DOUBLE) {
					mpvData->duration = * (double *) prop->data;
				}
#if MPVDEBUG
				fprintf (mpvData->debugfh, "mpv: ev: dur: %.2f\n", mpvData->duration);
				fflush (mpvData->debugfh); 
			}
#endif
		/***********i END PROPERTY CHANGE ***************/
		} else if (stateflag != PS_NONE) {
				  mpvData->state = stateflag;
#if MPVDEBUG
        fprintf (mpvData->debugfh, "mpv: state: %s\n", stateToStr(mpvData->state));
		fflush (mpvData->debugfh); 
#endif
		} /****** end stateflage != PS_NONE ********/

		mpv_event *event = mpv_wait_event (mpvData->inst, 0.0);
		stateflag = stateMap[(int) mpvData->stateMapIdx[event->event_id]].stateflag;

#if MPVDEBUG
		clock_gettime (CLOCK_MONOTONIC, &curtime);
		fprintf (mpvData->debugfh, "[%ld.%ld] mpv_event_name: %s\n", curtime.tv_sec, curtime.tv_nsec, mpv_event_name (event->event_id));
		fflush (mpvData->debugfh); 
#endif
	} /******** end while event != 0 *********/

  mpvData->timerToken = Tcl_CreateTimerHandler (CHKTIMER, &mpvEventHandler, mpvData);
}

int
mpvDurationCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  double            tm;
  mpvData_t *mpvData = (mpvData_t *) cd;
	
	RETURN_IF_NOT_INIT (mpvData->inst);

  if (objc != 1) {
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }

    tm = mpvData->duration;
    Tcl_SetObjResult (interp, Tcl_NewDoubleObj (tm));
	return TCL_OK;
}

int
mpvGetTimeCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  double    tm;
  mpvData_t *mpvData = (mpvData_t *) cd;

  if (objc != 1) {
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    tm = mpvData->tm;
    Tcl_SetObjResult (interp, Tcl_NewDoubleObj (tm));
  }
  return rc;
}

int
mpvIsPlayCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  int       rval;
  mpvData_t *mpvData = (mpvData_t *) cd;

  if (objc != 1) {
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }

  rc = TCL_OK;
  rval = 0;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    /*
     * In order to match the implementation of VLC's internal
     * isplaying command, return true if the player is paused
     * If the telnet VLC interface is ever dropped, this interface
     * could be enhanced.
     */
    if (mpvData->state ==  PS_OPENING ||
        mpvData->state ==  PS_PLAYING ||
        mpvData->state ==  PS_PAUSED) {
      rval = 1;
    }
    Tcl_SetObjResult (interp, Tcl_NewIntObj (rval));
  }
  return rc;
}

int
mpvMediaCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int               rc;
  int               status, lstatus, sstatus;
  mpvData_t         *mpvData = (mpvData_t *) cd;
  char              *fn;
	char			*stt;
  struct stat       statinfo;
  double            dval;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "media");
    return TCL_ERROR;
  }

	fprintf (mpvData->debugfh, "number of arguments to media command: %d\n", objc);
	fflush (mpvData->debugfh); 


#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "mpvData->inst: %p, mpvData->device: %s\n", (void *) mpvData->inst, mpvData->device);
		fflush (mpvData->debugfh); 
    }
#endif

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    if (mpvData->device != NULL) {
      status = mpv_set_property (mpvData->inst, "audio-device", MPV_FORMAT_STRING, (void *) mpvData->device);
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "set-ad:status:%d %s\n", status, mpv_error_string(status));
		fflush (mpvData->debugfh); 
      }
#endif
    }

    fn = Tcl_GetString(objv[1]);
    if (stat (fn, &statinfo) != 0) {
      rc = TCL_ERROR;
      return rc;
    }

	if (objc == 3) {
		stt = Tcl_GetString(objv[2]);
        fprintf (mpvData->debugfh, "stat: %d, stt: %s\n", stat(stt, &statinfo), stt);
		fflush (mpvData->debugfh); 
    }

      /* reset the duration and time */
    mpvData->duration = 0.0;
    mpvData->tm = 0.0;
    /* like many players, mpv will start playing when the 'loadfile'
     * command is executed.
     */
    const char *cmd[] = {"loadfile", fn, "replace", NULL};
    lstatus = mpv_command (mpvData->inst, cmd);
    dval = 1.0;
    sstatus = mpv_set_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &dval);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "loadfile:status:%d %s\n", lstatus, mpv_error_string(lstatus));
      fprintf (mpvData->debugfh, "speed-1:status:%d %s\n", sstatus, mpv_error_string(sstatus));
		fflush (mpvData->debugfh); 
    }
#endif
  }
  return rc;
}

int
mpvLoadFileCmd (
	ClientData cd,
	Tcl_Interp* interp,
	int objc,
	Tcl_Obj * const objv[]
	)
{
	int				rc;
	int				status;
	int				validflag;
	mpvData_t		*mpvData = (mpvData_t *) cd;
	char			*fn;
	char			*arg2;
	struct stat		statinfo;
	double			dval;
	char			errmsg[256];

	/********
	Call with: ::tcl::tclmpv::loadfile filename ?flags? ?options?
	flags: replace | append | append-play
	options: option1=foo,option2=bar
	There must be no spaces in the options arguments
	********/
	RETURN_IF_NOT_INIT (mpvData->inst);

	if (objc < 2 || objc > 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "URL ?flags? ?options?");
		return TCL_ERROR;
	}


	rc = TCL_OK;

	if (mpvData->device != NULL) {
		status = mpv_set_property (mpvData->inst, "audio-device", MPV_FORMAT_STRING, (void *) mpvData->device);
		if ( status < 0) {
			snprintf (errmsg, sizeof(errmsg), "error setting audio device: %s", mpv_error_string(status)); 
			Tcl_AddErrorInfo (interp, errmsg);
			return TCL_ERROR;
		}
	}

    fn = Tcl_GetString(objv[1]);
    if (stat (fn, &statinfo) != 0) {
		snprintf (errmsg, sizeof(errmsg), "mediafile %s does not exist", fn); 
		Tcl_AddErrorInfo (interp, errmsg);
		return  TCL_ERROR;
    }


	validflag = 1;
	status = 0;
	// If the objc is 3 or 4, the 2nd argument can be a flag
	if (objc > 2) {
		arg2 = Tcl_GetString(objv[2]);
		if (strcmp (arg2, "replace") &&
			strcmp (arg2, "append") &&
			strcmp (arg2, "append-play")) {
		//One of the values did not match, so we don't have a flag
		validflag = 0;
		}
		// If objc is 3 and arg2 is not a valid flag it is an option
		// if it is a valid flag then a flag was passed and not an option
		// If objc is 4 arg2 must be a valid flag. If not it is an error
		if (objc == 3) {
			if (validflag) {
				const char* cmd[] = {"loadfile", fn, arg2, NULL};
				status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
				fprintf (mpvData->debugfh, "loadfile: valid flag %s;\n", arg2);
#endif
			} else { 
			/* Not a valid flag so it must be an option */
				const char* cmd[] = {"loadfile", fn, "replace", arg2, NULL};
				status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
				fprintf (mpvData->debugfh, "loadfile: not a valid flag, options passed: %s;\n", arg2);
#endif
			}
		} else {
			/* Number of parameters > 2 and < 5 so must be 4 */
			if (validflag) {
				const char* cmd[] = {"loadfile", fn,  arg2, Tcl_GetString(objv[3]), NULL};
				status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
				fprintf (mpvData->debugfh, "loadfile: valid flag: %s,  options passed: %s\n", arg2, Tcl_GetString(objv[3]));
#endif
			} else {
				rc = TCL_ERROR;
			}
		}
    } else {
		/* no flags, no options */
		const char* cmd[] = {"loadfile", fn, "replace", NULL};
		status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
		fprintf (mpvData->debugfh, "loadfile: no flags, no options\n");
#endif
	} 

#if MPVDEBUG
    fprintf (mpvData->debugfh, "loadfile: mpv_command status:%d %s\n", status, mpv_error_string(status));
	fflush (mpvData->debugfh); 
#endif

	if (rc) {
		Tcl_AddErrorInfo (interp, "invalid flag specified, must be \"replace\", \"append\" or \"append-play\"");
		return rc;
	}
	if (status) {
		snprintf (errmsg, sizeof(errmsg), "error loading file into mpv: %s", mpv_error_string(status)); 
		Tcl_AddErrorInfo (interp, errmsg);
		return TCL_ERROR;
	}
	

	/* reset the duration and time */
	mpvData->duration = 0.0;
	mpvData->tm = 0.0;

    dval = 1.0;
    status = mpv_set_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &dval);

#if MPVDEBUG
    fprintf (mpvData->debugfh, "loadfile speed-1:status:%d %s\n", status, mpv_error_string(status));
	fflush (mpvData->debugfh); 
#endif

	if (status) {
		return TCL_ERROR;
	}
	return TCL_OK;
}

int
mpvPauseCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  int       status, ppause, result;
  mpvData_t *mpvData = (mpvData_t *) cd;

#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "objc: %d\n", objc);
        fprintf (mpvData->debugfh, "mpvData->inst: %p \n", (void *) mpvData->inst);
        fprintf (mpvData->debugfh, "mpvData->state %x \n", mpvData->state);
        fprintf (mpvData->debugfh, "mpvData->paused: %x \n", mpvData->paused);
		fflush (mpvData->debugfh);
      }
#endif

  if (objc != 1) {
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    if (mpvData->state != PS_PLAYING && mpvData->state != PS_PAUSED) {
      ;
    } else if (mpvData->state ==  PS_PLAYING &&
        mpvData->paused == 0) {
      int val = 1;
      status = mpv_set_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &val);
	  result = mpv_get_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &ppause);
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "pause-%d:status:%d %s\n", val, status, mpv_error_string(status));
        fprintf (mpvData->debugfh, "(pause) retrieved ppause:%d, error: %d %s\n", ppause, result, mpv_error_string(status));
		fflush (mpvData->debugfh);
      }
#endif
      mpvData->paused = 1;
      mpvData->state = PS_PAUSED;
    } else if (mpvData->state == PS_PAUSED &&
        mpvData->paused == 1) {
      int val = 0;
      status = mpv_set_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &val);
	  result = mpv_get_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &ppause);
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "unpause-%d:status:%d %s\n", val, status, mpv_error_string(status));
        fprintf (mpvData->debugfh, "(unpause) retrieved ppause:%d, error: %d %s\n", ppause, result, mpv_error_string(status));
		fflush (mpvData->debugfh);
      }
#endif
      mpvData->paused = 0;
      mpvData->state = PS_PLAYING;
    }
  }
  return rc;
}

int
mpvPlayCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  int       status;
  mpvData_t *mpvData = (mpvData_t *) cd;

  if (objc != 1) {
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "error: mpvData->inst == NULL \n");
		fflush (mpvData->debugfh);
      }
#endif
    rc = TCL_ERROR;
  } else {
    if (mpvData->state == PS_PAUSED &&
        mpvData->paused == 1) {
      int val = 0;
      status = mpv_set_property (mpvData->inst, "pause", MPV_FORMAT_FLAG, &val);
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "play:status:%d %s\n", status, mpv_error_string(status));
		fflush (mpvData->debugfh);
      }
#endif
      mpvData->paused = 0;
      mpvData->state = PS_PLAYING;
    }
  }
  return rc;
}

int
mpvRateCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  int       status;
  mpvData_t *mpvData = (mpvData_t *) cd;
  double    rate;
  double    d;


  if (objc != 1 && objc != 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "?rate?");
    return TCL_ERROR;
  }

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    if (objc == 2 && mpvData->state == PS_PLAYING) {
      rc = Tcl_GetDoubleFromObj (interp, objv[1], &d);
      if (rc == TCL_OK) {
        rate = d;
        status = mpv_set_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &rate);
#if MPVDEBUG
        if (mpvData->debugfh != NULL) {
          fprintf (mpvData->debugfh, "speed-%.2f:status:%d %s\n", rate, status, mpv_error_string(status));
        }
#endif
      }
    }

    status = mpv_get_property (mpvData->inst, "speed", MPV_FORMAT_DOUBLE, &rate);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "speed-get:status:%d %s\n", status, mpv_error_string(status));
    }
#endif
    Tcl_SetObjResult (interp, Tcl_NewDoubleObj ((double) rate));
  }
  return rc;
}

int
mpvSeekCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  //double    tm;
  mpvData_t *mpvData = (mpvData_t *) cd;
  double    pos;
  double    d;
  char      spos [40];

#if MPVDEBUG
        if (mpvData->debugfh != NULL) {
          fprintf (mpvData->debugfh, "seek entered: objc: %d, mpvData->state: %d \n", objc, mpvData->state);
			fflush (mpvData->debugfh );
        }
#endif
	/*
	TODO: Add support for flags as defined in mpv.io
	*/
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "?position?");
		return TCL_ERROR;
	}

	RETURN_IF_NOT_INIT (mpvData->inst);

	if (mpvData->state == PS_PLAYING) {
		rc = Tcl_GetDoubleFromObj (interp, objv[1], &d);
		if ( rc != TCL_OK) {
			Tcl_AddErrorInfo (interp, "error: seek time in seconds must be a decimal number");	
			return TCL_ERROR;
		}
		pos = (double) d;
		sprintf (spos, "%.1f", pos);
		const char *cmd[] = { "seek", spos, "absolute", NULL };
		rc = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
		fprintf (mpvData->debugfh, "seek-%s:status:%d %s\n", spos, rc, mpv_error_string(rc));
		fflush (mpvData->debugfh );
	}
#endif
	/* The statement below was in the original code.
	*	However it is useless because mpvData->tm is only
	*	set after mpv raises an event AND the event is catched
	*	in the event handler.
		tm = mpvData->tm;
		Tcl_SetObjResult (interp, Tcl_NewDoubleObj ((double) tm));
	*/
	return TCL_OK;
}

int
mpvStateCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int               rc;
  mpv_event_id      plstate;
  mpvData_t         *mpvData = (mpvData_t *) cd;

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    plstate = mpvData->state;
    Tcl_SetObjResult (interp, Tcl_NewStringObj (stateToStr(plstate), -1));
  }
  return rc;
}

int
mpvStopCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  int       status;
  mpvData_t *mpvData = (mpvData_t *) cd;

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    /* stop: stops playback and clears playlist */
    /* difference: vlc's stop command does not clear the playlist */
    const char *cmd[] = {"stop", NULL};
    status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
    if (mpvData->debugfh != NULL) {
      fprintf (mpvData->debugfh, "stop:status:%d %s\n", status, mpv_error_string(status));
    }
#endif
  }
  return rc;
}

int
mpvQuitCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;
  int       status;
  mpvData_t *mpvData = (mpvData_t *) cd;

      fprintf (mpvData->debugfh, "mpvQuitCmd entered\n");
		fflush (mpvData->debugfh);

  rc = TCL_OK;
  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    /* quits the player */
    const char *cmd[] = {"quit", 0,  NULL};
    status = mpv_command (mpvData->inst, cmd);
#if MPVDEBUG
      fprintf (mpvData->debugfh, "quit:status:%d %s\n", status, mpv_error_string(status));
		fflush (mpvData->debugfh);
#endif
  }
  return rc;
}

int
mpvHaveAudioDevListCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  int       rc;

  rc = 1;
  Tcl_SetObjResult (interp, Tcl_NewBooleanObj (rc));
  return TCL_OK;
}

int
mpvVersionCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  mpvData_t     *mpvData = (mpvData_t *) cd;

  Tcl_SetObjResult (interp, Tcl_NewStringObj (mpvData->version, -1));
  return TCL_OK;
}

void
mpvClose (
	mpvData_t		 *mpvData
	)
{
	/*
	* Internal function, not to be exposed to TCL
	*/
	int	 i;

	if (mpvData->inst != NULL) {
		mpv_terminate_destroy (mpvData->inst);
		mpvData->inst = NULL;
	}
	if (mpvData->argv != NULL) {
		for (i = 0; i < mpvData->argc; ++i) {
			ckfree (mpvData->argv[i]);
		}
		ckfree (mpvData->argv);
		mpvData->argv = NULL;
	}

	if (mpvData->device != NULL) {
		free ((void *) mpvData->device);
		mpvData->device = NULL;
	}

	mpvData->state = PS_STOPPED;
}

void
mpvExitHandler (
  void *cd
  )
{
  mpvData_t     *mpvData = (mpvData_t *) cd;


  Tcl_DeleteTimerHandler (mpvData->timerToken);
  mpvClose (mpvData);
/********
  if (mpvData->debugfh != NULL) {
    fclose (mpvData->debugfh);
    mpvData->debugfh = NULL;
  }
***********/
  ckfree (cd);
}

int
mpvReleaseCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  mpvData_t     *mpvData = (mpvData_t *) cd;

  Tcl_DeleteTimerHandler (mpvData->timerToken);
  mpvClose (mpvData);
/********
  if (mpvData->debugfh != NULL) {
    fclose (mpvData->debugfh);
    mpvData->debugfh = NULL;
  }
***********/
  return TCL_OK;
}

int
mpvInitCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  char          *tptr;
  char          *nptr;
  int           rc;
  int           i;
  int           len;
  int           gstatus;
  int           status;
  mpvData_t     *mpvData = (mpvData_t *) cd;

	/* FIXME: I don't quite understand this piece of code. It looks like the parameters
	* to the init command are processed to form options. However at the end  the 
	* parameter array in the mpvData structure is set to NULL. And no options are 
	* passed to the mpv instance.
	*/
  mpvData->argv = (const char **) ckalloc (sizeof(const char *) * (size_t) (objc + 1));
  for (i = 0; i < objc; ++i) {
    tptr = Tcl_GetStringFromObj (objv[i], &len);
    nptr = (char *) ckalloc (len+1);
    strcpy (nptr, tptr);
    mpvData->argv[i] = nptr;
  }
  mpvData->argc = objc;
  mpvData->argv[objc] = NULL;

  rc = TCL_ERROR;
  gstatus = 0;

  if (mpvData->inst == NULL) {
    mpvData->inst = mpv_create ();
#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "mpvData->inst: %p\n", (void *) mpvData->inst);
		fflush (mpvData->debugfh); 
      }
#endif
    if (mpvData->inst != NULL) {
		status = mpv_initialize (mpvData->inst);
#if MPVDEBUG
		fprintf (mpvData->debugfh, "initialization status:%d\n", status);
		fflush (mpvData->debugfh); 
#endif
		if (status < 0) { gstatus = status; }
		double vol = 100.0;
		mpv_set_property (mpvData->inst, "volume", MPV_FORMAT_DOUBLE, &vol);
		mpv_observe_property(mpvData->inst, 0, "duration", MPV_FORMAT_DOUBLE);
		mpv_observe_property(mpvData->inst, 0, "time-pos", MPV_FORMAT_DOUBLE);
		mpv_observe_property(mpvData->inst, 0, "filename", MPV_FORMAT_STRING);

		/*
		* From now on, it is expected that events be handled
		*/
		mpv_set_wakeup_callback (mpvData->inst, &mpvCallbackHandler, mpvData);
		mpvData->timerToken = Tcl_CreateTimerHandler (CHKTIMER, &mpvEventHandler, mpvData);
	}
  }
  if (mpvData->inst != NULL && gstatus == 0) {
    rc = TCL_OK;
  }
  return rc;
}

int
mpvAudioDevSetCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  mpvData_t     *mpvData = (mpvData_t *) cd;
  int           rc;
  char          *dev;

  if (objc != 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "deviceid");
    return TCL_ERROR;
  }

  rc = TCL_OK;

  if (mpvData->inst == NULL) {
    rc = TCL_ERROR;
  } else {
    dev = Tcl_GetString(objv[1]);
    if (mpvData->device != NULL) {
      free ((void *) mpvData->device);
    }
    mpvData->device = NULL;
    if (strlen (dev) > 0) {
      mpvData->device = strdup (dev);
    }
  }

  return rc;
}

int
mpvAudioDevListCmd (
  ClientData cd,
  Tcl_Interp* interp,
  int objc,
  Tcl_Obj * const objv[]
  )
{
  mpvData_t     *mpvData = (mpvData_t *) cd;
  Tcl_Obj       *lobj;
  Tcl_Obj       *sobj;
  mpv_node      anodes;
  mpv_node_list *infolist;
  mpv_node_list *nodelist;
  char          *nmptr = NULL;
  char          *descptr = NULL;

  if (mpvData->inst == NULL) {
    return TCL_ERROR;
  }

  lobj = Tcl_NewListObj (0, NULL);

  mpv_get_property (mpvData->inst, "audio-device-list", MPV_FORMAT_NODE, &anodes);
  if (anodes.format != MPV_FORMAT_NODE_ARRAY) {
    return TCL_ERROR;
  }
  nodelist = anodes.u.list;
  for (int i = 0; i < nodelist->num; ++i) {
    infolist = nodelist->values[i].u.list;
    for (int j = 0; j < infolist->num; ++j) {
      if (strcmp (infolist->keys[j], "name") == 0) {
        nmptr = infolist->values[j].u.string;
      } else if (strcmp (infolist->keys[j], "description") == 0) {
        descptr = infolist->values[j].u.string;
      } else {
        if (mpvData->debugfh != NULL) {
          fprintf (mpvData->debugfh, "dev: %s\n", infolist->keys[j]);
        }
      }
    }
	if (nmptr != NULL) {
		sobj = Tcl_NewStringObj (nmptr, -1);
		Tcl_ListObjAppendElement (interp, lobj, sobj);
	}
	if (descptr != NULL) {
		sobj = Tcl_NewStringObj (descptr, -1);
		Tcl_ListObjAppendElement (interp, lobj, sobj);
	}
  }
  mpv_free_node_contents (&anodes);
  Tcl_SetObjResult (interp, lobj);
  return TCL_OK;
}

int
Tclmpv_Init (Tcl_Interp *interp)
{
  Tcl_Namespace *nsPtr = NULL;
  Tcl_Command   ensemble = NULL;
  Tcl_Obj       *dictObj = NULL;
  Tcl_DString   ds;
  mpvData_t     *mpvData;
  unsigned int  ivers;
  int           i;
  int           debug;
  const char    *nsName = "::tclmpv";
  const char    *cmdName = nsName + 5;
  char          tvers [20];

  if (!Tcl_InitStubs (interp,"8.3",0)) {
    return TCL_ERROR;
  }

  debug = 0;
#if MPVDEBUG
  debug = 1;
#endif
  mpvData = (mpvData_t *) ckalloc (sizeof (mpvData_t));
  mpvData->interp = interp;
  mpvData->inst = NULL;
  mpvData->argv = NULL;
  mpvData->state = PS_NONE;
  mpvData->device = NULL;
  mpvData->paused = 0;
  mpvData->duration = 0.0;
  mpvData->tm = 0.0;
  mpvData->hasEvent = 0;
  //mpvData->timerToken = Tcl_CreateTimerHandler (CHKTIMER, &mpvEventHandler, mpvData);
  mpvData->timerToken = NULL;
  //mpvData->tickToken = Tcl_CreateTimerHandler (CHKTIMER, &mpvEventTick, mpvData);
  mpvData->tickToken = NULL;
  mpvData->debugfh = NULL;
  for (i = 0; i < stateMapIdxMax; ++i) {
    mpvData->stateMapIdx[i] = 0;
  }
  for (i = 0; i < stateMapMax; ++i) {
    mpvData->stateMapIdx[stateMap[i].state] = i;
  }

  if (debug) {
    mpvData->debugfh = fopen ("mpvdebug.txt", "w+");
  }

#if MPVDEBUG
      if (mpvData->debugfh != NULL) {
        fprintf (mpvData->debugfh, "debug output file created\n");
		fflush (mpvData->debugfh); 
      }
#endif

  nsPtr = Tcl_FindNamespace(interp, nsName, NULL, 0);
  if (nsPtr == NULL) {
    nsPtr = Tcl_CreateNamespace(interp, nsName, NULL, 0);
    if (nsPtr == NULL) {
      Tcl_Panic ("failed to create namespace: %s\n", nsName);
    }
  }

  ensemble = Tcl_CreateEnsemble(interp, cmdName, nsPtr, TCL_ENSEMBLE_PREFIX);
  if (ensemble == NULL) {
    Tcl_Panic ("failed to create ensemble: %s\n", cmdName);
  }
  Tcl_DStringInit (&ds);
  Tcl_DStringAppend (&ds, nsName, -1);

  dictObj = Tcl_NewObj();
  for (i = 0; mpvCmdMap[i].name != NULL; ++i) {
    Tcl_Obj *nameObj;
    Tcl_Obj *fqdnObj;

    nameObj = Tcl_NewStringObj (mpvCmdMap[i].name, -1);
    fqdnObj = Tcl_NewStringObj (Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
    Tcl_AppendStringsToObj (fqdnObj, "::", mpvCmdMap[i].name, NULL);
    Tcl_DictObjPut (NULL, dictObj, nameObj, fqdnObj);
    if (mpvCmdMap[i].proc) {
      Tcl_CreateObjCommand (interp, Tcl_GetString (fqdnObj),
           mpvCmdMap[i].proc, (ClientData) mpvData, NULL);
    }
  }

  if (ensemble) {
    Tcl_SetEnsembleMappingDict (interp, ensemble, dictObj);
  }

  Tcl_DStringFree(&ds);

  ivers = mpv_client_api_version();
  sprintf (tvers, "%d.%d", ivers >> 16, ivers & 0xFF);
  strcpy (mpvData->version, tvers);

  /* If the 'package ifneeded' and package provides do
   * not match, tcl fails.  Can't really use the mpv
   * version number here.
   */
  Tcl_PkgProvide (interp, TCLMPV_PKGNAME, TCLMPV_PKGVERSION	);

  return TCL_OK;
}
