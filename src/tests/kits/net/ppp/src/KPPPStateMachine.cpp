//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <OS.h>

#include <KPPPInterface.h>
#include <KPPPConfigurePacket.h>
#include <KPPPDevice.h>
#include <KPPPEncapsulator.h>
#include <KPPPOptionHandler.h>

#include <net/if.h>
#include <mbuf.h>


#define PPP_STATE_MACHINE_TIMEOUT			3000000


PPPStateMachine::PPPStateMachine(PPPInterface& interface)
	: fInterface(&interface), fLCP(interface.LCP()), fPhase(PPP_DOWN_PHASE),
	fState(PPP_INITIAL_STATE), fID(system_time() & 0xFF),
	fAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fPeerAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fAuthenticationName(NULL), fPeerAuthenticationName(NULL),
	fMaxRequest(10), fMaxTerminate(2), fMaxNak(5),
	fRequestID(0), fTerminateID(0), fNextTimeout(0)
{
}


PPPStateMachine::~PPPStateMachine()
{
	free(fAuthenticationName);
	free(fPeerAuthenticationName);
}


uint8
PPPStateMachine::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


// remember: NewState() must always be called _after_ IllegalEvent()
// because IllegalEvent() also looks at the current state.
void
PPPStateMachine::NewState(PPP_STATE next)
{
	// maybe we do not need the timer anymore
	if(next < PPP_CLOSING_STATE || next == PPP_OPENED_STATE)
		fNextTimeout = 0;
	
	if(State() == PPP_OPENED_STATE && next != State())
		ResetOptionHandlers();
	
	fState = next;
}


void
PPPStateMachine::NewPhase(PPP_PHASE next)
{
	// Report a down event to parent if we are not usable anymore.
	// The report threads get their notification later.
	if(Phase() == PPP_ESTABLISHED_PHASE && next != Phase()) {
		if(Interface()->Ifnet())
			Interface()->Ifnet()->if_flags &= ~IFF_RUNNING;
		
		if(!Interface()->DoesDialOnDemand())
			Interface()->UnregisterInterface();
		
		if(Interface()->Parent())
			Interface()->Parent()->StateMachine().DownEvent(Interface());
	}
	
	// there is nothing after established phase and nothing before down phase
	if(next > PPP_ESTABLISHED_PHASE)
		fPhase = PPP_ESTABLISHED_PHASE;
	else if(next < PPP_DOWN_PHASE)
		fPhase = PPP_DOWN_PHASE;
	else
		fPhase = next;
}


// authentication events
void
PPPStateMachine::AuthenticationRequested()
{
	LockerHelper locker(fLock);
	
	fAuthenticationStatus = PPP_AUTHENTICATING;
	free(fAuthenticationName);
	fAuthenticationName = NULL;
}


void
PPPStateMachine::AuthenticationAccepted(const char *name)
{
	LockerHelper locker(fLock);
	
	fAuthenticationStatus = PPP_AUTHENTICATION_SUCCESSFUL;
	free(fAuthenticationName);
	fAuthenticationName = strdup(name);
	
	Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_AUTHENTICATION_SUCCESSFUL,
		NULL, 0);
}


void
PPPStateMachine::AuthenticationDenied(const char *name)
{
	LockerHelper locker(fLock);
	
	fAuthenticationStatus = PPP_AUTHENTICATION_FAILED;
	free(fAuthenticationName);
	fAuthenticationName = strdup(name);
}


void
PPPStateMachine::PeerAuthenticationRequested()
{
	LockerHelper locker(fLock);
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATING;
	free(fPeerAuthenticationName);
	fPeerAuthenticationName = NULL;
}


void
PPPStateMachine::PeerAuthenticationAccepted(const char *name)
{
	LockerHelper locker(fLock);
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATION_SUCCESSFUL;
	free(fPeerAuthenticationName);
	fPeerAuthenticationName = strdup(name);
	
	Interface()->Report(PPP_CONNECTION_REPORT,
		PPP_REPORT_PEER_AUTHENTICATION_SUCCESSFUL, NULL, 0);
}


void
PPPStateMachine::PeerAuthenticationDenied(const char *name)
{
	LockerHelper locker(fLock);
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATION_FAILED;
	free(fPeerAuthenticationName);
	fPeerAuthenticationName = strdup(name);
	
	CloseEvent();
}


