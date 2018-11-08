/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Authors: Jean Le Feuvre
 *			Copyright (c) Telecom ParisTech 2017-2018
 *					All rights reserved
 *
 *  This file is part of GPAC / gpac application
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <gpac/main.h>
#include <gpac/filters.h>
#include <time.h>

static GF_SystemRTInfo rti;
static u32 list_filters = 0;
static Bool dump_stats = GF_FALSE;
static Bool dump_graph = GF_FALSE;
static Bool print_filter_info = GF_FALSE;
static Bool print_meta_filters = GF_FALSE;
static Bool load_test_filters = GF_FALSE;
static s32 nb_loops = 0;

//the default set of separators
static char separator_set[7] = GF_FS_DEFAULT_SEPS;
#define SEP_LINK	5
#define SEP_FRAG	2

static void print_filters(int argc, char **argv, GF_FilterSession *session, GF_FilterArgMode argmode);
static void dump_all_props(void);
static void dump_all_codec(GF_FilterSession *session);
void write_filters_options(GF_FilterSession *fsess);

static void gpac_filter_help(void)
{
	fprintf(stderr,
#ifndef GPAC_DISABLE_DOC
"Usage: gpac [options] FILTER_DECL [LINK] FILTER_DECL [...] \n"
"This is GPAC's command line tool for setting up and running filter chains.\n"
"See -h for available options.\n"
"Help is given with default separator sets :=#,@. See -seps to change them.\n"
"\n"
"General\n"
"\n"
"Filters are configurable processing units consuming and producing data packets. These packets are carried "
"between filters through a data channel called pid.\n"
"A pid is in charge of allocating/tracking data packets, and passing the packets to the destination filter(s).\n"
"A filter output pid may be connected to zero or more filters. This fan-out is handled internally by "
"the filter engine (no such thing as a tee filter in GPAC).\n"
"When a pid cannot be connected to any filter, a warning is thrown and all packets dispatched on "
"this pid will be immediately destroyed. The session may however still run.\n"
"Each output pid carries a set of properties describing the data it delivers (eg width, height, codec, ...).\n"
"Properties can be built-in (identified by a 4 character code in GPAC), or user-defined (identified by a string).\n"
"Each pid tracks its properties changes and triggers filter reconfiguration during packet processing.\n"
"This allows the filter chain to be reconfigured at run time, potentially reloading part of the chain "
"(eg unload a video decoder when switching from compressed to uncompressed sources).\n"
"Each filter exposes one or more sets of capabilities, called capability bundle, which are property type and values "
"that must be matched or excluded in connecting pids.\n"
"Each filter also exposes a set of argument to configure itself, using property types and string values.\n"
"\n"
"Property format\n"
"\n"
"boolean properties are formatted as yes/true/1 or no/false/0\n"
"Enum properties (for arguments) must use the syntax given in the argument description, otherwise value 0 (first in enum) is assumed.\n"
"1-dimension properties (numbers, floats, ints...) are formatted as value[unit], where unit can be k/K (x1000) or m/M (x1000000) or g/G (x1000000000).\n"
"For such properties, value +I means maximum possible value, -I minimum possible value.\n"
"Fraction properties are formatted as num/den or num-den or num, in which case the denominator is 1.\n"
"Unsigned 32 bit integer properties can be specified in hexadecimal using the format 0xAABBCCDD.\n"
"N-dimension properties (vectors) are formatted as DIM1xDIM2[xDIM3[xDIM4]] values, without multiplier.\n"
"Data properties are formatted as:\n"
"\t\"size@address\": constant data block, not internally copied; size gives the size of the block, address the data pointer.\n"
"\t\"0xBYTESTRING\": data block specified in hexadecimal, internally copied.\n"
"Pointer properties are formatted as \"address\", address giving the pointer address (32 or 64 bit depending on platforms).\n"
"String and interget lists are formatted as val1,val2[,...]\n"
"\n"
"Note: Special characters in property formats (0x,/,-,+I,-I,x) cannot be configured.\n"
"\n"
"Filter declaration [FILTER_DECL]\n"
"\n"
"Filters are listed by name, with options appended as a list of colon-separated Name=Value pairs.\n"
"\tValue can be omitted for booleans, defaulting to true (eg :noedit). Using '!'before the name negates "
"the result (eg :!moof_first)\n"
"\tName can be omitted for enumerations (eg :disp=pbo is equivalent to :pbo), provided that filter developers pay attention to not reuse enum names in one filter!\n"
"\n"
"When string parameters are used (eg URLs), it is recommended to escape the string using the keword \"gpac\" \n"
"\tEX: \"filter:ARG=http://foo/bar?yes:gpac:opt=VAL\" will properly extract the URL\n"
"\tEX: \"filter:ARG=http://foo/bar?yes:opt=VAL\" will fail to extract it and keep :opt=VAL as part of the URL\n"
"Note: that the escape mechanism is not needed for local source, for which file existence is probed during argument parsing\n"
"\tIt is also not needed for builtin procotol handlers (avin://, video://, audio://, pipe://)\n"
"For tcp:// and udp:// protocols, the escape is not needed if a trailing / is appended after the port\n"
"\tEX: \"-i tcp://127.0.0.1:1234:OPT\" will fail to extract the URL and options\n"
"\tEX: \"-i tcp://127.0.0.1:1234/:OPT\" will extract the URL and options\n"
"Note: one trick to avoid the escape sequence is to declare the urls option at the end, eg f1:opt1=foo:url=http://bar, provided you have only one URL. See arguments inheriting below.\n"
"\n"
"Source and sink filters do not need to be addressed by the filter name, specifying src= or dst= instead is enough.\n"
"You can also use the syntax -src URL or -i URL for sources and -dst URL or -o URL for destination\n"
"This allows prompt completion in shells\n"
"\tEX: \"src=file.mp4\", or \"-src file.mp4\" will find a filter (for example \"fin\") able to load src\n"
"The same result can be achieved by using \"fin:src=file.mp4\"\n"
"\tEX: \"src=file.mp4 dst=dump.yuv\" will dump the video content of file.mp4 in dump.yuv\n"
"\tSpecific source or sink filters may also be specified using filterName:src=URL or filterName:dst=URL.\n"
"\n"
"There is a special option called \"gfreg\" which allows specifying preferred filters to use when handling URLs\n"
"\tEX: \"src=file.mp4:gfreg=ffdmx,ffdec\" will use ffdmx to handle the file and ffdec to decode\n"
"This can be used to test a specific filter when alternate filter chains are possible.\n"
"\n"
"By default filters chain will be resolved without any decoding/encoding if the destination accepts the desired format. "
"Otherwise, decoders/encoders will be dynamically loaded to perform the conversion, unless dynamic resolution is disabled. "
"There is a special shorcut filter name for encoders \"enc\" allowing to match a filter providing the desired encoding. "
"The parameters for enc are:\n"
"\tc=NAME identifes the desired codec. NAME can be the gpac codec name or the encoder instance for ffmpeg/others\n"
"\tb=UINT or rate=UINT or bitrate=UINT indicates the bitrate in bits per second\n"
"\tg=UINT or gop=UINT indicates the GOP size in frames\n"
"\tpfmt=4CC indicates the target pixel format of the source, if supported by codec\n"
"\tall_intra=BOOL indicates all frames should be intra frames, if supported by codec\n"

"Other options will be passed to the filter if it accepts generic argument parsing (as is the case for ffmpeg).\n"
"\tEX: \"src=dump.yuv:size=320x240:fps=25 enc:c=avc:b=150000:g=50:cgop=true:fast=true dst=raw.264\" creates a 25 fps AVC\n"
"at 175kbps with a gop duration of 2 seconds, using closed gop and fast encoding settings for ffmpeg\n"
"\n"
"Note that the inverse operation (forcing a decode to happen) is possible using the reframer filter:\n"
"\tEX: \"src=file.mp4 reframer:raw @ -o null\" will force decoding media from file and trash the result (for example, decoder benchmark)\n"
"\n"
"Expliciting links between filters [LINK]\n"
"\n"
"Link between filters may be manually specified. The syntax is an '@' character optionnaly followed by an integer (0 if omitted).\n"
"This indicates which previous (0-based) filters should be link to the next filter listed.\n"
"Only the last link directive occuring before a filter is used to setup links for that filter.\n"
"\tEX: \"fA fB @1 fC\" indicates to direct fA outputs to fC\n"
"\tEX: \"fA fB @1 @0 fC\" indicates to direct fB outputs to fC, @1 is ignored\n"
"\n"
"If no link directives are given, the links will be dynamically solved to fullfill as many connections as possible.\n"
"For example, \"fA fB fC\" may have fA linked to fB and fC if fB and fC accept fA outputs\n"
"\n"
"LINK directive is just a quick shortcut to set generic filter arguments:\n"
"- FID=name, which sets the Filter ID\n"
"- SID=name1[,name2...], which set the sourceIDs restricting the list of possible inputs for a filter.\n"
"\n"
"\tEX: \"fA fB @1 fC\" is equivalent to \"fA fB:FID=1 fC:SID=1\" \n"
"\tEX: \"fA:FID=1 fB fC:SID=1\" indicates to direct fA outputs to fC\n"
"\tEX: \"fA:FID=1 fB:FID=2 fC:SID=1 fD:SID=1,2\" indicates to direct fA outputs to fC, and fA and fB outputs to fD\n"
"\n"
"NOTE: A filter with sourceID set cannot get input from filters with no IDs.\n"
"The name can be further extended using fragment identifier (# by default):\n"
"\tname#PIDNAME: accepts only pid(s) with name PIDNAME\n"
"\tname#TYPE: accepts only pids of matching media type. TYPE can be 'audio' 'video' 'scene' 'text' 'font'\n"
"\tname#TYPEN: accepts only Nth pid of matching type from source\n"
"\tname#P4CC=VAL: accepts only pids with property matching VAL.\n"
"\tname#PName=VAL: same as above, using the builtin name corresponding to the property.\n"
"\tname#AnyName=VAL: same as above, using the name of a non built-in property.\n"
"\n"
"If the property is not defined on the pid, the property is matched. Otherwise, its value is checked agains the given value\n"
"\n"
"The following modifiers for comparisons are allowed (for both P4CC=, PName= and AnyName=):\n"
"\tname#P4CC=!VAL: accepts only pids with property NOT matching VAL.\n"
"\tname#P4CC-VAL: accepts only pids with property strictly less than VAL (only for 1-dimension number properties).\n"
"\tname#P4CC+VAL: accepts only pids with property strictly greater than VAL (only for 1-dimension number properties).\n"
"\n"
"Note that these extensions also work with the LINK shortcut:\n"
"\tEX: \"fA fB @1#video fC\" indicates to direct fA video outputs to fC\n"
"\n"
"\tEX: \"src=img.heif @#ItemID=200 vout\" indicates to connect to vout only pids with ItemID property equal to 200\n"
"\tEX: \"src=vid.mp4 @#PID=1 vout\" indicates to connect to vout only pids with ID property equal to 1\n"
"\tEX: \"src=vid.mp4 @#Width=640 vout\" indicates to connect to vout only pids with Width property equal to 640\n"
"\tEX: \"src=vid.mp4 @#Width-640 vout\" indicates to connect to vout only pids with Width property less than 640\n"
"\n"
"Multiple fragment can be specified to check for multiple pid properties\n"
"\tEX: \"src=vid.mp4 @#Width=640#Height+380 vout\" indicates to connect to vout only pids with Width property equal to 640 and Height greater than 380\n"
"\n"
"Note that if a filter pid gets connected to a loaded filter, no further dynamic link resolution will "
"be done to connect it to other filters. Link directives should be carfully setup\n"
"\tEX: src=file.mp4 @ reframer dst=dump.mp4\n"
"This will link src pid (type file) to dst (type file) because dst has no sourceID and therefore will "
"accept input from src. Since the pid is connected, the filter engine will not try to solve "
"a link between src and reframer. The result is a direct copy of the source file, reframer being unused\n"
"\tEX: src=file.mp4 reframer @ dst=dump.mp4\n"
"This will force dst to accept only from reframer, a muxer will be loaded to solve this link, and "
"src pid will be linked to reframer (no source ID), loading a demuxer to solve the link. The result is a complete remux of the source file\n"
"\n"
"Arguments inheriting\n"
"\n"
"Unless explicitly disabled (-max-chain option), the filter engine will resolve implicit or explicit (LINK) connections between filters "
"and will allocate any filter chain required to connect the filters. "
"In doing so, it loads new filters with arguments inherited from both the source and the destination.\n"
"\tEX: \"src=file.mp4:OPT dst=file.aac dst=file.264\" will pass the \":OPT\" to all filters loaded between the source and the two destinations\n"
"\tEX: \"src=file.mp4 dst=file.aac:OPT dst=file.264\" will pass the \":OPT\" to all filters loaded between the source and the file.aac destination\n"
"NOTE: the destination arguments inherited are the arguments placed AFTER the dst= option.\n"
"\tEX: \"src=file.mp4 fout:OPTFOO:dst=file.aac:OPTBAR\" will pass the \":OPTBAR\" to all filters loaded between the source and the file.aac destination, but not OPTFOO\n"
"\n"
"URL templating\n"
"\n"
"Destination URLs can be templated using the same mechanism as MPEG-DASH:\n"
"\t$KEYWORD$ is replaced in the template with the resolved value,\n"
"\t$KEYWORD%%0Nd$ is replaced in the template with the resolved integer, padded with N zeros if needed,\n"
"\t$$ is an escape for $\n"
"\n"
"KEYWORD may be present multiple times in the string. Supported KEYWORD (!! case sensitive !!) are:\n"
"\tnum: replaced by file number if defined, 0 otherwise\n"
"\tPID: ID of the source pid\n"
"\tURL: URL of source file\n"
"\tFile: path on disk for source file\n"
"\tp4cc=ABCD: uses pid property with 4CC value ABCD\n"
"\tpname=VAL: uses pid property with name VAL\n"
"\tOTHER: locates property 4CC for the given name, or property name if no 4CC matches.\n"
"\n"
"Templating can be usefull when encoding several qualities in one pass:\n"
"\tEX: src=dump.yuv:size=640x360 vcrop:wnd=0x0x320x180 enc:c=avc:b=1M @2 enc:c=avc:b=750k dst=dump_$CropOrigin$x$Width$x$Height$.264\n"
"This will create a croped version of the source, encoded in AVC at 1M, and a full version of the content in AVC at 750k "
"outputs will be dump_0x0x320x180.264 for the croped version and dump_0x0x640x360.264 for the non-croped one\n"
"\n"
"Cloning filters\n"
"\n"
"When a filter accepts a single connection and has a connected input, it is no longer available for dynamic resolution. "
"There may be cases where this behaviour is undesired. Take a HEIF file with N items and do:\n"
"\tEX: src=img.heif dst=dump_$ItemID$.jpg\n"
"In this case, only one item (likely the first declared in the file) will connect to the destination.\n"
"Other items will not be connected since the destination only accepts one input pid.\n"
"There is a special option \"clone\" allowing destination filters (only) to be cloned with the same arguments:\n"
"\tEX: src=img.heif dst=dump_$ItemID$.jpg:clone\n"
"In this case, the destination will be cloned for each item, and all will be exported to different JPEGs thanks to URL templating.\n"
"\n"
"Assigning PID properties\n"
"\n"
"It is possible to define properties on output pids that will be declared by a filter. This allows tagging parts of the "
"graph with different properties than other parts (for example ServiceID). "
"The syntax uses the fragment separator to identify properties, eg #Name=Value. "
"This sets output pids property (4cc, built-in name or any name) to the given value.\n"\
"If a non built-in property is used, the value will be delared as string.\n"
"WARNING: Properties are not filtered and override the source props, be carefull not to break the session by overriding core "
"properties such as width/height/samplerate/... !\n"
"\tEX: -i v1.mp4:#ServiceID=4 -i v2.mp4:#ServiceID=2 -o dump.ts\n"
"This will mux the streams in dump.ts, using ServiceID 4 for pids from v1.mp4 and ServiceID 1 for pids from v2.mp4\n"
"\n"
"External filters\n"
"GPAC comes with a set of built-in filters in libgpac. It may also load external filters in dynamic libraries, located in\n"
"folders listed in GPAC config file section \"core\" key \"mod-dirs\". The files have to be named gf_* and export a single function\n"
"returning a filter register - see libgpac documentation for more details.\n"
"\n"
#else
"GPAC compiled without built-in doc.\n"
#endif
	);

}


