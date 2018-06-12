/*
 * libOPNMIDI is a free MIDI to WAV conversion library with OPN2 (YM2612) emulation
 *
 * MIDI parser and player (Original code from ADLMIDI): Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * OPNMIDI Library and YM2612 support:   Copyright (c) 2017-2018 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Library is based on the ADLMIDI, a MIDI player for Linux and Windows with OPL3 emulation:
 * http://iki.fi/bisqwit/source/adlmidi.html
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "opnmidi_private.hpp"

#define MaxCards 100
#define MaxCards_STR "100"

static OPN2_Version opn2_version = {
    OPNMIDI_VERSION_MAJOR,
    OPNMIDI_VERSION_MINOR,
    OPNMIDI_VERSION_PATCHLEVEL
};

static const OPNMIDI_AudioFormat opn2_DefaultAudioFormat =
{
    OPNMIDI_SampleType_S16,
    sizeof(int16_t),
    2 * sizeof(int16_t),
};

/*---------------------------EXPORTS---------------------------*/

OPNMIDI_EXPORT struct OPN2_MIDIPlayer *opn2_init(long sample_rate)
{
    OPN2_MIDIPlayer *midi_device;
    midi_device = (OPN2_MIDIPlayer *)malloc(sizeof(OPN2_MIDIPlayer));
    if(!midi_device)
    {
        OPN2MIDI_ErrorString = "Can't initialize OPNMIDI: out of memory!";
        return NULL;
    }

    OPNMIDIplay *player = new OPNMIDIplay(static_cast<unsigned long>(sample_rate));
    if(!player)
    {
        free(midi_device);
        OPN2MIDI_ErrorString = "Can't initialize OPNMIDI: out of memory!";
        return NULL;
    }
    midi_device->opn2_midiPlayer = player;
    opn2RefreshNumCards(midi_device);
    return midi_device;
}

OPNMIDI_EXPORT int opn2_setNumChips(OPN2_MIDIPlayer *device, int numCards)
{
    if(device == NULL)
        return -2;

    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.NumCards = static_cast<unsigned int>(numCards);
    if(play->m_setup.NumCards < 1 || play->m_setup.NumCards > MaxCards)
    {
        play->setErrorString("number of chips may only be 1.." MaxCards_STR ".\n");
        return -1;
    }

    play->opn.NumCards = play->m_setup.NumCards;
    opn2_reset(device);

    return opn2RefreshNumCards(device);
}

OPNMIDI_EXPORT int opn2_getNumChips(struct OPN2_MIDIPlayer *device)
{
    if(device == NULL)
        return -2;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return (int)play->m_setup.NumCards;
    return -2;
}

OPNMIDI_EXPORT int opn2_openBankFile(OPN2_MIDIPlayer *device, const char *filePath)
{
    if(device && device->opn2_midiPlayer)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
        play->m_setup.tick_skip_samples_delay = 0;
        if(!play->LoadBank(filePath))
        {
            std::string err = play->getErrorString();
            if(err.empty())
                play->setErrorString("OPN2 MIDI: Can't load file");
            return -1;
        }
        else return opn2RefreshNumCards(device);
    }
    OPN2MIDI_ErrorString = "Can't load file: OPN2 MIDI is not initialized";
    return -1;
}

OPNMIDI_EXPORT int opn2_openBankData(OPN2_MIDIPlayer *device, const void *mem, long size)
{
    if(device && device->opn2_midiPlayer)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
        play->m_setup.tick_skip_samples_delay = 0;
        if(!play->LoadBank(mem, static_cast<size_t>(size)))
        {
            std::string err = play->getErrorString();
            if(err.empty())
                play->setErrorString("OPN2 MIDI: Can't load data from memory");
            return -1;
        }
        else return 0;
    }

    OPN2MIDI_ErrorString = "Can't load file: OPN2 MIDI is not initialized";
    return -1;
}