void
PPPStateMachine::UpFailedEvent(PPPInterface *interface)
{
	// TODO:
	// log that an interface did not go up
}


void
PPPStateMachine::UpEvent(PPPInterface *interface)
{
	LockerHelper locker(fLock);
	
	if(Phase() <= PPP_TERMINATION_PHASE) {
		interface->StateMachine().CloseEvent();
		return;
	}
	
	Interface()->CalculateBaudRate();
	
	if(Phase() == PPP_ESTABLISHMENT_PHASE) {
		// this is the first interface that went up
		Interface()->SetLinkMTU(interface->LinkMTU());
		locker.UnlockNow();
		ThisLayerUp();
	} else if(Interface()->LinkMTU() > interface->LinkMTU())
		Interface()->SetLinkMTU(interface->LinkMTU());
			// linkMTU should always be the smallest value of all children
	
	NewState(PPP_OPENED_STATE);
}


void
PPPStateMachine::DownEvent(PPPInterface *interface)
{
	LockerHelper locker(fLock);
	
	uint32 linkMTU = 0;
		// the new linkMTU
	
	Interface()->CalculateBaudRate();
	
	// when all children are down we should not be running
	if(Interface()->IsMultilink() && !Interface()->Parent()) {
		uint32 count = 0;
		PPPInterface *child;
		for(int32 index = 0; index < Interface()->CountChildren(); index++) {
			child = Interface()->ChildAt(index);
			
			if(child && child->IsUp()) {
				// set linkMTU to the smallest value of all children
				if(linkMTU == 0)
					linkMTU = child->LinkMTU();
				else if(linkMTU > child->LinkMTU())
					linkMTU = child->LinkMTU();
				
				++count;
			}
		}
		
		Interface()->SetLinkMTU(linkMTU);
		
		if(count == 0) {
			locker.UnlockNow();
			DownEvent();
		}
	}
}


void
PPPStateMachine::UpFailedEvent(PPPProtocol *protocol)
{
	if((protocol->Flags() & PPP_NOT_IMPORTANT) == 0) {
		if(Interface()->Mode() == PPP_CLIENT_MODE) {
			// pretend we lost connection:
			if(Interface()->IsMultilink() && !Interface()->Parent())
				for(int32 index = 0; index < Interface()->CountChildren(); index++)
					Interface()->ChildAt(index)->StateMachine().CloseEvent();
			else if(Interface()->Device())
				Interface()->Device()->Down();
			else
				CloseEvent();
					// just to be on the secure side ;)
		} else
			CloseEvent();
	}
}


void
PPPStateMachine::UpEvent(PPPProtocol *protocol)
{
	LockerHelper locker(fLock);
	
	if(Phase() >= PPP_ESTABLISHMENT_PHASE)
		BringHandlersUp();
}


void
PPPStateMachine::DownEvent(PPPProtocol *protocol)
{
}


void
PPPStateMachine::UpFailedEvent(PPPEncapsulator *encapsulator)
{
	if((encapsulator->Flags() & PPP_NOT_IMPORTANT) == 0) {
		if(Interface()->Mode() == PPP_CLIENT_MODE) {
			// pretend we lost connection:
			if(Interface()->IsMultilink() && !Interface()->Parent())
				for(int32 index = 0; index < Interface()->CountChildren(); index++)
					Interface()->ChildAt(index)->StateMachine().CloseEvent();
			else if(Interface()->Device())
				Interface()->Device()->Down();
			else
				CloseEvent();
					// just to be on the secure side ;)
		} else
			CloseEvent();
	}
}


void
PPPStateMachine::UpEvent(PPPEncapsulator *encapsulator)
{
	LockerHelper locker(fLock);
	
	if(Phase() >= PPP_ESTABLISHMENT_PHASE)
		BringHandlersUp();
}


void
PPPStateMachine::DownEvent(PPPEncapsulator *encapsulator)
{
}


// This is called by the device to tell us that it entered establishment
// phase. We can use Device::Down() to abort establishment until UpEvent()
// is called.
// The return value says if we are waiting for an UpEvent(). If false is
// returned the device should immediately abort its attempt to connect.
bool
PPPStateMachine::TLSNotify()
{
	LockerHelper locker(fLock);
	
	if(State() == PPP_STARTING_STATE) {
			if(Phase() == PPP_DOWN_PHASE)
				NewPhase(PPP_ESTABLISHMENT_PHASE);
					// this says that the device is going up
			return true;
	}
	
	return false;
}