static void gpac_core_help(GF_FilterArgMode mode, Bool for_logs)
{
	u32 mask;
	fprintf(stderr, "libgpac %s options:\n", for_logs ? "logs" : "core");
	if (for_logs) {
		mask = GF_ARG_SUBSYS_LOG;
	} else {
		mask = 0xFFFFFFFF;
		mask &= ~GF_ARG_SUBSYS_LOG;
		mask &= ~GF_ARG_SUBSYS_FILTERS;
	}
	gf_sys_print_core_help(mode, mask);
}

GF_GPACArg gpac_args[] =
{
#ifdef GPAC_MEMORY_TRACKING
 	GF_DEF_ARG("mem-track", NULL, "enables memory tracker", NULL, NULL, GF_ARG_BOOL, GF_ARG_HINT_EXPERT),
 	GF_DEF_ARG("mem-track-stack", NULL, "enables memory tracker with stack dumping", NULL, NULL, GF_ARG_BOOL, GF_ARG_HINT_EXPERT),
#endif
 	GF_DEF_ARG("p", NULL, "uses indicated profile for the global GPAC config. If not found, config file is created. If a file path is indicated, tries to load profile from that file", NULL, NULL, GF_ARG_STRING, GF_ARG_HINT_ADVANCED),

	GF_DEF_ARG("ltf", NULL, "load test-unit filters (used for for unit tests only)", NULL, NULL, GF_ARG_BOOL, GF_ARG_HINT_EXPERT),
	GF_DEF_ARG("loop", NULL, "loops execution of session, creating a session at each loop, mainly used for testing. If no value is given, loops forever", NULL, NULL, GF_ARG_INT, GF_ARG_HINT_EXPERT),
	GF_DEF_ARG("wp", NULL, "writes all filter options in the config file", NULL, NULL, GF_ARG_BOOL, GF_ARG_HINT_EXPERT),
	GF_DEF_ARG("wpx", NULL, "writes all filter options and all meta filter arguments in the config file (large config file !)", NULL, NULL, GF_ARG_BOOL, GF_ARG_HINT_EXPERT),

	GF_DEF_ARG("stats", NULL, "print stats after execution. Stats can be viewed at runtime by typing 's' in the prompt", NULL, NULL, GF_ARG_BOOL, 0),
	GF_DEF_ARG("graph", NULL, "print graph after execution. Graph can be viewed at runtime by typing 'g' in the prompt", NULL, NULL, GF_ARG_BOOL, 0),
	GF_DEF_ARG("seps", NULL, "sets the default character sets used to seperate various arguments.\n"\
		"\tThe first char is used to seperate argument names\n"\
		"\tThe second char, if present, is used to seperate names and values\n"\
		"\tThe third char, if present, is used to seperate fragments for pid sources\n"\
		"\tThe fourth char, if present, is used for list separators (sourceIDs, gfreg, ...)\n"\
		"\tThe fifth char, if present, is used for boolean negation\n"\
		"\tThe sixth char, if present, is used for LINK directives (cf -h doc)\n", GF_FS_DEFAULT_SEPS, NULL, GF_ARG_STRING, GF_ARG_HINT_EXPERT),

	GF_DEF_ARG("i", "src", "specifies an input file - see filters help -h doc", NULL, NULL, GF_ARG_STRING, 0),
	GF_DEF_ARG("o", "dst", "specifies an output file - see filters help -h doc", NULL, NULL, GF_ARG_STRING, 0),
	GF_DEF_ARG("h", "help -ha -hx -hh", "prints help. By default only basic options are printed; -ha prints advanced options, -hx prints expert options and -hh prints all. String parameter can be:\n"\
			"\tempty: prints command line options help.\n"\
			"\tdoc: prints the general filter info.\n"\
			"\tlog: prints the log system help.\n"\
			"\tcore: prints the supported libgpac core options. Use -ha/-hx/-hh for advanced/expert options.\n"\
			"\tfilters: prints name of all available filters.\n"\
			"\tfilters:*: prints name of all available filters, including meta filters.\n"\
			"\tcodecs: prints the supported builtin codecs.\n"\
			"\tprops: prints the supported builtin pid properties.\n"\
			"\tlinks: prints possible connections between each supported filters.\n"\
			"\tFNAME: prints filter FNAME info (multiple FNAME can be given). For meta-filters, use FNAME:INST, eg ffavin:avfoundation. Use * to print info on all filters (warning, big output!), *:* to print info on all filters including meta filter instances (warning, big big output!). By default only basic filter options and description are shown. Use -ha to show advanced options and filter IO capabilities, -hx for expert options, -hh for all options\n"\
		, NULL, NULL, GF_ARG_STRING, 0),

	{0}
};


