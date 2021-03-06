/*****************************************************************************
 * vlc_input.h: Core input structures
 *****************************************************************************
 * Copyright (C) 1999-2015 VLC authors and VideoLAN
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_INPUT_H
#define VLC_INPUT_H 1

/**
 * \defgroup input Input
 * \ingroup vlc
 * Input thread
 * @{
 * \file
 * Input thread interface
 */

#include <vlc_es.h>
#include <vlc_meta.h>
#include <vlc_epg.h>
#include <vlc_events.h>
#include <vlc_input_item.h>
#include <vlc_vout.h>
#include <vlc_vout_osd.h>

#include <string.h>

/*****************************************************************************
 * Seek point: (generalisation of chapters)
 *****************************************************************************/
struct seekpoint_t
{
    vlc_tick_t i_time_offset;
    char    *psz_name;
};

static inline seekpoint_t *vlc_seekpoint_New( void )
{
    seekpoint_t *point = (seekpoint_t*)malloc( sizeof( seekpoint_t ) );
    if( !point )
        return NULL;
    point->i_time_offset = -1;
    point->psz_name = NULL;
    return point;
}

static inline void vlc_seekpoint_Delete( seekpoint_t *point )
{
    if( !point ) return;
    free( point->psz_name );
    free( point );
}

static inline seekpoint_t *vlc_seekpoint_Duplicate( const seekpoint_t *src )
{
    seekpoint_t *point = vlc_seekpoint_New();
    if( likely(point) )
    {
        if( src->psz_name ) point->psz_name = strdup( src->psz_name );
        point->i_time_offset = src->i_time_offset;
    }
    return point;
}

/*****************************************************************************
 * Title:
 *****************************************************************************/

/* input_title_t.i_flags field */
#define INPUT_TITLE_MENU         0x01   /* Menu title */
#define INPUT_TITLE_INTERACTIVE  0x02   /* Interactive title. Playback position has no meaning. */

typedef struct input_title_t
{
    char        *psz_name;

    vlc_tick_t  i_length;   /* Length(microsecond) if known, else 0 */

    unsigned    i_flags;    /* Is it a menu or a normal entry */

    /* Title seekpoint */
    int         i_seekpoint;
    seekpoint_t **seekpoint;
} input_title_t;

static inline input_title_t *vlc_input_title_New(void)
{
    input_title_t *t = (input_title_t*)malloc( sizeof( input_title_t ) );
    if( !t )
        return NULL;

    t->psz_name = NULL;
    t->i_flags = 0;
    t->i_length = 0;
    t->i_seekpoint = 0;
    t->seekpoint = NULL;

    return t;
}

static inline void vlc_input_title_Delete( input_title_t *t )
{
    int i;
    if( t == NULL )
        return;

    free( t->psz_name );
    for( i = 0; i < t->i_seekpoint; i++ )
        vlc_seekpoint_Delete( t->seekpoint[i] );
    free( t->seekpoint );
    free( t );
}

static inline input_title_t *vlc_input_title_Duplicate( const input_title_t *t )
{
    input_title_t *dup = vlc_input_title_New( );
    if( dup == NULL) return NULL;

    if( t->psz_name ) dup->psz_name = strdup( t->psz_name );
    dup->i_flags     = t->i_flags;
    dup->i_length    = t->i_length;
    if( t->i_seekpoint > 0 )
    {
        dup->seekpoint = (seekpoint_t**)vlc_alloc( t->i_seekpoint, sizeof(seekpoint_t*) );
        if( likely(dup->seekpoint) )
        {
            for( int i = 0; i < t->i_seekpoint; i++ )
                dup->seekpoint[i] = vlc_seekpoint_Duplicate( t->seekpoint[i] );
            dup->i_seekpoint = t->i_seekpoint;
        }
    }

    return dup;
}

/*****************************************************************************
 * Attachments
 *****************************************************************************/
struct input_attachment_t
{
    char *psz_name;
    char *psz_mime;
    char *psz_description;

    size_t i_data;
    void *p_data;
};

static inline void vlc_input_attachment_Delete( input_attachment_t *a )
{
    if( !a )
        return;

    free( a->p_data );
    free( a->psz_description );
    free( a->psz_mime );
    free( a->psz_name );
    free( a );
}