// This is called by the device to tell us that it entered termination phase.
// A Device::Up() should wait until the device went down.
// If false is returned we want to stay connected, though we called
// Device::Down().
bool
PPPStateMachine::TLFNotify()
{
	LockerHelper locker(fLock);
	
	// from now on no packets may be sent to the device
	NewPhase(PPP_DOWN_PHASE);
	
	return true;
}


void
PPPStateMachine::UpFailedEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_STARTING_STATE:
			// TLSNotify() sets establishment phase
			if(Phase() != PPP_ESTABLISHMENT_PHASE) {
				// there must be a BUG in the device add-on or someone is trying to
				// fool us (UpEvent() is public) as we did not request the device
				// to go up
				IllegalEvent(PPP_UP_FAILED_EVENT);
				NewState(PPP_INITIAL_STATE);
				break;
			}
			
			Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_DEVICE_UP_FAILED,
				NULL, 0);
			
			if(Interface()->Parent())
				Interface()->Parent()->StateMachine().UpFailedEvent(Interface());
		break;
		
		default:
			IllegalEvent(PPP_UP_FAILED_EVENT);
	}
}


void
PPPStateMachine::UpEvent()
{
	// This call is public, thus, it might not only be called by the device.
	// We must recognize these attempts to fool us and handle them correctly.
	
	LockerHelper locker(fLock);
	
	if(!Interface()->Device() || !Interface()->Device()->IsUp())
		return;
			// it is not our device that went up...
	
	Interface()->CalculateBaudRate();
	
	switch(State()) {
		case PPP_INITIAL_STATE:
			if(Interface()->Mode() != PPP_SERVER_MODE
				|| Phase() != PPP_ESTABLISHMENT_PHASE) {
				// we are a client or we do not listen for an incoming
				// connection, so this is an illegal event
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				locker.UnlockNow();
				ThisLayerFinished();
				
				return;
			}
			
			// TODO: handle server-up! (maybe already done correctly)
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_STARTING_STATE:
			// we must have called TLS() which sets establishment phase
			if(Phase() != PPP_ESTABLISHMENT_PHASE) {
				// there must be a BUG in the device add-on or someone is trying to
				// fool us (UpEvent() is public) as we did not request the device
				// to go up
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				locker.UnlockNow();
				ThisLayerFinished();
				break;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		default:
			IllegalEvent(PPP_UP_EVENT);
	}
}


void
PPPStateMachine::DownEvent()
{
	LockerHelper locker(fLock);
	
	if(Interface()->Device() && Interface()->Device()->IsUp())
		return;
			// it is not our device that went up...
	
	Interface()->CalculateBaudRate();
	
	// reset IdleSince
	Interface()->fIdleSince = 0;
	
	switch(State()) {
		case PPP_CLOSED_STATE:
		case PPP_CLOSING_STATE:
			NewState(PPP_INITIAL_STATE);
		break;
		
		case PPP_STOPPED_STATE:
			// The RFC says we should reconnect, but our implementation
			// will only do this if auto-redial is enabled (only clients).
			NewState(PPP_STARTING_STATE);
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
		case PPP_OPENED_STATE:
			NewState(PPP_STARTING_STATE);
		break;
		
		default:
			IllegalEvent(PPP_DOWN_EVENT);
	}
	
	DownProtocols();
	DownEncapsulators();
	
	fAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	fPeerAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	
	NewPhase(PPP_DOWN_PHASE);
	
	// maybe we need to redial
	if(State() == PPP_STARTING_STATE) {
		bool needsRedial = false;
		
		if(fAuthenticationStatus == PPP_AUTHENTICATION_FAILED
				|| fAuthenticationStatus == PPP_AUTHENTICATING
				|| fPeerAuthenticationStatus == PPP_AUTHENTICATION_FAILED
				|| fPeerAuthenticationStatus == PPP_AUTHENTICATING)
			Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_AUTHENTICATION_FAILED,
				NULL, 0);
		else {
			// if we are going up and lost connection the redial attempt becomes
			// a dial retry which is managed by the main thread in Interface::Up()
			if(Interface()->fUpThread == -1)
				needsRedial = true;
			
			Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_CONNECTION_LOST,
				NULL, 0);
		}
		
		if(Interface()->Parent())
			Interface()->Parent()->StateMachine().UpFailedEvent(Interface());
		
		NewState(PPP_INITIAL_STATE);
		
		if(Interface()->DoesAutoRedial()) {
			if(needsRedial)
				Interface()->Redial();
		} else if(!Interface()->DoesDialOnDemand())
			Interface()->Delete();
	} else {
		Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_DOWN_SUCCESSFUL, NULL, 0);
		
		if(!Interface()->DoesDialOnDemand())
			Interface()->Delete();
	}
}