static void gpac_usage(GF_FilterArgMode argmode)
{
	u32 i=0;
	if ((argmode != GF_ARGMODE_ADVANCED) && (argmode != GF_ARGMODE_EXPERT) ) {
		fprintf(stderr, "Usage: gpac [options] FILTER_DECL [LINK] FILTER_DECL [...] \n"
			"gpac is GPAC's command line tool for setting up and running filter chains. Options do not require any specific order.\n\n"
		);
	}

	while (gpac_args[i].name) {
		GF_GPACArg *arg = &gpac_args[i];
		i++;

		if (argmode!=GF_ARGMODE_ALL) {
			if ((argmode==GF_ARGMODE_EXPERT) && !(arg->flags & GF_ARG_HINT_EXPERT)) continue;
			if ((argmode==GF_ARGMODE_ADVANCED) && !(arg->flags & GF_ARG_HINT_ADVANCED)) continue;
			else if ((argmode==GF_ARGMODE_BASE) && (arg->flags & (GF_ARG_HINT_ADVANCED| GF_ARG_HINT_EXPERT)) ) continue;
		}

		gf_sys_print_arg(arg);
	}

	if (argmode>=GF_ARGMODE_ADVANCED) {
		fprintf(stderr, "The following options are part of libgpac core and can also be assigned though the config file from section \"core\" using option name without first '-' as key name. For example, having [core]threads=2 in the config file is similar as using -threads=2. The option specified at prompt overrides the value of the config file.\n\n");

		gf_sys_print_core_help(argmode, GF_ARG_SUBSYS_FILTERS);
	}

	if (argmode==GF_ARGMODE_BASE) {
		fprintf(stderr, "gpac - GPAC command line filter engine - version "GPAC_FULL_VERSION"\n"
	        "Written by Jean Le Feuvre (c) Telecom ParisTech 2017-2018\n"
		);
	}
}


