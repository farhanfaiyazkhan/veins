/* -*- mode:c++ -*- ********************************************************
 * file:        BurstApplLayer.cc
 *
 * author:      Daniel Willkomm
 *
 * copyright:   (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * part of:     framework implementation developed by tkn
 * description: application layer like the TestApplLayer but sends burst of
 *               messages
 **************************************************************************/


#include "BurstApplLayerBattery.h"

Define_Module(BurstApplLayerBattery);

// do some initialization
void BurstApplLayerBattery::initialize(int stage)
{
    BurstApplLayer::initialize(stage);

    if(stage==0) {

      EV << "subscribe to host state" << endl;
        int scopeHost = (this->findHost())->getId();
        HostState hs;
        hostStateCat = utility->subscribe(this, &hs, scopeHost);
        hostState = HostState::ON;

        bcastOut = replyOut = replyIn = 0;
    }
}

void BurstApplLayerBattery::handleSelfMsg(cMessage *msg)
{
  switch(msg->getKind()) {
  case SEND_BROADCAST_TIMER:
    for(int i=0; i<burstSize; i++) {
      bcastOut += 1;
      EV << "bcastQueued = " << bcastOut << endl;
      sendBroadcast();
    }
    break;
  default:
    EV <<" Unkown selfmessage! kind: "<<msg->getKind() << endl;
    break;
  }
}

void BurstApplLayerBattery::handleLowerMsg( cMessage* msg )
{
  ApplPkt *m;

  switch( msg->getKind() ) {
  case BROADCAST_MESSAGE:
    m = static_cast<ApplPkt *>(msg);
    EV << "Received a broadcast packet from host["<<m->getSrcAddr()<<"] -> sending reply\n";
    replyOut += 1;
    EV << simTime() << ": replyOut = " << replyOut << endl;
    sendReply(m);
    break;
  case BROADCAST_REPLY_MESSAGE:
    m = static_cast<ApplPkt *>(msg);
    EV << "Received reply from host["<<m->getSrcAddr()<<"]; delete msg\n";
    replyIn += 1;
    EV << simTime() << ": replyIn = " << replyIn << endl;
    delete msg;
    break;
  default:
    EV <<"Error! got packet with unknown kind: " << msg->getKind()<<endl;
    break;
  }
}


void BurstApplLayerBattery::receiveBBItem(int category, const BBItem *details, int scopeModuleId)
{
    Enter_Method_Silent();
    BaseModule::receiveBBItem(category, details, scopeModuleId);

    if (category == hostStateCat) {

      hostState = ((HostState *)details)->get();

      switch (hostState) {
      case HostState::FAILED:
        EV << "t = " << simTime() << " host state FAILED" << endl;
        recordScalar("HostState::FAILED", simTime());

        // BurstApplLayer sends all its frames to the MAC layer at
        // once, so this happens only if the battery fails before the
        // burst.  Frames that are already sendDown() are stopped at
        // the SnrEval.

        // battery is depleted, stop events
        if (delayTimer->isScheduled()) {
          EV << "canceling frame burst" << endl;
          cancelEvent(delayTimer);
        }
        break;
      default:
        // ON/OFF aren't used
        break;
      }
    }
}

void BurstApplLayerBattery::finish()
{
    BurstApplLayer::finish();
    recordScalar("broadcast queued", bcastOut);
    recordScalar("replies sent", replyOut);
    recordScalar("replies received", replyIn);
}