// private events
void
PPPStateMachine::OpenEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
			if(!Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_GOING_UP, NULL, 0))
				return;
			
			if(Interface()->Mode() == PPP_SERVER_MODE) {
				NewPhase(PPP_ESTABLISHMENT_PHASE);
				
				if(Interface()->Device())
					Interface()->Device()->Listen();
			} else
				NewState(PPP_STARTING_STATE);
			
			if(Interface()->IsMultilink() && !Interface()->Parent()) {
				NewPhase(PPP_ESTABLISHMENT_PHASE);
				for(int32 index = 0; index < Interface()->CountChildren(); index++)
					if(Interface()->ChildAt(index)->Mode() == Interface()->Mode())
						Interface()->ChildAt(index)->StateMachine().OpenEvent();
			} else {
				locker.UnlockNow();
				ThisLayerStarted();
			}
		break;
		
		case PPP_CLOSED_STATE:
			if(Phase() == PPP_DOWN_PHASE) {
				// the device is already going down
				return;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_STOPPING_STATE);
		break;
		
		default:
			;
	}
}


void
PPPStateMachine::CloseEvent()
{
	LockerHelper locker(fLock);
	
	if(Interface()->IsMultilink() && !Interface()->Parent()) {
		NewState(PPP_INITIAL_STATE);
		
		if(Phase() != PPP_DOWN_PHASE)
			NewPhase(PPP_TERMINATION_PHASE);
		
		ThisLayerDown();
		
		for(int32 index = 0; index < Interface()->CountChildren(); index++)
			Interface()->ChildAt(index)->StateMachine().CloseEvent();
		
		return;
	}
	
	switch(State()) {
		case PPP_OPENED_STATE:
			ThisLayerDown();
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_CLOSING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendTerminateRequest();
		break;
		
		case PPP_STARTING_STATE:
			NewState(PPP_INITIAL_STATE);
			
			// TLSNotify() will know that we were faster because we
			// are in PPP_INITIAL_STATE now
			if(Phase() == PPP_ESTABLISHMENT_PHASE && Interface()->Parent()) {
				// the device is already up
				NewPhase(PPP_DOWN_PHASE);
					// this says the following DownEvent() was not caused by
					// a connection fault
				locker.UnlockNow();
				ThisLayerFinished();
			}
		break;
		
		case PPP_STOPPING_STATE:
			NewState(PPP_CLOSING_STATE);
		break;
		
		case PPP_STOPPED_STATE:
			NewState(PPP_STOPPED_STATE);
		break;
		
		default:
			;
	}
}


// timeout (restart counters are > 0)
void
PPPStateMachine::TOGoodEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_CLOSING_STATE:
		case PPP_STOPPING_STATE:
			locker.UnlockNow();
			SendTerminateRequest();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		default:
			IllegalEvent(PPP_TO_GOOD_EVENT);
	}
}


// timeout (restart counters are <= 0)
void
PPPStateMachine::TOBadEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		default:
			IllegalEvent(PPP_TO_BAD_EVENT);
	}
}


// receive configure request (acceptable request)
void
PPPStateMachine::RCRGoodEvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_GOOD_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			SendTerminateAck(packet);
		break;
		
		case PPP_STOPPED_STATE:
			// irc,scr,sca/8
			// XXX: should we do nothing and wait for DownEvent()?
		break;
		
		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_SENT_STATE);
		
		case PPP_ACK_SENT_STATE:
			locker.UnlockNow();
			SendConfigureAck(packet);
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_OPENED_STATE);
			locker.UnlockNow();
			SendConfigureAck(packet);
			ThisLayerUp();
		break;
		
		case PPP_OPENED_STATE:
			// tld,scr,sca/8
			NewState(PPP_ACK_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
			SendConfigureAck(packet);
		break;
		
		default:
			;
	}
}