static Bool gpac_fsess_task(GF_FilterSession *fsess, void *callback, u32 *reschedule_ms)
{
	if (gf_prompt_has_input()) {
		char c = gf_prompt_get_char();
		switch (c) {
		case 'q':
			gf_fs_abort(fsess);
			nb_loops = 0;
			return GF_FALSE;
		case 's':
			gf_fs_print_stats(fsess);
			break;
		case 'g':
			gf_fs_print_connections(fsess);
			break;
		default:
			break;
		}
	}
	if (gf_fs_is_last_task(fsess))
		return GF_FALSE;
	*reschedule_ms = 500;
	return GF_TRUE;
}

static void parse_sep_set(const char *arg, Bool *override_seps)
{
	if (!arg) return;
	u32 len = (u32) strlen(arg);
	if (!len) return;
	*override_seps = GF_TRUE;
	if (len>=1) separator_set[0] = arg[0];
	if (len>=2) separator_set[1] = arg[1];
	if (len>=3) separator_set[2] = arg[2];
	if (len>=4) separator_set[3] = arg[3];
	if (len>=5) separator_set[4] = arg[4];
	if (len>=6) separator_set[5] = arg[5];
}

static int gpac_main(int argc, char **argv)
{
	GF_Err e=GF_OK;
	int i;
	const char *profile=NULL;
	u32 sflags=0;
	Bool override_seps=GF_FALSE;
	Bool write_profile=GF_FALSE;
	s32 link_prev_filter = -1;
	char *link_prev_filter_ext=NULL;
	GF_List *loaded_filters=NULL;
	GF_FilterArgMode argmode = GF_ARGMODE_BASE;
	u32 nb_filters = 0;
	Bool nothing_to_do = GF_TRUE;
	GF_MemTrackerType mem_track=GF_MemTrackerNone;
	GF_FilterSession *session;
	Bool view_filter_conn = GF_FALSE;
	Bool dump_codecs = GF_FALSE;

	//look for mem track and profile, and also process all helpers
	for (i=1; i<argc; i++) {
		char *arg = argv[i];
		if (!strcmp(arg, "-mem-track") || !strcmp(arg, "-mem-track-stack")) {
#ifdef GPAC_MEMORY_TRACKING
            mem_track = !strcmp(arg, "-mem-track-stack") ? GF_MemTrackerBackTrace : GF_MemTrackerSimple;
#else
			fprintf(stderr, "WARNING - GPAC not compiled with Memory Tracker - ignoring \"%s\"\n", arg);
#endif
		} else if (!strncmp(arg, "-p=", 3)) {
			profile = arg+3;
		}
		else if (!strcmp(arg, "-h") || !strcmp(arg, "-help") || !strcmp(arg, "-ha") || !strcmp(arg, "-hx") || !strcmp(arg, "-hh")) {
			if (!strcmp(arg, "-ha")) argmode = GF_ARGMODE_ADVANCED;
			else if (!strcmp(arg, "-hx")) argmode = GF_ARGMODE_EXPERT;
			else if (!strcmp(arg, "-hh")) argmode = GF_ARGMODE_ALL;

			if (i+1==argc) {
				gpac_usage(argmode);
				return 0;
			} else if (!strcmp(argv[i+1], "log")) {
				gpac_core_help(GF_ARGMODE_ALL, GF_TRUE);
				return 0;
			} else if (!strcmp(argv[i+1], "core")) {
				gpac_core_help(argmode, GF_FALSE);
				return 0;
			} else if (!strcmp(argv[i+1], "doc")) {
				gpac_filter_help();
				return 0;
			} else if (!strcmp(argv[i+1], "props")) {
				dump_all_props();
				return 0;
			} else if (!strcmp(argv[i+1], "codecs")) {
				dump_codecs = GF_TRUE;
				i++;
			} else if (!strcmp(argv[i+1], "links")) {
				view_filter_conn = GF_TRUE;
				i++;
			} else if (!strcmp(argv[i+1], "bin")) {
				fprintf(stderr, "GPAC binary information:\n"\
				 	"Version "GPAC_FULL_VERSION"\n"\
	        		"Compilation configuration: " GPAC_CONFIGURATION "\n"\
	        		"Enabled features: %s\n" \
	        		"Disabled features: %s\n", gpac_enabled_features(), gpac_disabled_features()
				);
				return 0;
			} else if (!strcmp(argv[i+1], "filters")) {
				list_filters = 1;
				i++;
			} else if (!strcmp(argv[i+1], "filters:*")) {
				list_filters = 2;
				i++;
			} else {
				print_filter_info = 1;
			}
		}
	}
	gf_sys_init(mem_track, profile);

	gf_log_set_tool_level(GF_LOG_ALL, GF_LOG_WARNING);
	gf_log_set_tool_level(GF_LOG_APP, GF_LOG_INFO);

	//this will parse default args
	e = gf_sys_set_args(argc, (const char **) argv);
	if (e) {
		fprintf(stderr, "Error assigning libgpac arguments: %s\n", gf_error_to_string(e) );
		gf_sys_close();
		return 1;
	}

	for (i=1; i<argc; i++) {
		char *arg = argv[i];
		char *arg_val = strchr(arg, '=');
		if (arg_val) {
			arg_val[0]=0;
			arg_val++;
		}

		if (!strcmp(arg, "-ltf")) {
			load_test_filters = GF_TRUE;
		} else if (!strcmp(arg, "-stats")) {
			dump_stats = GF_TRUE;
		} else if (!strcmp(arg, "-graph")) {
			dump_graph = GF_TRUE;
		} else if (strstr(arg, ":*") && list_filters) {
			list_filters = 3;
		} else if (strstr(arg, ":*")) {
			print_meta_filters = GF_TRUE;
		} else if (!strcmp(arg, "-wp")) {
			write_profile = GF_TRUE;
		} else if (!strcmp(arg, "-wpx")) {
			write_profile = GF_TRUE;
			sflags |= GF_FS_FLAG_LOAD_META;
		} else if (!strcmp(arg, "-loop")) {
			nb_loops = -1;
			if (arg_val) nb_loops = atoi(arg_val);
		}
		else if (!strcmp(arg, "-cfg")) {
			nothing_to_do = GF_FALSE;
		}
		else if (!strncmp(arg, "-seps=", 3)) {
			parse_sep_set(arg_val, &override_seps);
		} else if (!strcmp(arg, "-h") || !strcmp(arg, "-help") || !strcmp(arg, "-ha") || !strcmp(arg, "-hx") || !strcmp(arg, "-hh")) {
			i++;
		} else if (!strcmp(arg, "-mem-track") || !strcmp(arg, "-mem-track-stack")) {

		} else if (arg[0]=='-') {
			if (!strcmp(arg, "-i") || !strcmp(arg, "-src") || !strcmp(arg, "-o") || !strcmp(arg, "-dst") ) {
			} else if (!gf_sys_is_gpac_arg(arg) ) {
				GF_LOG(GF_LOG_ERROR, GF_LOG_APP, ("Unrecognized option \"%s\", check usage \"gpac -h\"\n", arg));
				gf_sys_close();
				return 1;
			}
		}

		if (arg_val) {
			arg_val--;
			arg_val[0]='=';
		}
	}

	if (dump_stats && gf_sys_get_rti(0, &rti, 0) ) {
		GF_LOG(GF_LOG_INFO, GF_LOG_APP, ("System info: %d MB RAM - %d cores\n", (u32) (rti.physical_memory/1024/1024), rti.nb_cores));
	}
	if ((list_filters>=2) || print_meta_filters || dump_codecs || print_filter_info) sflags |= GF_FS_FLAG_LOAD_META;

restart:

	session = gf_fs_new_defaults(sflags);

	if (!session) {
		return 1;
	}
	if (override_seps) gf_fs_set_separators(session, separator_set);
	if (load_test_filters) gf_fs_register_test_filters(session);

	if (gf_fs_get_max_resolution_chain_length(session) <= 1 ) {
		GF_LOG(GF_LOG_INFO, GF_LOG_APP, ("\nDynamic resolution of filter connections disabled\n\n"));
	}

	if (list_filters || print_filter_info) {
		print_filters(argc, argv, session, argmode);
		goto exit;
	}
	if (view_filter_conn) {
		gf_fs_print_all_connections(session);
		goto exit;
	}
	if (dump_codecs) {
		dump_all_codec(session);
		goto exit;
	}
	if (write_profile) {
		write_filters_options(session);
		goto exit;
	}

	//all good to go, load filters
	loaded_filters = gf_list_new();
	for (i=1; i<argc; i++) {
		GF_Filter *filter=NULL;
		Bool is_simple=GF_FALSE;
		Bool f_loaded = GF_FALSE;
		char *arg = argv[i];

		if (!strcmp(arg, "-src") || !strcmp(arg, "-i") ) {
			filter = gf_fs_load_source(session, argv[i+1], NULL, NULL, &e);
			arg = argv[i+1];
			i++;
			f_loaded = GF_TRUE;
		} else if (!strcmp(arg, "-dst") || !strcmp(arg, "-o") ) {
			filter = gf_fs_load_destination(session, argv[i+1], NULL, NULL, &e);
			arg = argv[i+1];
			i++;
			f_loaded = GF_TRUE;
		}
		//appart from the above src/dst, other args starting with - are not filters
		else if (arg[0]=='-') {
			continue;
		}
		if (!f_loaded) {
			if (arg[0]== separator_set[SEP_LINK] ) {
				char *ext = strchr(arg, separator_set[SEP_FRAG]);
				if (ext) {
					ext[0] = 0;
					link_prev_filter_ext = ext+1;
				}
				link_prev_filter = 0;
				if (strlen(arg)>1) {
					link_prev_filter = atoi(arg+1);
					if (link_prev_filter<0) {
						GF_LOG(GF_LOG_ERROR, GF_LOG_APP, ("Wrong filter index %d, must be positive\n", link_prev_filter));
						e = GF_BAD_PARAM;
						goto exit;
					}
				}

				if (ext) ext[0] = separator_set[SEP_FRAG];
				continue;
			}

			if (!strncmp(arg, "src=", 4) ) {
				filter = gf_fs_load_source(session, arg+4, NULL, NULL, &e);
			} else if (!strncmp(arg, "dst=", 4) ) {
				filter = gf_fs_load_destination(session, arg+4, NULL, NULL, &e);
			} else {
				filter = gf_fs_load_filter(session, arg);
				is_simple=GF_TRUE;
			}
		}

		if (!filter) {
			 GF_LOG(GF_LOG_ERROR, GF_LOG_APP, ("Failed to load filter%s %s\n", is_simple ? "" : " for",  arg));
			if (!e) e = GF_NOT_SUPPORTED;
			goto exit;
		}
		nb_filters++;

		if (link_prev_filter>=0) {
			GF_Filter *link_from = gf_list_get(loaded_filters, gf_list_count(loaded_filters)-1-link_prev_filter);
			if (!link_from) {
				GF_LOG(GF_LOG_ERROR, GF_LOG_APP, ("Wrong filter index @%d\n", link_prev_filter));
				e = GF_BAD_PARAM;
				goto exit;
			}
			link_prev_filter = -1;
			gf_filter_set_source(filter, link_from, link_prev_filter_ext);
			link_prev_filter_ext = NULL;
		}

		gf_list_add(loaded_filters, filter);
	}
	if (!gf_list_count(loaded_filters)) {
		if (nothing_to_do) {
			GF_LOG(GF_LOG_INFO, GF_LOG_APP, ("Nothing to do, check usage \"gpac -h\"\ngpac - GPAC command line filter engine - version "GPAC_FULL_VERSION"\n"));
			e = GF_BAD_PARAM;
		} else {
			e = GF_EOS;
		}
		goto exit;
	}
	GF_LOG(GF_LOG_INFO, GF_LOG_APP, ("Running session, press 'q' to abort\n"));

	gf_fs_post_user_task(session, gpac_fsess_task, NULL, "gpac_fsess_task");
	e = gf_fs_run(session);
	if (e>0) e = GF_OK;
	if (!e) e = gf_fs_get_last_connect_error(session);
	if (!e) e = gf_fs_get_last_process_error(session);

exit:
	if (e && nb_filters) {
		gf_fs_run(session);
	}
	if (dump_stats)
		gf_fs_print_stats(session);
	if (dump_graph)
		gf_fs_print_connections(session);

	gf_fs_del(session);
	if (loaded_filters) gf_list_del(loaded_filters);

	if (!e && (nb_loops>0) ) {
		nb_loops--;
		goto restart;
	}

	gf_sys_close();
	if (e<0) return 1;

#ifdef GPAC_MEMORY_TRACKING
	if (gf_memory_size() || gf_file_handles_count() ) {
		gf_log_set_tool_level(GF_LOG_MEMORY, GF_LOG_INFO);
		gf_memory_print();
		return 2;
	}
#endif
	return 0;
}