OPNMIDI_EXPORT void opn2_setScaleModulators(OPN2_MIDIPlayer *device, int smod)
{
    if(!device) return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.ScaleModulators = smod;
    play->opn.ScaleModulators = (play->m_setup.ScaleModulators != 0);
}

OPNMIDI_EXPORT void opn2_setFullRangeBrightness(struct OPN2_MIDIPlayer *device, int fr_brightness)
{
    if(!device) return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.fullRangeBrightnessCC74 = (fr_brightness != 0);
}

OPNMIDI_EXPORT void opn2_setLoopEnabled(OPN2_MIDIPlayer *device, int loopEn)
{
    if(!device) return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.loopingIsEnabled = (loopEn != 0);
}

/* !!!DEPRECATED!!! */
OPNMIDI_EXPORT void opn2_setLogarithmicVolumes(struct OPN2_MIDIPlayer *device, int logvol)
{
    if(!device) return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.LogarithmicVolumes = static_cast<unsigned int>(logvol);
    if(play->m_setup.LogarithmicVolumes != 0)
        play->opn.ChangeVolumeRangesModel(OPNMIDI_VolumeModel_CMF);
    else
        play->opn.ChangeVolumeRangesModel(static_cast<OPNMIDI_VolumeModels>(play->m_setup.VolumeModel));
}

OPNMIDI_EXPORT void opn2_setVolumeRangeModel(OPN2_MIDIPlayer *device, int volumeModel)
{
    if(!device) return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.VolumeModel = volumeModel;
    play->opn.ChangeVolumeRangesModel(static_cast<OPNMIDI_VolumeModels>(volumeModel));
}

OPNMIDI_EXPORT int opn2_openFile(OPN2_MIDIPlayer *device, const char *filePath)
{
    if(device && device->opn2_midiPlayer)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
        play->m_setup.tick_skip_samples_delay = 0;
        if(!play->LoadMIDI(filePath))
        {
            std::string err = play->getErrorString();
            if(err.empty())
                play->setErrorString("OPN2 MIDI: Can't load file");
            return -1;
        }
        else return 0;
#else
        (void)filePath;
        play->setErrorString("OPNMIDI: MIDI Sequencer is not supported in this build of library!");
        return -1;
#endif
    }

    OPN2MIDI_ErrorString = "Can't load file: OPN2 MIDI is not initialized";
    return -1;
}

OPNMIDI_EXPORT int opn2_openData(OPN2_MIDIPlayer *device, const void *mem, unsigned long size)
{
    if(device && device->opn2_midiPlayer)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
        play->m_setup.tick_skip_samples_delay = 0;
        if(!play->LoadMIDI(mem, static_cast<size_t>(size)))
        {
            std::string err = play->getErrorString();
            if(err.empty())
                play->setErrorString("OPN2 MIDI: Can't load data from memory");
            return -1;
        }
        else return 0;
#else
        (void)mem;(void)size;
        play->setErrorString("OPNMIDI: MIDI Sequencer is not supported in this build of library!");
        return -1;
#endif
    }

    OPN2MIDI_ErrorString = "Can't load file: OPN2 MIDI is not initialized";
    return -1;
}

OPNMIDI_EXPORT const char *opn2_emulatorName()
{
    return "<opn2_emulatorName() is deprecated! Use opn2_chipEmulatorName() instead!>";
}

OPNMIDI_EXPORT const char *opn2_chipEmulatorName(struct OPN2_MIDIPlayer *device)
{
    if(device)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
        if(play && !play->opn.cardsOP2.empty())
            return play->opn.cardsOP2[0]->emulatorName();
    }
    return "Unknown";
}

OPNMIDI_EXPORT int opn2_switchEmulator(struct OPN2_MIDIPlayer *device, int emulator)
{
    if(device)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
        if(play && (emulator >= 0) && (emulator < OPNMIDI_EMU_end))
        {
            play->m_setup.emulator = emulator;
            opn2_reset(device);
            return 0;
        }
        play->setErrorString("OPN2 MIDI: Unknown emulation core!");
    }
    return -1;
}