// receive configure request (unacceptable request)
void
PPPStateMachine::RCRBadEvent(struct mbuf *nak, struct mbuf *reject)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_BAD_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			SendTerminateAck(NULL);
		break;
		
		case PPP_STOPPED_STATE:
			// irc,scr,scn/6
			// XXX: should we do nothing and wait for DownEvent()?
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		
		case PPP_ACK_SENT_STATE:
			if(State() == PPP_ACK_SENT_STATE)
				NewState(PPP_REQ_SENT_STATE);
					// OPENED_STATE might have set this already
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
			locker.UnlockNow();
			if(nak && ntohs(mtod(nak, ppp_lcp_packet*)->length) > 3)
				SendConfigureNak(nak);
			else if(reject && ntohs(mtod(reject, ppp_lcp_packet*)->length) > 3)
				SendConfigureNak(reject);
		break;
		
		default:
			;
	}
}


// receive configure ack
void
PPPStateMachine::RCAEvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		return;
	}
	
	PPPOptionHandler *handler;
	PPPConfigurePacket ack(packet);
	
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		handler = LCP().OptionHandlerAt(index);
		handler->ParseAck(&ack);
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCA_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			SendTerminateAck(NULL);
		break;
		
		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_RCVD_STATE);
			InitializeRestartCount();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_ACK_SENT_STATE:
			NewState(PPP_OPENED_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			ThisLayerUp();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
		
		default:
			;
	}
}


// receive configure nak/reject
void
PPPStateMachine::RCNEvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		return;
	}
	
	PPPOptionHandler *handler;
	PPPConfigurePacket nak_reject(packet);
	
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		handler = LCP().OptionHandlerAt(index);
		
		if(nak_reject.Code() == PPP_CONFIGURE_NAK)
			handler->ParseNak(&nak_reject);
		else if(nak_reject.Code() == PPP_CONFIGURE_REJECT)
			handler->ParseReject(&nak_reject);
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCN_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			SendTerminateAck(packet);
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			InitializeRestartCount();
		
		case PPP_ACK_RCVD_STATE:
			if(State() == PPP_ACK_RCVD_STATE)
				NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
		
		default:
			;
	}
}


// receive terminate request
void
PPPStateMachine::RTREvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	// we should not use the same ID as the peer
	if(fID == mtod(packet, ppp_lcp_packet*)->id)
		fID -= 128;
	
	fAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	fPeerAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RTR_EVENT);
		break;
		
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			locker.UnlockNow();
			SendTerminateAck(packet);
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			ZeroRestartCount();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateAck(packet);
		break;
		
		default:
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			locker.UnlockNow();
			SendTerminateAck(packet);
	}
}


// receive terminate ack
void
PPPStateMachine::RTAEvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	if(fTerminateID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		return;
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RTA_EVENT);
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
			NewState(PPP_STOPPED_STATE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
		
		default:
			;
	}
}


// receive unknown code
void
PPPStateMachine::RUCEvent(struct mbuf *packet, uint16 protocol, uint8 type = PPP_PROTOCOL_REJECT)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RUC_EVENT);
		break;
		
		default:
			locker.UnlockNow();
			SendCodeReject(packet, protocol, type);
	}
}


// receive code/protocol reject (acceptable such as IPX reject)
void
PPPStateMachine::RXJGoodEvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXJ_GOOD_EVENT);
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		break;
		
		default:
			;
	}
}


// receive code/protocol reject (catastrophic such as LCP reject)
void
PPPStateMachine::RXJBadEvent(struct mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXJ_BAD_EVENT);
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
		
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateRequest();
		break;
	}
}


// receive echo request/reply, discard request
void
PPPStateMachine::RXREvent(struct mbuf *packet)
{
	ppp_lcp_packet *echo = mtod(packet, ppp_lcp_packet*);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXR_EVENT);
		break;
		
		case PPP_OPENED_STATE:
			if(echo->code == PPP_ECHO_REQUEST)
				SendEchoReply(packet);
		break;
		
		default:
			;
	}
}