static inline input_attachment_t *vlc_input_attachment_New( const char *psz_name,
                                                            const char *psz_mime,
                                                            const char *psz_description,
                                                            const void *p_data,
                                                            size_t i_data )
{
    input_attachment_t *a = (input_attachment_t *)malloc( sizeof (*a) );
    if( unlikely(a == NULL) )
        return NULL;

    a->psz_name = strdup( psz_name ? psz_name : "" );
    a->psz_mime = strdup( psz_mime ? psz_mime : "" );
    a->psz_description = strdup( psz_description ? psz_description : "" );
    a->i_data = i_data;
    a->p_data = malloc( i_data );
    if( i_data > 0 && likely(a->p_data != NULL) )
        memcpy( a->p_data, p_data, i_data );

    if( unlikely(a->psz_name == NULL || a->psz_mime == NULL
              || a->psz_description == NULL || (i_data > 0 && a->p_data == NULL)) )
    {
        vlc_input_attachment_Delete( a );
        a = NULL;
    }
    return a;
}

static inline input_attachment_t *vlc_input_attachment_Duplicate( const input_attachment_t *a )
{
    return vlc_input_attachment_New( a->psz_name, a->psz_mime, a->psz_description,
                                     a->p_data, a->i_data );
}

/*****************************************************************************
 * input defines/constants.
 *****************************************************************************/

/**
 * This defines an opaque input resource handler.
 */
typedef struct input_resource_t input_resource_t;

/**
 * Main structure representing an input thread. This structure is mostly
 * private. The only public fields are read-only and constant.
 */
struct input_thread_t
{
    struct vlc_common_members obj;
};

/**
 * Record prefix string.
 * TODO make it configurable.
 */
#define INPUT_RECORD_PREFIX "vlc-record-%Y-%m-%d-%Hh%Mm%Ss-$ N-$ p"

/*****************************************************************************
 * Input events and variables
 *****************************************************************************/

/**
 * \defgroup inputvariable Input variables
 *
 * The input provides multiples variable you can write to and/or read from.
 *
 * TODO complete the documentation.
 * The read only variables are:
 *  - "length"
 *  - "can-seek" (if you can seek, it doesn't say if 'bar display' has be shown
 *    or not, for that check position != 0.0)
 *  - "can-pause"
 *  - "can-rate"
 *  - "can-rewind"
 *  - "can-record" (if a stream can be recorded while playing)
 *  - "teletext-es" (list of id from the spu tracks (spu-es) that are teletext, the
 *                   variable value being the one currently selected, -1 if no teletext)
 *  - "signal-quality"
 *  - "signal-strength"
 *  - "program-scrambled" (if the current program is scrambled)
 *  - "cache" (level of data cached [0 .. 1])
 *
 * The read-write variables are:
 *  - state (\see input_state_e)
 *  - rate
 *  - position
 *  - time, time-offset
 *  - title, next-title, prev-title
 *  - chapter, next-chapter, next-chapter-prev
 *  - program, audio-es, video-es, spu-es
 *  - audio-delay, spu-delay
 *  - bookmark (bookmark list)
 *  - record
 *  - frame-next
 *  - navigation (list of "title %2i")
 *  - "title %2i"
 *
 * The variable used for event is
 *  - intf-event (\see input_event_type_e)
 */

/**
 * Input state
 *
 * This enum is used by the variable "state"
 */
typedef enum input_state_e
{
    INIT_S = 0,
    OPENING_S,
    PLAYING_S,
    PAUSE_S,
    END_S,
    ERROR_S,
} input_state_e;

/**
 * Input rate.
 *
 * It is an float used by the variable "rate" in the
 * range [INPUT_RATE_MIN, INPUT_RATE_MAX]
 * the default value being 1.f. It represents the ratio of playback speed to
 * nominal speed (bigger is faster).
 */

/**
 * Minimal rate value
 */
#define INPUT_RATE_MIN 0.03125f
/**
 * Maximal rate value
 */
#define INPUT_RATE_MAX 31.25f

/**
 * Input events
 *
 * You can catch input event by adding a callback on the variable "intf-event".
 * This variable is an integer that will hold a input_event_type_e value.
 */