OPNMIDI_EXPORT int opn2_setRunAtPcmRate(OPN2_MIDIPlayer *device, int enabled)
{
    if(device)
    {
        OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
        if(play)
        {
            play->m_setup.runAtPcmRate = (enabled != 0);
            opn2_reset(device);
            return 0;
        }
    }
    return -1;
}


OPNMIDI_EXPORT const char *opn2_linkedLibraryVersion()
{
#if !defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    return OPNMIDI_VERSION;
#else
    return OPNMIDI_VERSION " (HQ)";
#endif
}

OPNMIDI_EXPORT const OPN2_Version *opn2_linkedVersion()
{
    return &opn2_version;
}

OPNMIDI_EXPORT const char *opn2_errorString()
{
    return OPN2MIDI_ErrorString.c_str();
}

OPNMIDI_EXPORT const char *opn2_errorInfo(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return opn2_errorString();
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!play)
        return opn2_errorString();
    return play->getErrorString().c_str();
}

OPNMIDI_EXPORT const char *opn2_getMusicTitle(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return "";
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!play)
        return "";
    return play->musTitle.c_str();
#else
    return "";
#endif
}

OPNMIDI_EXPORT void opn2_close(OPN2_MIDIPlayer *device)
{
    if(device->opn2_midiPlayer)
        delete reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    device->opn2_midiPlayer = NULL;
    free(device);
    device = NULL;
}

OPNMIDI_EXPORT void opn2_reset(OPN2_MIDIPlayer *device)
{
    if(!device)
        return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->m_setup.tick_skip_samples_delay = 0;
    play->opn.runAtPcmRate = play->m_setup.runAtPcmRate;
    play->opn.Reset(play->m_setup.emulator, play->m_setup.PCM_RATE);
    play->ch.clear();
    play->ch.resize(play->opn.NumChannels);
}

OPNMIDI_EXPORT double opn2_totalTimeLength(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return -1.0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->timeLength();
    else
#endif
        return -1.0;
}

OPNMIDI_EXPORT double opn2_loopStartTime(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return -1.0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->getLoopStart();
    else
#endif
        return -1.0;
}

OPNMIDI_EXPORT double opn2_loopEndTime(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return -1.0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->getLoopEnd();
    else
#endif
        return -1.0;
}

OPNMIDI_EXPORT double opn2_positionTell(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return -1.0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->tell();
    else
#endif
        return -1.0;
}

OPNMIDI_EXPORT void opn2_positionSeek(struct OPN2_MIDIPlayer *device, double seconds)
{
    if(!device)
        return;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        play->seek(seconds);
#else
    (void)seconds;
#endif
}

OPNMIDI_EXPORT void opn2_positionRewind(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        play->rewind();
#endif
}

OPNMIDI_EXPORT void opn2_setTempo(struct OPN2_MIDIPlayer *device, double tempo)
{
    if(!device || (tempo <= 0.0))
        return;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        play->setTempo(tempo);
#endif
}



OPNMIDI_EXPORT const char *opn2_metaMusicTitle(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return "";
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->musTitle.c_str();
    else
#endif
        return "";
}


OPNMIDI_EXPORT const char *opn2_metaMusicCopyright(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return "";
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->musCopyright.c_str();
    else
#endif
        return "";
}

OPNMIDI_EXPORT size_t opn2_metaTrackTitleCount(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return 0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->musTrackTitles.size();
    else
#endif
        return 0;
}

OPNMIDI_EXPORT const char *opn2_metaTrackTitle(struct OPN2_MIDIPlayer *device, size_t index)
{
    if(!device)
        return 0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(index >= play->musTrackTitles.size())
        return "INVALID";
    return play->musTrackTitles[index].c_str();
#else
    (void)index;
    return "NOT SUPPORTED";
#endif
}


OPNMIDI_EXPORT size_t opn2_metaMarkerCount(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return 0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(play)
        return play->musMarkers.size();
    else
#endif
        return 0;
}