GF_MAIN_FUNC(gpac_main)


static void dump_caps(u32 nb_caps, const GF_FilterCapability *caps)
{
	u32 i;
	for (i=0;i<nb_caps; i++) {
		const char *szName;
		const char *szVal;
		char szDump[GF_PROP_DUMP_ARG_SIZE];
		const GF_FilterCapability *cap = &caps[i];
		if (!(cap->flags & GF_CAPFLAG_IN_BUNDLE) && i+1==nb_caps) break;
		if (!i) fprintf(stderr, "Capabilities Bundle:\n");
		else if (!(cap->flags & GF_CAPFLAG_IN_BUNDLE) ) {
			fprintf(stderr, "Capabilities Bundle:\n");
			continue;
		}

		szName = cap->name ? cap->name : gf_props_4cc_get_name(cap->code);
		if (!szName) szName = gf_4cc_to_str(cap->code);
		fprintf(stderr, "\t Flags:");
		if (cap->flags & GF_CAPFLAG_INPUT) fprintf(stderr, " Input");
		if (cap->flags & GF_CAPFLAG_OUTPUT) fprintf(stderr, " Output");
		if (cap->flags & GF_CAPFLAG_EXCLUDED) fprintf(stderr, " Exclude");
		if (cap->flags & GF_CAPFLAG_LOADED_FILTER) fprintf(stderr, " LoadedFilterOnly");

		//dump some interesting predefined ones which are not mapped to types
		if (cap->code==GF_PROP_PID_STREAM_TYPE) szVal = gf_stream_type_name(cap->val.value.uint);
		else if (cap->code==GF_PROP_PID_CODECID) szVal = (const char *) gf_codecid_name(cap->val.value.uint);
		else szVal = gf_prop_dump_val(&cap->val, szDump, GF_FALSE, NULL);

		fprintf(stderr, " - Type: %s - Value: %s", szName,  szVal);
		if (cap->priority) fprintf(stderr, ", priority=%d", cap->priority);
		fprintf(stderr, "\n");
	}
}