typedef enum input_event_type_e
{
    /* "state" has changed */
    INPUT_EVENT_STATE,
    /* b_dead is true */
    INPUT_EVENT_DEAD,

    /* "rate" has changed */
    INPUT_EVENT_RATE,

    /* "capabilities" has changed */
    INPUT_EVENT_CAPABILITIES,

    /* At least one of "position" or "time" */
    INPUT_EVENT_POSITION,

    /* "length" has changed */
    INPUT_EVENT_LENGTH,

    /* A title has been added or removed or selected.
     * It implies that the chapter has changed (no chapter event is sent) */
    INPUT_EVENT_TITLE,
    /* A chapter has been added or removed or selected. */
    INPUT_EVENT_CHAPTER,

    /* A program ("program") has been added or removed or selected,
     * or "program-scrambled" has changed.*/
    INPUT_EVENT_PROGRAM,
    /* A ES has been added or removed or selected */
    INPUT_EVENT_ES,

    /* "record" has changed */
    INPUT_EVENT_RECORD,

    /* input_item_t media has changed */
    INPUT_EVENT_ITEM_META,
    /* input_item_t info has changed */
    INPUT_EVENT_ITEM_INFO,
    /* input_item_t epg has changed */
    INPUT_EVENT_ITEM_EPG,

    /* Input statistics have been updated */
    INPUT_EVENT_STATISTICS,
    /* At least one of "signal-quality" or "signal-strength" has changed */
    INPUT_EVENT_SIGNAL,

    /* "audio-delay" has changed */
    INPUT_EVENT_AUDIO_DELAY,
    /* "spu-delay" has changed */
    INPUT_EVENT_SUBTITLE_DELAY,

    /* "bookmark" has changed */
    INPUT_EVENT_BOOKMARK,

    /* cache" has changed */
    INPUT_EVENT_CACHE,

    /* A vout_thread_t object has been created/deleted by *the input* */
    INPUT_EVENT_VOUT,

    /* (pre-)parsing events */
    INPUT_EVENT_SUBITEMS,

    /* vbi_page has changed */
    INPUT_EVENT_VBI_PAGE,
    /* vbi_transparent has changed */
    INPUT_EVENT_VBI_TRANSPARENCY,

    /* subs_fps has changed */
    INPUT_EVENT_SUBS_FPS,

    /* Thumbnail generation */
    INPUT_EVENT_THUMBNAIL_READY,
} input_event_type_e;

#define VLC_INPUT_CAPABILITIES_SEEKABLE (1<<0)
#define VLC_INPUT_CAPABILITIES_PAUSEABLE (1<<1)
#define VLC_INPUT_CAPABILITIES_CHANGE_RATE (1<<2)
#define VLC_INPUT_CAPABILITIES_REWINDABLE (1<<3)
#define VLC_INPUT_CAPABILITIES_RECORDABLE (1<<4)

struct vlc_input_event_position
{
    float percentage;
    vlc_tick_t ms;
};

struct vlc_input_event_title
{
    enum {
        VLC_INPUT_TITLE_NEW_LIST,
        VLC_INPUT_TITLE_SELECTED,
    } action;
    union
    {
        struct
        {
            input_title_t *const *array;
            size_t count;
        } list;
        size_t selected_idx;
    };
};

struct vlc_input_event_chapter
{
    int title;
    int seekpoint;
};

struct vlc_input_event_program {
    enum {
        VLC_INPUT_PROGRAM_ADDED,
        VLC_INPUT_PROGRAM_DELETED,
        VLC_INPUT_PROGRAM_UPDATED,
        VLC_INPUT_PROGRAM_SELECTED,
        VLC_INPUT_PROGRAM_SCRAMBLED,
    } action;
    int id;
    union {
        const char *title;
        bool scrambled;
    };
};

struct vlc_input_event_es {
    enum {
        VLC_INPUT_ES_ADDED,
        VLC_INPUT_ES_DELETED,
        VLC_INPUT_ES_UPDATED,
        VLC_INPUT_ES_SELECTED,
        VLC_INPUT_ES_UNSELECTED,
    } action;
    /**
     * ES track id: only valid from the event callback, unless the id is held
     * by the user with vlc_es_Hold(). */
    vlc_es_id_t *id;
    /**
     * Title of ES track, can be updated after the VLC_INPUT_ES_UPDATED event.
     */
    const char *title;
    /**
     * ES track information, can be updated after the VLC_INPUT_ES_UPDATED event.
     */
    const es_format_t *fmt;
};

struct vlc_input_event_signal {
    float quality;
    float strength;
};