OPNMIDI_EXPORT Opn2_MarkerEntry opn2_metaMarker(struct OPN2_MIDIPlayer *device, size_t index)
{
    struct Opn2_MarkerEntry marker;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!device || !play || (index >= play->musMarkers.size()))
    {
        marker.label = "INVALID";
        marker.pos_time = 0.0;
        marker.pos_ticks = 0;
        return marker;
    }
    else
    {
        OPNMIDIplay::MIDI_MarkerEntry &mk = play->musMarkers[index];
        marker.label = mk.label.c_str();
        marker.pos_time = mk.pos_time;
        marker.pos_ticks = (unsigned long)mk.pos_ticks;
    }
#else
    (void)device; (void)index;
    marker.label = "NOT SUPPORTED";
    marker.pos_time = 0.0;
    marker.pos_ticks = 0;
#endif
    return marker;
}

OPNMIDI_EXPORT void opn2_setRawEventHook(struct OPN2_MIDIPlayer *device, OPN2_RawEventHook rawEventHook, void *userData)
{
    if(!device)
        return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->hooks.onEvent = rawEventHook;
    play->hooks.onEvent_userData = userData;
}

/* Set note hook */
OPNMIDI_EXPORT void opn2_setNoteHook(struct OPN2_MIDIPlayer *device, OPN2_NoteHook noteHook, void *userData)
{
    if(!device)
        return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->hooks.onNote = noteHook;
    play->hooks.onNote_userData = userData;
}

/* Set debug message hook */
OPNMIDI_EXPORT void opn2_setDebugMessageHook(struct OPN2_MIDIPlayer *device, OPN2_DebugMessageHook debugMessageHook, void *userData)
{
    if(!device)
        return;
    OPNMIDIplay *play = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    play->hooks.onDebugMessage = debugMessageHook;
    play->hooks.onDebugMessage_userData = userData;
}


template <class Dst>
static void CopySamplesRaw(OPN2_UInt8 *dstLeft, OPN2_UInt8 *dstRight, const int32_t *src,
                           size_t frameCount, unsigned sampleOffset)
{
    for(size_t i = 0; i < frameCount; ++i) {
        *(Dst *)(dstLeft + (i * sampleOffset)) = src[2 * i];
        *(Dst *)(dstRight + (i * sampleOffset)) = src[(2 * i) + 1];
    }
}

template <class Dst, class Ret>
static void CopySamplesTransformed(OPN2_UInt8 *dstLeft, OPN2_UInt8 *dstRight, const int32_t *src,
                                   size_t frameCount, unsigned sampleOffset,
                                   Ret(&transform)(int32_t))
{
    for(size_t i = 0; i < frameCount; ++i) {
        *(Dst *)(dstLeft + (i * sampleOffset)) = transform(src[2 * i]);
        *(Dst *)(dstRight + (i * sampleOffset)) = transform(src[(2 * i) + 1]);
    }
}