static void print_filter(const GF_FilterRegister *reg, GF_FilterArgMode argmode)
{
	fprintf(stderr, "Name: %s\n", reg->name);
#ifndef GPAC_DISABLE_DOC
	if (reg->description) fprintf(stderr, "Description: %s\n", reg->description);
	if (reg->author) fprintf(stderr, "Author: %s\n", reg->author);
	if (reg->help) fprintf(stderr, "\n%s\n\n", reg->help);
#else
	fprintf(stderr, "GPAC compiled without built-in doc\n");
#endif

	if (argmode==GF_ARGMODE_EXPERT) {
		if (reg->max_extra_pids==(u32) -1) fprintf(stderr, "Max Input pids: any\n");
		else fprintf(stderr, "Max Input pids: %d\n", 1 + reg->max_extra_pids);

		fprintf(stderr, "Flags:");
		if (reg->flags & GF_FS_REG_EXPLICIT_ONLY) fprintf(stderr, " ExplicitOnly");
		if (reg->flags & GF_FS_REG_MAIN_THREAD) fprintf(stderr, " MainThread");
		if (reg->flags & GF_FS_REG_CONFIGURE_MAIN_THREAD) fprintf(stderr, " ConfigureMainThread");
		if (reg->flags & GF_FS_REG_HIDE_WEIGHT) fprintf(stderr, " HideWeight");
		if (reg->flags & GF_FS_REG_DYNLIB) fprintf(stderr, " DynamicLib");

		if (reg->probe_url) fprintf(stderr, " IsSource");
		if (reg->reconfigure_output) fprintf(stderr, " ReconfigurableOutput");
		if (reg->probe_data) fprintf(stderr, " DataProber");
		fprintf(stderr, "\nPriority %d", reg->priority);

		fprintf(stderr, "\n");
	}


	if (reg->args) {
		u32 idx=0;
		switch (argmode) {
		case GF_ARGMODE_ALL:
		case GF_ARGMODE_EXPERT:
			fprintf(stderr, "Options (expert):\n");
			break;
		case GF_ARGMODE_ADVANCED:
			fprintf(stderr, "Options (advanced):\n");
			break;
		case GF_ARGMODE_BASE:
			fprintf(stderr, "Options (basic):\n");
			break;
		}

		while (1) {
			Bool print = GF_FALSE;
			const GF_FilterArgs *a = & reg->args[idx];
			if (!a || !a->arg_name) break;
			idx++;
			switch (argmode) {
			case GF_ARGMODE_ALL:
			case GF_ARGMODE_EXPERT:
			 	print = GF_TRUE;
			 	break;
			case GF_ARGMODE_ADVANCED:
			 	if (!(a->flags & GF_FS_ARG_HINT_EXPERT)) print = GF_TRUE;
			 	break;
			case GF_ARGMODE_BASE:
			 	if (!(a->flags & (GF_FS_ARG_HINT_EXPERT|GF_FS_ARG_HINT_ADVANCED)) ) print = GF_TRUE;
			 	break;
			}
			if (!print) continue;

			fprintf(stderr, "\t%s (%s): %s.", a->arg_name, gf_props_get_type_name(a->arg_type), a->arg_desc);
			if (a->arg_default_val) {
				fprintf(stderr, " Default %s.", a->arg_default_val);
			} else {
				fprintf(stderr, " No default.");
			}
			if (a->min_max_enum) {
				fprintf(stderr, " %s: %s", strchr(a->min_max_enum, '|') ? "Enum" : "minmax", a->min_max_enum);
			}
			if (a->flags & GF_FS_ARG_UPDATE) fprintf(stderr, " Updatable");
			if (a->flags & GF_FS_ARG_META) fprintf(stderr, " Meta");
			fprintf(stderr, "\n");
		}
	} else {
		fprintf(stderr, "No options\n");
	}

	if (reg->nb_caps) {
		if (argmode>=GF_ARGMODE_ADVANCED) {
			dump_caps(reg->nb_caps, reg->caps);
		}
	}
	fprintf(stderr, "\n");
}