// general events (for Good/Bad events)
void
PPPStateMachine::TimerEvent()
{
	LockerHelper locker(fLock);
	if(fNextTimeout == 0 || fNextTimeout > system_time())
		return;
	fNextTimeout = 0;
	locker.UnlockNow();
	
	switch(State()) {
		case PPP_CLOSING_STATE:
		case PPP_STOPPING_STATE:
			if(fTerminateCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			if(fRequestCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;
		
		default:
			;
	}
	
}


// ReceiveConfigureRequest
// Here we get a configure-request packet from LCP and aks all OptionHandlers
// if its values are acceptable. From here we call our Good/Bad counterparts.
void
PPPStateMachine::RCREvent(struct mbuf *packet)
{
	PPPConfigurePacket request(packet), nak(PPP_CONFIGURE_NAK),
		reject(PPP_CONFIGURE_REJECT);
	PPPOptionHandler *handler;
	
	// we should not use the same id as the peer
	if(fID == mtod(packet, ppp_lcp_packet*)->id)
		fID -= 128;
	
	nak.SetID(request.ID());
	reject.SetID(request.ID());
	
	// each handler should add unacceptable values for each item
	bool handled;
	int32 error;
	for(int32 item = 0; item <= request.CountItems(); item++) {
		// if we sent too many naks we should not append additional values
		if(fNakCounter == 0 && item == request.CountItems())
			break;
		
		handled = false;
		
		for(int32 index = 0; index < LCP().CountOptionHandlers();
				index++) {
			handler = LCP().OptionHandlerAt(index);
			error = handler->ParseRequest(&request, item, &nak, &reject);
			if(error == PPP_UNHANDLED)
				continue;
			else if(error == B_ERROR) {
				// the request contains a value that has been sent more than
				// once or the value is corrupted
				CloseEvent();
				return;
			} else if(error == B_OK)
				handled = true;
		}
		
		if(!handled && item < request.CountItems()) {
			// unhandled items should be added to reject
			reject.AddItem(request.ItemAt(item));
		}
	}
	
	if(nak.CountItems() > 0)
		RCRBadEvent(nak.ToMbuf(LCP().AdditionalOverhead()), NULL);
	else if(reject.CountItems() > 0)
		RCRBadEvent(NULL, reject.ToMbuf(LCP().AdditionalOverhead()));
	else
		RCRGoodEvent(packet);
}


// ReceiveCodeReject
// LCP received a code/protocol-reject packet and we look if it is acceptable.
// From here we call our Good/Bad counterparts.
void
PPPStateMachine::RXJEvent(struct mbuf *packet)
{
	ppp_lcp_packet *reject mtod(packet, ppp_lcp_packet*);
	
	if(reject->code == PPP_CODE_REJECT) {
		// we only have basic codes, so there must be something wrong
		if(Interface()->IsMultilink() && !Interface()->Parent())
			CloseEvent();
		else
			RXJBadEvent(packet);
	} else if(reject->code == PPP_PROTOCOL_REJECT) {
		// disable all handlers for rejected protocol type
		uint16 rejected = *((uint16*) reject->data);
			// rejected protocol number
		
		if(rejected == PPP_LCP_PROTOCOL) {
			// LCP must not be rejected!
			RXJBadEvent(packet);
			return;
		}
		
		int32 index;
		PPPProtocol *protocol_handler;
		PPPEncapsulator *encapsulator_handler = Interface()->FirstEncapsulator();
		
		for(index = 0; index < Interface()->CountProtocols(); index++) {
			protocol_handler = Interface()->ProtocolAt(index);
			if(protocol_handler && protocol_handler->Protocol() == rejected)
				protocol_handler->SetEnabled(false);
					// disable protocol
		}
		
		for(; encapsulator_handler;
				encapsulator_handler = encapsulator_handler->Next()) {
			if(encapsulator_handler->Protocol() == rejected)
				encapsulator_handler->SetEnabled(false);
					// disable encapsulator
		}
		
		RXJGoodEvent(packet);
		
		// notify parent, too
		if(Interface()->Parent())
			Interface()->Parent()->StateMachine().RXJEvent(packet);
	}
}


// actions (all private)
void
PPPStateMachine::IllegalEvent(PPP_EVENT event)
{
	// TODO:
	// update error statistics
}


void
PPPStateMachine::ThisLayerUp()
{
	LockerHelper locker(fLock);
	
	// We begin with authentication phase and wait until each phase is done.
	// We stop when we reach established phase.
	
	// Do not forget to check if we are going down.
	if(Phase() != PPP_ESTABLISHMENT_PHASE)
		return;
	
	NewPhase(PPP_AUTHENTICATION_PHASE);
	
	locker.UnlockNow();
	
	BringHandlersUp();
}


void
PPPStateMachine::ThisLayerDown()
{
	// PPPProtocol/Encapsulator::Down() should block if needed.
	DownProtocols();
	DownEncapsulators();
}


void
PPPStateMachine::ThisLayerStarted()
{
	if(Interface()->Device())
		Interface()->Device()->Up();
}


void
PPPStateMachine::ThisLayerFinished()
{
	if(Interface()->Device())
		Interface()->Device()->Down();
}


void
PPPStateMachine::InitializeRestartCount()
{
	fRequestCounter = fMaxRequest;
	fTerminateCounter = fMaxTerminate;
	fNakCounter = fMaxNak;
	
	LockerHelper locker(fLock);
	fNextTimeout = system_time() + PPP_STATE_MACHINE_TIMEOUT;
}


void
PPPStateMachine::ZeroRestartCount()
{
	fRequestCounter = 0;
	fTerminateCounter = 0;
	fNakCounter = 0;
	
	LockerHelper locker(fLock);
	fNextTimeout = system_time() + PPP_STATE_MACHINE_TIMEOUT;
}


void
PPPStateMachine::SendConfigureRequest()
{
	--fRequestCounter;
	
	PPPConfigurePacket request(PPP_CONFIGURE_REQUEST);
	request.SetID(NextID());
	fRequestID = request.ID();
	
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		// add all items
		if(!LCP().OptionHandlerAt(index)->AddToRequest(&request)) {
			CloseEvent();
			return;
		}
	}
	
	LCP().Send(request.ToMbuf(LCP().AdditionalOverhead()));
}


void
PPPStateMachine::SendConfigureAck(struct mbuf *packet)
{
	mtod(packet, ppp_lcp_packet*)->code = PPP_CONFIGURE_ACK;
	PPPConfigurePacket ack(packet);
	
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++)
		LCP().OptionHandlerAt(index)->SendingAck(&ack);
	
	LCP().Send(packet);
}