static int SendStereoAudio(int         samples_requested,
                           ssize_t     in_size,
                           int32_t    *_in,
                           ssize_t     out_pos,
                           OPN2_UInt8 *left,
                           OPN2_UInt8 *right,
                           const OPNMIDI_AudioFormat *format)
{
    if(!in_size)
        return 0;
    size_t outputOffset = static_cast<size_t>(out_pos);
    size_t inSamples    = static_cast<size_t>(in_size * 2);
    size_t maxSamples   = static_cast<size_t>(samples_requested) - outputOffset;
    size_t toCopy       = std::min(maxSamples, inSamples);

    OPNMIDI_SampleType sampleType = format->type;
    const unsigned containerSize = format->containerSize;
    const unsigned sampleOffset = format->sampleOffset;

    left  += (outputOffset / 2) * sampleOffset;
    right += (outputOffset / 2) * sampleOffset;

    typedef int32_t(&pfnConvert)(int32_t);

    switch(sampleType) {
    case OPNMIDI_SampleType_S8:
    case OPNMIDI_SampleType_U8:
    {
        pfnConvert cvt = (sampleType == OPNMIDI_SampleType_S8) ? opn2_cvtS8 : opn2_cvtU8;
        switch(containerSize) {
        case sizeof(int8_t):
            CopySamplesTransformed<int8_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        case sizeof(int16_t):
            CopySamplesTransformed<int16_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        case sizeof(int32_t):
            CopySamplesTransformed<int32_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        default:
            return -1;
        }
        break;
    }
    case OPNMIDI_SampleType_S16:
    case OPNMIDI_SampleType_U16:
    {
        pfnConvert cvt = (sampleType == OPNMIDI_SampleType_S16) ? opn2_cvtS16 : opn2_cvtU16;
        switch(containerSize) {
        case sizeof(int16_t):
            CopySamplesTransformed<int16_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        case sizeof(int32_t):
            CopySamplesRaw<int32_t>(left, right, _in, toCopy / 2, sampleOffset);
            break;
        default:
            return -1;
        }
        break;
    }
    case OPNMIDI_SampleType_S24:
    case OPNMIDI_SampleType_U24:
    {
        pfnConvert cvt = (sampleType == OPNMIDI_SampleType_S24) ? opn2_cvtS24 : opn2_cvtU24;
        switch(containerSize) {
        case sizeof(int32_t):
            CopySamplesTransformed<int32_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        default:
            return -1;
        }
        break;
    }
    case OPNMIDI_SampleType_S32:
    case OPNMIDI_SampleType_U32:
    {
        pfnConvert cvt = (sampleType == OPNMIDI_SampleType_S32) ? opn2_cvtS32 : opn2_cvtU32;
        switch(containerSize) {
        case sizeof(int32_t):
            CopySamplesTransformed<int32_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        default:
            return -1;
        }
        break;
    }
    case OPNMIDI_SampleType_F32:
        if(containerSize != sizeof(float))
            return -1;
        CopySamplesTransformed<float>(left, right, _in, toCopy / 2, sampleOffset, opn2_cvtReal<float>);
        break;
    case OPNMIDI_SampleType_F64:
        if(containerSize != sizeof(double))
            return -1;
        CopySamplesTransformed<double>(left, right, _in, toCopy / 2, sampleOffset, opn2_cvtReal<double>);
        break;
    default:
        return -1;
    }

    return 0;
}


OPNMIDI_EXPORT int opn2_play(struct OPN2_MIDIPlayer *device, int sampleCount, short *out)
{
    return opn2_playFormat(device, sampleCount, (OPN2_UInt8 *)out, (OPN2_UInt8 *)(out + 1), &opn2_DefaultAudioFormat);
}

