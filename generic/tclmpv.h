/*
 * Copyright 2018 Brad Lanam Walnut Creek CA US
*/

#ifndef _TCLMPV_H
#define _TCLMPV_H

#define CHKTIMER 100

typedef struct { char *name; Tcl_ObjCmdProc *proc; } EnsembleData;

typedef enum playstate {
  PS_NONE = 0,
  PS_IDLE = 1,
  PS_OPENING = 2,
    /* for MPV, mark the state as buffering until we receive the */
    /* duration property change */
  PS_BUFFERING = 3,
  PS_PLAYING = 4,
  PS_PAUSED = 5,
  PS_STOPPED = 6,
  PS_ERROR = 7
} playstate;

typedef struct {
  playstate             state;
  const char *          name;
} playStateMap_t;

static const playStateMap_t playStateMap[] = {
  { PS_NONE, "none" },
  { PS_IDLE, "idle" },
  { PS_OPENING, "opening" },
  { PS_BUFFERING, "buffering" },
  { PS_PLAYING, "playing" },
  { PS_PAUSED, "paused" },
  { PS_STOPPED, "stopped" },
  { PS_ERROR, "error" }
};
#define playStateMapMax (sizeof(playStateMap)/sizeof(playStateMap_t))

typedef struct {
  mpv_event_id          state;
  const char *          name;
  playstate             stateflag;
} stateMap_t;

static const stateMap_t stateMap[] = {
  { MPV_EVENT_NONE,             "none", PS_NONE },
  { MPV_EVENT_IDLE,             "idle", PS_IDLE },
  { MPV_EVENT_START_FILE,       "opening", PS_OPENING },
  { MPV_EVENT_FILE_LOADED,      "buffering", PS_BUFFERING },
  { MPV_EVENT_PROPERTY_CHANGE,  "property-chg", PS_NONE },
  { MPV_EVENT_SEEK,             "seeking", PS_NONE },
  { MPV_EVENT_PLAYBACK_RESTART, "playafterseek", PS_NONE },
  { MPV_EVENT_END_FILE,         "stopped", PS_STOPPED },
  { MPV_EVENT_SHUTDOWN,         "ended", PS_STOPPED },
    /* these next three are only for debugging */
  { MPV_EVENT_TRACKS_CHANGED,   "tracks-changed", PS_NONE },
  { MPV_EVENT_AUDIO_RECONFIG,   "audio-reconf", PS_NONE },
  { MPV_EVENT_METADATA_UPDATE,  "metadata-upd", PS_NONE }
};
#define stateMapMax (sizeof(stateMap)/sizeof(stateMap_t))

#define stateMapIdxMax 40 /* mpv currently has 24 states coded */
typedef struct {
  Tcl_Interp            *interp;
  mpv_handle            *inst;
  char                  version [40];
  //mpv_event_id          state;
  playstate				state;	
  int                   argc;
  const char            **argv;
  const char            *device;
  double                duration;
  double                tm;
  int                   paused;
  int                   hasEvent;       /* flag to process mpv event */
  Tcl_TimerToken        timerToken;
  Tcl_TimerToken        tickToken;
  int                   stateMapIdx [stateMapIdxMax];
  FILE                  *debugfh;
} mpvData_t;

const char * mpvStateToStr (mpv_event_id state);
const char * stateToStr (playstate state);
void mpvCallbackHandler (void *cd);
void mpvEventHandler (ClientData cd);
int mpvDurationCmd (ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvGetTimeCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvIsPlayCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvMediaCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvLoadFileCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvPauseCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvPlayCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvRateCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvSeekCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvStateCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvStopCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvQuitCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvHaveAudioDevListCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvVersionCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
void mpvClose ( mpvData_t     *mpvData);
void mpvExitHandler ( void *cd);
int mpvReleaseCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvInitCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvAudioDevSetCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);
int mpvAudioDevListCmd ( ClientData cd, Tcl_Interp* interp, int objc, Tcl_Obj * const objv[]);

static const EnsembleData mpvCmdMap[] = {
  { "audiodevlist", mpvAudioDevListCmd },
  { "audiodevset",  mpvAudioDevSetCmd },
  { "close",        mpvReleaseCmd },
  { "duration",     mpvDurationCmd },
  { "gettime",      mpvGetTimeCmd },
  { "init",         mpvInitCmd },
  { "haveaudiodevlist", mpvHaveAudioDevListCmd },
  { "isplay",       mpvIsPlayCmd },
  { "loadfile",     mpvLoadFileCmd },
  { "media",        mpvMediaCmd },
  { "pause",        mpvPauseCmd },
  { "play",         mpvPlayCmd },
  { "quit",         mpvQuitCmd },
  { "rate",         mpvRateCmd },
  { "seek",         mpvSeekCmd },
  { "state",        mpvStateCmd },
  { "stop",         mpvStopCmd },
  { "version",      mpvVersionCmd },
  { NULL, NULL }
};

int Tclmpv_Init (Tcl_Interp *interp);

#endif 