void
PPPStateMachine::SendConfigureNak(struct mbuf *packet)
{
	ppp_lcp_packet *nak = mtod(packet, ppp_lcp_packet*);
	if(nak->code == PPP_CONFIGURE_NAK) {
		if(fNakCounter == 0) {
			// We sent enough naks. Let's try a reject.
			nak->code = PPP_CONFIGURE_REJECT;
		} else
			--fNakCounter;
	}
	
	LCP().Send(packet);
}


void
PPPStateMachine::SendTerminateRequest()
{
	struct mbuf *m = m_gethdr(MT_DATA);
	if(!m)
		return;
	
	--fTerminateCounter;
	
	m->m_len = 4;
	
	// reserve some space for other protocols
	m->m_data += LCP().AdditionalOverhead();
	
	ppp_lcp_packet *request = mtod(m, ppp_lcp_packet*);
	request->code = PPP_TERMINATE_REQUEST;
	request->id = fTerminateID = NextID();
	request->length = htons(4);
	
	LCP().Send(m);
}


void
PPPStateMachine::SendTerminateAck(struct mbuf *request)
{
	struct mbuf *reply = request;
	
	ppp_lcp_packet *ack;
	
	if(!reply) {
		reply = m_gethdr(MT_DATA);
		if(!reply)
			return;
		
		reply->m_data += LCP().AdditionalOverhead();
		reply->m_len = 4;
		
		ack = mtod(reply, ppp_lcp_packet*);
		ack->id = NextID();
		ack->length = 4;
	}
	
	ack = mtod(reply, ppp_lcp_packet*);
	ack->code = PPP_TERMINATE_ACK;
	
	LCP().Send(reply);
}