static void print_filters(int argc, char **argv, GF_FilterSession *session, GF_FilterArgMode argmode)
{
	Bool found = GF_FALSE;
	u32 i, count = gf_fs_filters_registry_count(session);
	if (list_filters) fprintf(stderr, "Listing %d supported filters%s:\n", count, (list_filters==2) ? " including meta-filters" : "");
	for (i=0; i<count; i++) {
		const GF_FilterRegister *reg = gf_fs_get_filter_registry(session, i);
		if (print_filter_info) {
			u32 k;
			//all good to go, load filters
			for (k=1; k<(u32) argc; k++) {
				char *arg = argv[k];
				if (arg[0]=='-') continue;

				if (!strcmp(arg, reg->name) ) {
					print_filter(reg, argmode);
					found = GF_TRUE;
				} else {
					char *sep = strchr(arg, ':');
					if (!strcmp(arg, "*:*")
						|| (!sep && !strcmp(arg, "*"))
					 	|| (sep && !strcmp(sep, ":*") && !strncmp(reg->name, arg, 1+sep - arg) )
					) {
						print_filter(reg, argmode);
						found = GF_TRUE;
						break;
					}
				}
			}
		} else {
#ifndef GPAC_DISABLE_DOC
			fprintf(stderr, "%s: %s\n", reg->name, reg->description);
#else
			fprintf(stderr, "%s (compiled without built-in doc)\n", reg->name);
#endif
			found = GF_TRUE;

		}
	}
	if (!found) fprintf(stderr, "No such filter\n");
}