struct vlc_input_event_vout
{
    enum {
        VLC_INPUT_EVENT_VOUT_ADDED,
        VLC_INPUT_EVENT_VOUT_DELETED,
    } action;
    vout_thread_t *vout;
};

struct vlc_input_event
{
    input_event_type_e type;

    union {
        /* INPUT_EVENT_STATE */
        input_state_e state;
        /* INPUT_EVENT_RATE */
        float rate;
        /* INPUT_EVENT_CAPABILITIES */
        int capabilities; /**< cf. VLC_INPUT_CAPABILITIES_* bitwise flags */
        /* INPUT_EVENT_POSITION */
        struct vlc_input_event_position position;
        /* INPUT_EVENT_LENGTH */
        vlc_tick_t length;
        /* INPUT_EVENT_TITLE */
        struct vlc_input_event_title title;
        /* INPUT_EVENT_CHAPTER */
        struct vlc_input_event_chapter chapter;
        /* INPUT_EVENT_PROGRAM */
        struct vlc_input_event_program program;
        /* INPUT_EVENT_ES */
        struct vlc_input_event_es es;
        /* INPUT_EVENT_RECORD */
        bool record;
        /* INPUT_EVENT_STATISTICS */
        const struct input_stats_t *stats;
        /* INPUT_EVENT_SIGNAL */
        struct vlc_input_event_signal signal;
        /* INPUT_EVENT_AUDIO_DELAY */
        vlc_tick_t audio_delay;
        /* INPUT_EVENT_SUBTITLE_DELAY */
        vlc_tick_t subtitle_delay;
        /* INPUT_EVENT_CACHE */
        float cache;
        /* INPUT_EVENT_VOUT */
        struct vlc_input_event_vout vout;
        /* INPUT_EVENT_SUBITEMS */
        input_item_node_t *subitems;
        /* INPUT_EVENT_VBI_PAGE */
        unsigned vbi_page;
        /* INPUT_EVENT_VBI_TRANSPARENCY */
        bool vbi_transparent;
        /* INPUT_EVENT_SUBS_FPS */
        float subs_fps;
        /* INPUT_EVENT_THUMBNAIL_READY */
        picture_t *thumbnail;
    };
};

typedef void (*input_thread_events_cb)( input_thread_t *input,
                                        const struct vlc_input_event *event,
                                        void *userdata);

/**
 * Input queries
 */
enum input_query_e
{
    /* Menu (VCD/DVD/BD) Navigation */
    /** Activate the navigation item selected. res=can fail */
    INPUT_NAV_ACTIVATE,
    /** Use the up arrow to select a navigation item above. res=can fail */
    INPUT_NAV_UP,
    /** Use the down arrow to select a navigation item under. res=can fail */
    INPUT_NAV_DOWN,
    /** Use the left arrow to select a navigation item on the left. res=can fail */
    INPUT_NAV_LEFT,
    /** Use the right arrow to select a navigation item on the right. res=can fail */
    INPUT_NAV_RIGHT,
    /** Activate the popup Menu (for BD). res=can fail */
    INPUT_NAV_POPUP,
    /** Activate disc Root Menu. res=can fail */
    INPUT_NAV_MENU,

    /* bookmarks */
    INPUT_GET_BOOKMARK,    /* arg1= seekpoint_t *               res=can fail */
    INPUT_GET_BOOKMARKS,   /* arg1= seekpoint_t *** arg2= int * res=can fail */
    INPUT_CLEAR_BOOKMARKS, /* res=can fail */
    INPUT_ADD_BOOKMARK,    /* arg1= seekpoint_t *  res=can fail   */
    INPUT_CHANGE_BOOKMARK, /* arg1= seekpoint_t * arg2= int * res=can fail   */
    INPUT_DEL_BOOKMARK,    /* arg1= seekpoint_t *  res=can fail   */
    INPUT_SET_BOOKMARK,    /* arg1= int  res=can fail    */

    /* titles */
    INPUT_GET_FULL_TITLE_INFO,     /* arg1=input_title_t*** arg2= int * res=can fail */

    /* On the fly input slave */
    INPUT_ADD_SLAVE,       /* arg1= enum slave_type, arg2= const char *,
                            * arg3= bool forced, arg4= bool notify,
                            * arg5= bool check_extension */

    /* ES */
    INPUT_RESTART_ES_BY_ID,/* arg1=int (-AUDIO/VIDEO/SPU_ES for the whole category) */