void
PPPStateMachine::SendCodeReject(struct mbuf *packet, uint16 protocol, uint8 type)
{
	int32 length;
		// additional space needed for this reject
	if(type == PPP_PROTOCOL_REJECT)
		length = 6;
	else
		length = 4;
	
	int32 adjust = 0;
		// adjust packet size by this value if packet is too big
	if(packet->m_flags & M_PKTHDR) {
		if((uint32) packet->m_pkthdr.len > Interface()->LinkMTU())
			adjust = Interface()->LinkMTU() - packet->m_pkthdr.len;
	} else if(packet->m_len > Interface()->LinkMTU())
		adjust = Interface()->LinkMTU() - packet->m_len;
	
	if(adjust != 0)
		m_adj(packet, adjust);
	
	ppp_lcp_packet *reject = mtod(packet, ppp_lcp_packet*);
	reject->code = type;
	reject->id = NextID();
	if(packet->m_flags & M_PKTHDR)
		reject->length = htons(packet->m_pkthdr.len);
	else
		reject->length = htons(packet->m_len);
	
	protocol = htons(protocol);
	if(type == PPP_PROTOCOL_REJECT)
		memcpy(&reject->data, &protocol, sizeof(protocol));
	
	LCP().Send(packet);
}


void
PPPStateMachine::SendEchoReply(struct mbuf *request)
{
	ppp_lcp_packet *reply = mtod(request, ppp_lcp_packet*);
	reply->code = PPP_ECHO_REPLY;
		// the request becomes a reply
	
	LCP().Send(request);
}


// methods for bringing protocols/encapsulators up
void
PPPStateMachine::BringHandlersUp()
{
	while(Phase() <= PPP_ESTABLISHED_PHASE && Phase() >= PPP_AUTHENTICATION_PHASE) {
		if(BringPhaseUp() > 0)
			break;
		
		LockerHelper locker(fLock);
		
		if(Phase() < PPP_AUTHENTICATION_PHASE)
			return;
		else if(Phase() == PPP_ESTABLISHED_PHASE) {
			if(Interface()->Parent())
				Interface()->Parent()->StateMachine().UpEvent(Interface());
			
			if(Interface()->Ifnet())
				Interface()->Ifnet()->if_flags |= IFF_RUNNING;
			
			Interface()->Report(PPP_CONNECTION_REPORT, PPP_REPORT_UP_SUCCESSFUL, NULL, 0);
		} else
			NewPhase((PPP_PHASE) (Phase() + 1));
	}
}


// this returns the number of handlers waiting to go up
uint32
PPPStateMachine::BringPhaseUp()
{
	LockerHelper locker(fLock);
	
	if(Phase() < PPP_AUTHENTICATION_PHASE)
		return 0;
	
	uint32 count = 0;
	PPPProtocol *protocol_handler;
	PPPEncapsulator *encapsulator_handler = Interface()->FirstEncapsulator();
	
	for(int32 index = 0; index < Interface()->CountProtocols(); index++) {
		protocol_handler = Interface()->ProtocolAt(index);
		if(protocol_handler && protocol_handler->Phase() == Phase()) {
			if(protocol_handler->IsUpRequested()) {
				++count;
				protocol_handler->Up();
			} else if(protocol_handler->IsGoingUp())
				++count;
		}
	}
	
	for(; encapsulator_handler;
			encapsulator_handler = encapsulator_handler->Next()) {
		if(encapsulator_handler && encapsulator_handler->Phase() == Phase()) {
			if(encapsulator_handler->IsUpRequested()) {
				++count;
				encapsulator_handler->Up();
			} else if(encapsulator_handler->IsGoingUp())
				++count;
		}
	}
	
	return count;
}


void
PPPStateMachine::DownProtocols()
{
	for(int32 index = 0; index < Interface()->CountProtocols(); index++)
		if(Interface()->ProtocolAt(index)->IsEnabled())
			Interface()->ProtocolAt(index)->Down();
}


void
PPPStateMachine::DownEncapsulators()
{
	PPPEncapsulator *encapsulator_handler = Interface()->FirstEncapsulator();
	
	for(; encapsulator_handler;
			encapsulator_handler = encapsulator_handler->Next())
		if(encapsulator_handler->IsEnabled())
			encapsulator_handler->Down();
}


void
PPPStateMachine::ResetOptionHandlers()
{
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++)
		LCP().OptionHandlerAt(index)->Reset();
}