OPNMIDI_EXPORT int opn2_playFormat(OPN2_MIDIPlayer *device, int sampleCount,
                                   OPN2_UInt8 *out_left, OPN2_UInt8 *out_right,
                                   const OPNMIDI_AudioFormat *format)
{
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    sampleCount -= sampleCount % 2; //Avoid even sample requests
    if(sampleCount < 0)
        return 0;
    if(!device)
        return 0;

    OPNMIDIplay * player = (reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer));
    OPNMIDIplay::Setup &setup = player->m_setup;

    ssize_t gotten_len = 0;
    ssize_t n_periodCountStereo = 512;
    //ssize_t n_periodCountPhys = n_periodCountStereo * 2;
    int left = sampleCount;
    bool hasSkipped = setup.tick_skip_samples_delay > 0;

    while(left > 0)
    {
        {//
            const double eat_delay = setup.delay < setup.maxdelay ? setup.delay : setup.maxdelay;
            if(hasSkipped)
            {
                size_t samples = setup.tick_skip_samples_delay > sampleCount ? sampleCount : setup.tick_skip_samples_delay;
                n_periodCountStereo = samples / 2;
            }
            else
            {
                setup.delay -= eat_delay;
                setup.carry += setup.PCM_RATE * eat_delay;
                n_periodCountStereo = static_cast<ssize_t>(setup.carry);
                setup.carry -= n_periodCountStereo;
            }

            //if(setup.SkipForward > 0)
            //    setup.SkipForward -= 1;
            //else
            {
                if((player->atEnd) && (setup.delay <= 0.0))
                    break;//Stop to fetch samples at reaching the song end with disabled loop

                ssize_t leftSamples = left / 2;
                if(n_periodCountStereo > leftSamples)
                {
                    setup.tick_skip_samples_delay = (n_periodCountStereo - leftSamples) * 2;
                    n_periodCountStereo = leftSamples;
                }
                //! Count of stereo samples
                ssize_t in_generatedStereo = (n_periodCountStereo > 512) ? 512 : n_periodCountStereo;
                //! Total count of samples
                ssize_t in_generatedPhys = in_generatedStereo * 2;
                //! Unsigned total sample count
                //fill buffer with zeros
                int32_t *out_buf = player->outBuf;
                std::memset(out_buf, 0, static_cast<size_t>(in_generatedPhys) * sizeof(out_buf[0]));
                unsigned int chips = player->opn.NumCards;
                if(chips == 1)
                    player->opn.cardsOP2[0]->generate32(out_buf, (size_t)in_generatedStereo);
                else/* if(n_periodCountStereo > 0)*/
                {
                    /* Generate data from every chip and mix result */
                    for(size_t card = 0; card < chips; ++card)
                        player->opn.cardsOP2[card]->generateAndMix32(out_buf, (size_t)in_generatedStereo);
                }
                /* Process it */
                if(SendStereoAudio(sampleCount, in_generatedStereo, out_buf, gotten_len, out_left, out_right, format) == -1)
                    return 0;

                left -= (int)in_generatedPhys;
                gotten_len += (in_generatedPhys) /* - setup.stored_samples*/;
            }
            if(hasSkipped)
            {
                setup.tick_skip_samples_delay -= n_periodCountStereo * 2;
                hasSkipped = setup.tick_skip_samples_delay > 0;
            }
            else
                setup.delay = player->Tick(eat_delay, setup.mindelay);
        }//
    }

    return static_cast<int>(gotten_len);
#else
    return 0;
#endif //OPNMIDI_DISABLE_MIDI_SEQUENCER
}


OPNMIDI_EXPORT int opn2_generate(struct OPN2_MIDIPlayer *device, int sampleCount, short *out)
{
    return opn2_generateFormat(device, sampleCount, (OPN2_UInt8 *)out, (OPN2_UInt8 *)(out + 1), &opn2_DefaultAudioFormat);
}

OPNMIDI_EXPORT int opn2_generateFormat(struct OPN2_MIDIPlayer *device, int sampleCount,
                                       OPN2_UInt8 *out_left, OPN2_UInt8 *out_right,
                                       const OPNMIDI_AudioFormat *format)
{
    sampleCount -= sampleCount % 2; //Avoid even sample requests
    if(sampleCount < 0)
        return 0;
    if(!device)
        return 0;

    OPNMIDIplay * player = (reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer));
    OPNMIDIplay::Setup &setup = player->m_setup;

    ssize_t gotten_len = 0;
    ssize_t n_periodCountStereo = 512;

    int     left = sampleCount;
    double  delay = double(sampleCount) / double(setup.PCM_RATE);

    while(left > 0)
    {
        {//
            const double eat_delay = delay < setup.maxdelay ? delay : setup.maxdelay;
            delay -= eat_delay;
            setup.carry += setup.PCM_RATE * eat_delay;
            n_periodCountStereo = static_cast<ssize_t>(setup.carry);
            setup.carry -= n_periodCountStereo;

            {
                ssize_t leftSamples = left / 2;
                if(n_periodCountStereo > leftSamples)
                    n_periodCountStereo = leftSamples;
                //! Count of stereo samples
                ssize_t in_generatedStereo = (n_periodCountStereo > 512) ? 512 : n_periodCountStereo;
                //! Total count of samples
                ssize_t in_generatedPhys = in_generatedStereo * 2;
                //! Unsigned total sample count
                //fill buffer with zeros
                int32_t *out_buf = player->outBuf;
                std::memset(out_buf, 0, static_cast<size_t>(in_generatedPhys) * sizeof(out_buf[0]));
                unsigned int chips = player->opn.NumCards;
                if(chips == 1)
                    player->opn.cardsOP2[0]->generate32(out_buf, (size_t)in_generatedStereo);
                else/* if(n_periodCountStereo > 0)*/
                {
                    /* Generate data from every chip and mix result */
                    for(size_t card = 0; card < chips; ++card)
                        player->opn.cardsOP2[card]->generateAndMix32(out_buf, (size_t)in_generatedStereo);
                }
                /* Process it */
                if(SendStereoAudio(sampleCount, in_generatedStereo, out_buf, gotten_len, out_left, out_right, format) == -1)
                    return 0;

                left -= (int)in_generatedPhys;
                gotten_len += (in_generatedPhys) /* - setup.stored_samples*/;
            }

            player->TickIteratos(eat_delay);
        }//
    }

    return static_cast<int>(gotten_len);
}