    /* Viewpoint */
    INPUT_UPDATE_VIEWPOINT, /* arg1=(const vlc_viewpoint_t*), arg2=bool b_absolute */
    INPUT_SET_INITIAL_VIEWPOINT, /* arg1=(const vlc_viewpoint_t*) */

    /* Input ressources
     * XXX You must call vlc_object_release as soon as possible */
    INPUT_GET_AOUT,         /* arg1=audio_output_t **              res=can fail */
    INPUT_GET_VOUTS,        /* arg1=vout_thread_t ***, size_t *        res=can fail */
    INPUT_GET_ES_OBJECTS,   /* arg1=int id, vlc_object_t **dec, vout_thread_t **, audio_output_t ** */

    /* Renderers */
    INPUT_SET_RENDERER,     /* arg1=vlc_renderer_item_t* */

    /* External clock managments */
    INPUT_GET_PCR_SYSTEM,   /* arg1=vlc_tick_t *, arg2=vlc_tick_t *       res=can fail */
    INPUT_MODIFY_PCR_SYSTEM,/* arg1=int absolute, arg2=vlc_tick_t   res=can fail */
};

/** @}*/

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
VLC_API input_thread_t * input_Create( vlc_object_t *p_parent,
                                       input_thread_events_cb event_cb, void *events_data,
                                       input_item_t *, const char *psz_log, input_resource_t *,
                                       vlc_renderer_item_t* p_renderer ) VLC_USED;
#define input_Create(a,b,c,d,e,f,g) input_Create(VLC_OBJECT(a),b,c,d,e,f,g)


/**
 * Creates an item preparser.
 *
 * Creates an input thread to preparse an item. The input needs to be started
 * with input_Start() afterwards.
 *
 * @param obj parent object
 * @param item input item to preparse
 * @return an input thread or NULL on error
 */
VLC_API input_thread_t *input_CreatePreparser(vlc_object_t *obj,
                                              input_thread_events_cb events_cb,
                                              void *events_data, input_item_t *item)
VLC_USED;

VLC_API
input_thread_t *input_CreateThumbnailer(vlc_object_t *obj,
                                        input_thread_events_cb events_cb,
                                        void *events_data, input_item_t *item)
VLC_USED;

VLC_API int input_Start( input_thread_t * );

VLC_API void input_Stop( input_thread_t * );

VLC_API int input_Read( vlc_object_t *, input_item_t *,
                        input_thread_events_cb, void * );
#define input_Read(a,b,c,d) input_Read(VLC_OBJECT(a),b,c,d)

VLC_API int input_vaControl( input_thread_t *, int i_query, va_list  );

VLC_API int input_Control( input_thread_t *, int i_query, ...  );

VLC_API void input_Close( input_thread_t * );

VLC_API void input_SetTime( input_thread_t *, vlc_tick_t i_time, bool b_fast );

VLC_API void input_SetPosition( input_thread_t *, float f_position, bool b_fast );

VLC_API void input_LegacyEvents(input_thread_t *, const struct vlc_input_event *, void * );
VLC_API void input_LegacyVarInit ( input_thread_t * );

/**
 * Get the input item for an input thread
 *
 * You have to keep a reference to the input or to the input_item_t until
 * you do not need it anymore.
 */
VLC_API input_item_t* input_GetItem( input_thread_t * ) VLC_USED;

/**
 * Return one of the video output (if any). If possible, you should use
 * INPUT_GET_VOUTS directly and process _all_ video outputs instead.
 * @param p_input an input thread from which to get a video output
 * @return NULL on error, or a video output thread pointer (which needs to be
 * released with vlc_object_release()).
 */
static inline vout_thread_t *input_GetVout( input_thread_t *p_input )
{
     vout_thread_t **pp_vout, *p_vout;
     size_t i_vout;

     if( input_Control( p_input, INPUT_GET_VOUTS, &pp_vout, &i_vout ) )
         return NULL;

     for( size_t i = 1; i < i_vout; i++ )
         vlc_object_release( (vlc_object_t *)(pp_vout[i]) );

     p_vout = (i_vout >= 1) ? pp_vout[0] : NULL;
     free( pp_vout );
     return p_vout;
}

static inline int input_AddSlave( input_thread_t *p_input, enum slave_type type,
                                  const char *psz_uri, bool b_forced,
                                  bool b_notify, bool b_check_ext )
{
    return input_Control( p_input, INPUT_ADD_SLAVE, type, psz_uri, b_forced,
                          b_notify, b_check_ext );
}

