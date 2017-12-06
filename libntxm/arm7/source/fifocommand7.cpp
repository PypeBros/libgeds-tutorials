/*
 * fifocommand7.cpp
 *
 *  Created on: Mar 28, 2010
 *      Author: tob
 */

#include <nds.h>
#include <stdarg.h>

#include "ntxm/fifocommand.h"
#include "ntxm/linear_freq_table.h"

extern "C" {
#ifdef __USING_MIKE
  #include "xtoa.h"
  #include "ntxm/tobmic.h"
#endif
}

#include "ntxm/player.h"
#include "ntxm/ntxm7.h"

extern NTXM7 *ntxm7;
bool ntxm_recording = false;

static void RecvCommandPlaySfx(PlaySfxCommand *ps)
{
    ntxm7->playSfx(ps->pot, ps->row, ps->channel, ps->nchannels);
}

#ifdef __HAS_PLAYSAMPLE
static void RecvCommandPlaySample(PlaySampleCommand *ps)
{
    ntxm7->playSample(ps->sample, ps->note, ps->volume, ps->channel);
}

static void RecvCommandStopSample(StopSampleSoundCommand* ss) {
    ntxm7->stopChannel(ss->channel);
}
#endif
#ifdef __USING_MIKE
static void RecvCommandMicOn(void)
{
    tob_MIC_On();
}

static void RecvCommandMicOff(void)
{
    tob_MIC_Off();
}

static void RecvCommandStartRecording(StartRecordingCommand* sr)
{
    ntxm_recording = true;
    tob_StartRecording(sr->buffer, sr->length);
}

static void RecvCommandStopRecording()
{
    int ret = tob_StopRecording();
    fifoSendValue32(FIFO_NTXM, (u32)ret);
    ntxm_recording = false;
}
#endif

static void RecvCommandSetSong(SetSongCommand *c) {
    ntxm7->setSong((Song*)c->ptr);
}

static void RecvCommandStartPlay(StartPlayCommand *c) {
  ntxm7->play(c->potpos, c->row, c->loop);
}

static void RecvCommandStopPlay(StopPlayCommand *c) {
    ntxm7->stop();
}

static void RecvCommandPlayInst(PlayInstCommand *c) {
    ntxm7->playInst(c->inst, c->note, c->volume, c->channel);
}

static void RecvCommandStopInst(StopInstCommand *c) {
  //    ntxm7->stopChannel(c->channel);
}
#ifdef __HAS_SETPATTERNLOOP
static void RecvCommandPatternLoop(PatternLoopCommand *c) {
    ntxm7->setPatternLoop(c->state);
}
#endif

#ifdef __CUSTOM_DEBUG
void CommandDbgOut(const char *formatstr, ...)
{
    NTXMFifoMessage command;
    command.commandType = DBG_OUT;

    DbgOutCommand *cmd = &command.dbgOut;

    va_list marker;
    va_start(marker, formatstr);

    char *debugstr = cmd->msg;

    for(u16 i=0;i<DEBUGSTRSIZE; ++i)
        debugstr[i] = 0;

    u16 strpos = 0;
    char *outptr = debugstr;
    char c;
    while((strpos < DEBUGSTRSIZE-1)&&(formatstr[strpos]!=0))
    {
        c=formatstr[strpos];

        if(c!='%') {
            *outptr = c;
            outptr++;
        } else {
            strpos++;
            c=formatstr[strpos];
            if(c=='d') {
                long l = va_arg(marker, long);
                outptr = ltoa(l, outptr, 10);
            } else if(c=='u'){
                unsigned long ul = va_arg(marker, unsigned long);
                outptr = ultoa(ul, outptr, 10);
            }
        }

        strpos++;
    }

    va_end(marker);

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}
#endif

void CommandUpdateRow(u16 row)
{
    NTXMFifoMessage command;
    command.commandType = UPDATE_ROW;

    UpdateRowCommand *c = &command.updateRow;
    c->row = row;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandUpdatePotPos(u16 potpos)
{
    NTXMFifoMessage command;
    command.commandType = UPDATE_POTPOS;

    UpdatePotPosCommand *c = &command.updatePotPos;
    c->potpos = potpos;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandNotifyStop(void)
{
    NTXMFifoMessage command;
    command.commandType = NOTIFY_STOP;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandSampleFinish(void)
{
    NTXMFifoMessage command;
    command.commandType = SAMPLE_FINISH;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandRecvHandler(int bytes, void *user_data) {
    NTXMFifoMessage command;

    fifoGetDatamsg(FIFO_NTXM, bytes, (u8*)&command);

    switch(command.commandType) {
        case PLAY_SFX:
            RecvCommandPlaySfx(&command.playSfx);
            break;
#ifdef __HAS_SAMPLEPLAY
        case PLAY_SAMPLE:
            RecvCommandPlaySample(&command.playSample);
            break;
        case STOP_SAMPLE:
            RecvCommandStopSample(&command.stopSample);
            break;
#endif
#ifdef __USING_MIKE
        case START_RECORDING:
            RecvCommandStartRecording(&command.startRecording);
            break;
        case STOP_RECORDING:
            RecvCommandStopRecording();
            break;
#endif
        case SET_SONG:
            RecvCommandSetSong(&command.setSong);
            break;
        case START_PLAY:
            RecvCommandStartPlay(&command.startPlay);
            break;
        case STOP_PLAY:
            RecvCommandStopPlay(&command.stopPlay);
            break;
        case PLAY_INST:
            RecvCommandPlayInst(&command.playInst);
            break;
        case STOP_INST:
            RecvCommandStopInst(&command.stopInst);
            break;
#ifdef __USING_MIKE
        case MIC_ON:
            RecvCommandMicOn();
            break;
        case MIC_OFF:
            RecvCommandMicOff();
            break;
#endif
#ifdef __HAS_PATTERN_LOOP
        case PATTERN_LOOP:
            RecvCommandPatternLoop(&command.ptnLoop);
            break;
#endif
        default:
            break;
    }
}

void CommandInit(void)
{
    fifoSetDatamsgHandler(FIFO_NTXM, CommandRecvHandler, 0);
    //fifoSetValue32Handler(FIFO_NTXM, CommandRecvHandler, 0);
}