OPNMIDI_EXPORT double opn2_tickEvents(struct OPN2_MIDIPlayer *device, double seconds, double granuality)
{
    if(!device)
        return -1.0;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return -1.0;
    return player->Tick(seconds, granuality);
#else
    (void)seconds; (void)granuality;
    return -1.0;
#endif
}

OPNMIDI_EXPORT int opn2_atEnd(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return 1;
#ifndef OPNMIDI_DISABLE_MIDI_SEQUENCER
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return 1;
    return (int)player->atEnd;
#else
    return 1;
#endif
}

OPNMIDI_EXPORT void opn2_panic(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_panic();
}

OPNMIDI_EXPORT void opn2_rt_resetState(struct OPN2_MIDIPlayer *device)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_ResetState();
}

OPNMIDI_EXPORT int opn2_rt_noteOn(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 note, OPN2_UInt8 velocity)
{
    if(!device)
        return 0;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return 0;
    return (int)player->realTime_NoteOn(channel, note, velocity);
}

OPNMIDI_EXPORT void opn2_rt_noteOff(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 note)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_NoteOff(channel, note);
}

OPNMIDI_EXPORT void opn2_rt_noteAfterTouch(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 note, OPN2_UInt8 atVal)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_NoteAfterTouch(channel, note, atVal);
}

OPNMIDI_EXPORT void opn2_rt_channelAfterTouch(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 atVal)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_ChannelAfterTouch(channel, atVal);
}

OPNMIDI_EXPORT void opn2_rt_controllerChange(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 type, OPN2_UInt8 value)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_Controller(channel, type, value);
}

OPNMIDI_EXPORT void opn2_rt_patchChange(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 patch)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_PatchChange(channel, patch);
}

OPNMIDI_EXPORT void opn2_rt_pitchBend(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt16 pitch)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_PitchBend(channel, pitch);
}

OPNMIDI_EXPORT void opn2_rt_pitchBendML(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 msb, OPN2_UInt8 lsb)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_PitchBend(channel, msb, lsb);
}

OPNMIDI_EXPORT void opn2_rt_bankChangeLSB(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 lsb)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_BankChangeLSB(channel, lsb);
}

OPNMIDI_EXPORT void opn2_rt_bankChangeMSB(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_UInt8 msb)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_BankChangeMSB(channel, msb);
}

OPNMIDI_EXPORT void opn2_rt_bankChange(struct OPN2_MIDIPlayer *device, OPN2_UInt8 channel, OPN2_SInt16 bank)
{
    if(!device)
        return;
    OPNMIDIplay *player = reinterpret_cast<OPNMIDIplay *>(device->opn2_midiPlayer);
    if(!player)
        return;
    player->realTime_BankChange(channel, (uint16_t)bank);
}
