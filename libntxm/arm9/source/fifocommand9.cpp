/*
 * fifocommand9.cpp
 *
 *  Created on: Mar 27, 2010
 *      Author: tob
 */

#include <nds/ndstypes.h>
#include "ntxm/fifocommand.h"

void (*onUpdateRow)(u16 row) = 0;
void (*onStop)(void) = 0;
void (*onPlaySampleFinished)(void) = 0;
void (*onPotPosChange)(u16 potpos) = 0;

void RegisterRowCallback(void (*onUpdateRow_)(u16))
{
    onUpdateRow = onUpdateRow_;
}

void RegisterStopCallback(void (*onStop_)(void))
{
    onStop = onStop_;
}

void RegisterPlaySampleFinishedCallback(void (*onPlaySampleFinished_)(void))
{
    onPlaySampleFinished = onPlaySampleFinished_;
}

void RegisterPotPosChangeCallback(void (*onPotPosChange_)(u16))
{
    onPotPosChange = onPotPosChange_;
}

void RecvCommandUpdateRow(UpdateRowCommand *c)
{
    if(onUpdateRow)
        onUpdateRow(c->row);
}

void RecvCommandUpdatePotPos(UpdatePotPosCommand *c)
{
    if(onPotPosChange)
        onPotPosChange(c->potpos);
}

void RecvCommandNotifyStop(void)
{
    if(onStop)
        onStop();
}

void RecvCommandSampleFinish(void) {
    if(onPlaySampleFinished)
        onPlaySampleFinished();
}

void CommandRecvHandler(int bytes, void *user_data) {
    NTXMFifoMessage msg;

    fifoGetDatamsg(FIFO_NTXM, bytes, (u8*)&msg);

    switch(msg.commandType) {
        case DBG_OUT: // TODO it's not safe to do this in an interrupt handler
            iprintf("%s", msg.dbgOut.msg);
            break;

        case UPDATE_ROW:
            RecvCommandUpdateRow(&msg.updateRow);
            break;

        case UPDATE_POTPOS:
            RecvCommandUpdatePotPos(&msg.updatePotPos);
            break;

        case NOTIFY_STOP:
            RecvCommandNotifyStop();
            break;

        case SAMPLE_FINISH:
            RecvCommandSampleFinish();
            break;

        default:
            break;
    }
}

void CommandInit() {
    fifoSetDatamsgHandler(FIFO_NTXM, CommandRecvHandler, 0);
    //fifoSetValue32Handler(FIFO_NTXM, CommandRecvHandler, 0);
}

void CommandPlaySfx(u8 pot, u8 row, u8 channel, u8 nchannels)
{
  NTXMFifoMessage command;
  PlaySfxCommand* ps = &command.playSfx;
  command.commandType = PLAY_SFX;
  ps->pot = pot;
  ps->row = row;
  ps->channel = channel;
  ps->nchannels = nchannels;

  fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandPlaySample(Sample *sample, u8 note, u8 volume, u8 channel)
{
    NTXMFifoMessage command;
    PlaySampleCommand* ps = &command.playSample;

    command.commandType = PLAY_SAMPLE;

    ps->sample = sample;
    ps->note = note;
    ps->volume = volume;
    ps->channel = channel;


    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandStopSample(int channel)
{
    NTXMFifoMessage command;
    StopSampleSoundCommand *ss = &command.stopSample;

    command.commandType = STOP_SAMPLE;
    ss->channel = channel;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandStartRecording(u16* buffer, int length)
{
    NTXMFifoMessage command;
    StartRecordingCommand* sr = &command.startRecording;

    command.commandType = START_RECORDING;
    sr->buffer = buffer;
    sr->length = length;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

int CommandStopRecording(void)
{
    NTXMFifoMessage command;
    command.commandType = STOP_RECORDING;

    while(!fifoCheckValue32(FIFO_NTXM))
        swiDelay(1);

    return (int)fifoGetValue32(FIFO_NTXM);
}

void CommandSetSong(void *song)
{
    NTXMFifoMessage command;
    SetSongCommand* c = &command.setSong;

    command.commandType = SET_SONG;
    c->ptr = song;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandStartPlay(u8 potpos, u16 row, bool loop)
{
    NTXMFifoMessage command;
    StartPlayCommand* c = &command.startPlay;

    command.commandType = START_PLAY;
    c->potpos = potpos;
    c->row = row;
    c->loop = loop?1:0;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandStopPlay(void) {

    NTXMFifoMessage command;
    command.commandType = STOP_PLAY;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandPlayInst(u8 inst, u8 note, u8 volume, u8 channel)
{
    NTXMFifoMessage command;
    command.commandType = PLAY_INST;

    PlayInstCommand* c = &command.playInst;

    c->inst    = inst;
    c->note    = note;
    c->volume  = volume;
    c->channel = channel;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandStopInst(u8 channel)
{
    NTXMFifoMessage command;
    command.commandType = STOP_INST;

    StopInstCommand* c = &command.stopInst;

    c->channel = channel;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandMicOn(void)
{
    NTXMFifoMessage command;
    command.commandType = MIC_ON;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandMicOff(void)
{
    NTXMFifoMessage command;
    command.commandType = MIC_OFF;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}

void CommandSetPatternLoop(bool state)
{
    NTXMFifoMessage command;
    command.commandType = PATTERN_LOOP;

    PatternLoopCommand* c = &command.ptnLoop;
    c->state = state;

    fifoSendDatamsg(FIFO_NTXM, sizeof(command), (u8*)&command);
}