static void dump_all_props(void)
{
	u32 i=0;
	const GF_BuiltInProperty *prop_info;

	fprintf(stderr, "Built-in properties for PIDs and packets \"Name (4CC type FLAGS): description\"\nFLAGS can be D (dropable - cf gsfm help), P (packet)\n\n");
	while ((prop_info = gf_props_get_description(i))) {
		i++;
		char szFlags[10];
		if (! prop_info->name) continue;

		szFlags[0]=0;
		if (prop_info->flags & GF_PROP_FLAG_GSF_REM) strcat(szFlags, "D");
		if (prop_info->flags & GF_PROP_FLAG_PCK) strcat(szFlags, "P");

		fprintf(stderr, "%s (%s %s %s): %s", prop_info->name, gf_4cc_to_str(prop_info->type), gf_props_get_type_name(prop_info->data_type), szFlags, prop_info->description);

		if (prop_info->data_type==GF_PROP_PIXFMT) {
			fprintf(stderr, "\n\tNames: %s\n\tFile extensions: %s", gf_pixel_fmt_all_names(), gf_pixel_fmt_all_shortnames() );
		} else if (prop_info->data_type==GF_PROP_PCMFMT) {
			fprintf(stderr, "\n\tNames: %s\n\tFile extensions: %s", gf_audio_fmt_all_names(), gf_audio_fmt_all_shortnames() );
		}
		fprintf(stderr, "\n");
	}
}

static void dump_all_codec(GF_FilterSession *session)
{
	GF_PropertyValue rawp;
	GF_PropertyValue cp;
	u32 cidx=0;
	u32 count = gf_fs_filters_registry_count(session);
	fprintf(stderr, "Codec names listed as built_in_name[|variant](I: Filter Input support, O: Filter Output support): full_name\n");
	rawp.type = cp.type = GF_PROP_UINT;
	rawp.value.uint = GF_CODECID_RAW;
	while (1) {
		u32 i;
		const char *lname;
		const char *sname;
		Bool enc_found = GF_FALSE;
		Bool dec_found = GF_FALSE;
		cp.value.uint = gf_codecid_enum(cidx, &sname, &lname);
		cidx++;
		if (cp.value.uint == GF_CODECID_NONE) break;
		if (cp.value.uint == GF_CODECID_RAW) continue;
		if (!sname) break;

		for (i=0; i<count; i++) {
			const GF_FilterRegister *reg = gf_fs_get_filter_registry(session, i);
			if ( gf_fs_check_registry_cap(reg, GF_PROP_PID_CODECID, &rawp, GF_PROP_PID_CODECID, &cp)) enc_found = GF_TRUE;
			if ( gf_fs_check_registry_cap(reg, GF_PROP_PID_CODECID, &cp, GF_PROP_PID_CODECID, &rawp)) dec_found = GF_TRUE;
		}

		fprintf(stderr, "%s (%c%c): %s\n", sname, dec_found ? 'I' : '-', enc_found ? 'O' : '-', lname);
	}
	fprintf(stderr, "\n");
}

void write_filters_options(GF_FilterSession *fsess)
{
	u32 i, count;
	count = gf_fs_filters_registry_count(fsess);
	for (i=0; i<count; i++) {
		char *meta_sep;
		u32 j=0;
		char szSecName[200];

		const GF_FilterRegister *freg = gf_fs_get_filter_registry(fsess, i);
		sprintf(szSecName, "filter@%s", freg->name);
		meta_sep = strchr(szSecName + 7, ':');
		if (meta_sep) meta_sep[0] = 0;

		while (freg->args) {
			const GF_FilterArgs *arg = &freg->args[j];
			if (!arg || !arg->arg_name) break;
			j++;

			if (arg->arg_default_val) {
				gf_opts_set_key(szSecName, arg->arg_name, arg->arg_default_val);
			}
		}
	}
}