/**
 * Update the viewpoint of the input thread. The viewpoint will be applied to
 * all vouts and aouts.
 *
 * @param p_input an input thread
 * @param p_viewpoint the viewpoint value
 * @param b_absolute if true replace the old viewpoint with the new one. If
 * false, increase/decrease it.
 * @return VLC_SUCCESS or a VLC error code
 */
static inline int input_UpdateViewpoint( input_thread_t *p_input,
                                         const vlc_viewpoint_t *p_viewpoint,
                                         bool b_absolute )
{
    return input_Control( p_input, INPUT_UPDATE_VIEWPOINT, p_viewpoint,
                          b_absolute );
}

/**
 * Return the audio output (if any) associated with an input.
 * @param p_input an input thread
 * @return NULL on error, or the audio output (which needs to be
 * released with vlc_object_release()).
 */
static inline audio_output_t *input_GetAout( input_thread_t *p_input )
{
     audio_output_t *p_aout;
     return input_Control( p_input, INPUT_GET_AOUT, &p_aout ) ? NULL : p_aout;
}

/**
 * Returns the objects associated to an ES.
 *
 * You must release all non NULL object using vlc_object_release.
 * You may set pointer of pointer to NULL to avoid retreiving it.
 */
static inline int input_GetEsObjects( input_thread_t *p_input, int i_id,
                                      vlc_object_t **pp_decoder,
                                      vout_thread_t **pp_vout, audio_output_t **pp_aout )
{
    return input_Control( p_input, INPUT_GET_ES_OBJECTS, i_id,
                          pp_decoder, pp_vout, pp_aout );
}

/**
 * \see input_clock_GetSystemOrigin
 */
static inline int input_GetPcrSystem( input_thread_t *p_input, vlc_tick_t *pi_system, vlc_tick_t *pi_delay )
{
    return input_Control( p_input, INPUT_GET_PCR_SYSTEM, pi_system, pi_delay );
}
/**
 * \see input_clock_ChangeSystemOrigin
 */
static inline int input_ModifyPcrSystem( input_thread_t *p_input, bool b_absolute, vlc_tick_t i_system )
{
    return input_Control( p_input, INPUT_MODIFY_PCR_SYSTEM, b_absolute, i_system );
}

/* */
VLC_API decoder_t * input_DecoderCreate( vlc_object_t *, const es_format_t *, input_resource_t * ) VLC_USED;
VLC_API void input_DecoderDelete( decoder_t * );
VLC_API void input_DecoderDecode( decoder_t *, block_t *, bool b_do_pace );
VLC_API void input_DecoderDrain( decoder_t * );
VLC_API void input_DecoderFlush( decoder_t * );
VLC_API int  input_DecoderSetSpuHighlight( decoder_t *, const vlc_spu_highlight_t * );

/**
 * This function creates a sane filename path.
 */
VLC_API char * input_CreateFilename( input_thread_t *, input_item_t *,
                                     const char *psz_path, const char *psz_prefix,
                                     const char *psz_extension ) VLC_USED;

/**
 * It creates an empty input resource handler.
 *
 * The given object MUST stay alive as long as the input_resource_t is
 * not deleted.
 */
VLC_API input_resource_t * input_resource_New( vlc_object_t * ) VLC_USED;

/**
 * It releases an input resource.
 */
VLC_API void input_resource_Release( input_resource_t * );

/**
 * Forcefully destroys the video output (e.g. when the playlist is stopped).
 */
VLC_API void input_resource_TerminateVout( input_resource_t * );

/**
 * This function releases all resources (object).
 */
VLC_API void input_resource_Terminate( input_resource_t * );

/**
 * \return the current audio output if any.
 * Use vlc_object_release() to drop the reference.
 */
VLC_API audio_output_t *input_resource_HoldAout( input_resource_t * );

/**
 * This function creates or recycles an audio output.
 */
VLC_API audio_output_t *input_resource_GetAout( input_resource_t * );

/**
 * This function retains or destroys an audio output.
 */
VLC_API void input_resource_PutAout( input_resource_t *, audio_output_t * );

/**
 * Prevents the existing audio output (if any) from being recycled.
 */
VLC_API void input_resource_ResetAout( input_resource_t * );

/** @} */
#endif
