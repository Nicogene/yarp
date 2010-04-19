// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2006, 2007 Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#include <yarp/os/impl/InputProtocol.h>
#include <yarp/os/impl/Logger.h>
#include <yarp/os/impl/PortCore.h>
#include <yarp/os/impl/BufferedConnectionWriter.h>
#include <yarp/os/impl/NameClient.h>
#include <yarp/os/impl/PortCoreInputUnit.h>
#include <yarp/os/impl/PortCoreOutputUnit.h>
#include <yarp/os/impl/StreamConnectionReader.h>
#include <yarp/os/impl/Name.h>

#include <yarp/os/impl/Companion.h>
#include <yarp/os/Network.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Time.h>

#include <ace/OS_NS_stdio.h>


//#define YMSG(x) ACE_OS::printf x;
//#define YTRACE(x) YMSG(("at %s\n",x))

#define YMSG(x)
#define YTRACE(x) 

using namespace yarp::os::impl;
using namespace yarp::os;
using namespace yarp::os::impl;
using namespace yarp;

/*
  Phases:
  dormant
  listening
  running
*/

PortCore::~PortCore() {
    closeMain();
}


bool PortCore::listen(const Address& address) {
    bool success = false;

    if (!NetworkBase::initialized()) {
        YARP_ERROR(log, "YARP not initialized; create a yarp::os::Network object before using ports");
        return false;
    }

    YTRACE("PortCore::listen");

    if (!address.isValid()) {
        YARP_ERROR(log, "Port does not have a valid address");
        return false;
    }

    YARP_ASSERT(address.isValid());

    // try to enter listening phase
    stateMutex.wait();
    YARP_ASSERT(listening==false);
    YARP_ASSERT(running==false);
    YARP_ASSERT(closing==false);
    YARP_ASSERT(finished==false);
    YARP_ASSERT(face==NULL);
    this->address = address;
    setName(address.getRegName());

    face = Carriers::listen(address);

    if (face==NULL) {
        return false;
    }

    bool announce = false;
    if (face!=NULL) {
        listening = true;
        success = true;
        announce = true;
    }

    if (success) {
        log.setPrefix(address.getRegName().c_str());
    }

    stateMutex.post();

    if (announce) {
        NameClient& nic = NameClient::getNameClient();
        if (!nic.isFakeMode()) {
            ConstString serverName = NetworkBase::getNameServerName();
            ConstString portName = address.getRegName().c_str();
            if (serverName!=portName) {
                Bottle cmd, reply;
                cmd.addString("announce");
                cmd.addString(portName.c_str());
                NetworkBase::write(NetworkBase::getNameServerContact(),
                                   cmd, reply);
            }
        }
    }

    // we have either entered listening phase (face=valid, listening=true)
    // or remained in dormant phase

    return success;
}


void PortCore::setReadHandler(Readable& reader) {
    YARP_ASSERT(running==false);
    YARP_ASSERT(this->reader==NULL);
    this->reader = &reader;
}

void PortCore::setReadCreator(ReadableCreator& creator) {
    YARP_ASSERT(running==false);
    YARP_ASSERT(this->readableCreator==NULL);
    this->readableCreator = &creator;
}



void PortCore::run() {
    YTRACE("PortCore::run");

    // enter running phase
    YARP_ASSERT(listening==true);
    YARP_ASSERT(running==false);
    YARP_ASSERT(closing==false);
    YARP_ASSERT(finished==false);
    YARP_ASSERT(starting==true); // can only run if called from start
    running = true;
    starting = false;
    stateMutex.post();

    YTRACE("PortCore::run running");

    // main loop
    bool shouldStop = false;
    while (!shouldStop) {

        // block and wait for an event
        InputProtocol *ip = NULL;
        ip = face->read();

        // got an event, but before processing it, we check whether
        // we should shut down
        stateMutex.wait();
        if (ip!=NULL) {
            YARP_DEBUG(log,"PortCore received something");
        }
        shouldStop |= closing;
        events++;
        stateMutex.post();

        if (!shouldStop) {
            // process event
            if (ip!=NULL) {
                addInput(ip);
            }
            YARP_DEBUG(log,"PortCore spun off a connection");
            ip = NULL;
        }

        // the event normally gets handed off.  If it remains, delete it.
        if (ip!=NULL) {
            ip->close();
            delete ip;
            ip = NULL;
        }
        reapUnits();
        stateMutex.wait();
        for (int i=0; i<connectionListeners; i++) {
            connectionChange.post();
        }
        connectionListeners = 0;
        stateMutex.post();
    }

    YTRACE("PortCore::run closing");

    // closing phase
    stateMutex.wait();
    for (int i=0; i<connectionListeners; i++) {
        connectionChange.post();
    }
    connectionListeners = 0;
    finished = true;
    stateMutex.post();
}


void PortCore::close() {
    closeMain();
}


bool PortCore::start() {
    YTRACE("PortCore::start");

    stateMutex.wait();
    YARP_ASSERT(listening==true);
    YARP_ASSERT(running==false);
    YARP_ASSERT(starting==false);
    YARP_ASSERT(finished==false);
    YARP_ASSERT(closing==false);
    starting = true;
    bool started = ThreadImpl::start();
    if (!started) {
        // run() won't be happening
        stateMutex.post();
    } else {
        // wait for run() to change state
        stateMutex.wait();
        YARP_ASSERT(running==true);
        stateMutex.post();
    }
    return started;
}



void PortCore::interrupt() {
    if (interruptible) {
        stateMutex.wait();
        // Check if someone is waiting for input.  If so, wake them up
        if (reader!=NULL) {
            // send empty data out
            YARP_DEBUG(log,"sending interrupt message to listener");
            StreamConnectionReader sbr;
            reader->read(sbr);
        }
        stateMutex.post();
    }
}


void PortCore::closeMain() {
    stateMutex.wait();
    if (finishing||!running) {
        stateMutex.post();
        return;
    }

    YTRACE("PortCore::closeMain");

    // Politely pre-disconnect inputs
    finishing = true;
    YARP_DEBUG(log,"now preparing to shut down port");
    stateMutex.post();

    bool done = false;
    String prevName = "";
    while (!done) {
        done = true;
        String removeName = "";
        stateMutex.wait();
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                if (unit->isInput()) {
                    if (!unit->isDoomed()) {
                        Route r = unit->getRoute();
                        String s = r.getFromName();
                        if (s.length()>=1) {
                            if (s[0]=='/') {
                                if (s!=getName()) {
                                    if (s!=prevName) {
                                        removeName = s;
                                        done = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        stateMutex.post();
        if (!done) {
            YARP_DEBUG(log,String("requesting removal of connection from ")+
                       removeName);
            int result = Companion::disconnect(removeName.c_str(),
                                               getName().c_str(),
                                               true);
            if (result!=0) {
                Companion::disconnectInput(getName().c_str(),
                                           removeName.c_str(),
                                           true);
            }
            prevName = removeName;
        }
    }

	// politely remove all outputs
	done = false;
    while (!done) {
        done = true;
        Route removeRoute;
        stateMutex.wait();
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                if (unit->isOutput()&&!unit->isFinished()) {
					removeRoute = unit->getRoute();
					if (removeRoute.getFromName()==getName()) {
						done = false;
						break;
					}
                }
            }
        }
        stateMutex.post();
        if (!done) {
			removeUnit(removeRoute,true);
        }
    }

    stateMutex.wait();
    bool stopRunning = running;
    stateMutex.post();

    if (stopRunning) {
        // we need to stop the thread
        stateMutex.wait();
        closing = true;
        stateMutex.post();

        // wake it up
        OutputProtocol *op = face->write(address);
        if (op!=NULL) {
            op->close();
            delete op;
        }
        join();

        // should be finished
        stateMutex.wait();
        YARP_ASSERT(finished==true);
        stateMutex.post();

        // should down units - this is the only time it is valid to do this
        closeUnits();

        stateMutex.wait();
        finished = false;
        closing = false;
        running = false;
        stateMutex.post();

        String name = getName();
        if (name!=String("")) {
            if (controlRegistration) {
                NameClient::getNameClient().unregisterName(name);
            }
        }
    }

    // there should be no other threads at this point
    // can stop listening

    if (listening) {
        YARP_ASSERT(face!=NULL);
        face->close();
        delete face;
        face = NULL;
        listening = false;
    }

    // Check if someone is waiting for input.  If so, wake them up
    if (reader!=NULL) {
        // send empty data out
        YARP_DEBUG(log,"sending end-of-port message to listener");
        StreamConnectionReader sbr;
        reader->read(sbr);
        reader = NULL;
    }
    finishing = false;

    // fresh as a daisy
    YARP_ASSERT(listening==false);
    YARP_ASSERT(running==false);
    YARP_ASSERT(starting==false);
    YARP_ASSERT(closing==false);
    YARP_ASSERT(finished==false);
    YARP_ASSERT(finishing==false);
    YARP_ASSERT(face==NULL);
}


int PortCore::getEventCount() {
    stateMutex.wait();
    int ct = events;
    stateMutex.post();
    return ct;
}


void PortCore::closeUnits() {
    stateMutex.wait();
    YARP_ASSERT(finished==true); // this is the only valid phase for this
    stateMutex.post();

    // in the "finished" phase, nobody else touches the units,
    // so we can go ahead and shut them down and delete them

    for (unsigned int i=0; i<units.size(); i++) {
        PortCoreUnit *unit = units[i];
        if (unit!=NULL) {
            YARP_DEBUG(log,"closing a unit");
            unit->close();
            YARP_DEBUG(log,"joining a unit");
            unit->join();
            delete unit;
            YARP_DEBUG(log,"deleting a unit");
            units[i] = NULL;
        }
    }
    units.clear();
}

void PortCore::reapUnits() {
    stateMutex.wait();
    if (!finished) {
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                if (unit->isDoomed()&&!unit->isFinished()) {	
                    String s = unit->getRoute().toString();
                    YARP_DEBUG(log,String("Informing connection ") + 
                               s + " that it is doomed");
                    unit->close();
                    YARP_DEBUG(log,String("Closed connection ") + 
                               s);
                    YARP_DEBUG(log,"closed REAPING a unit");
                    unit->join();
                    YARP_DEBUG(log,String("Joined thread of connection ") + 
                               s);
                } 
            }
        }
    }
    stateMutex.post();
    cleanUnits();
}

void PortCore::cleanUnits() {
    int updatedInputCount = 0;
    int updatedOutputCount = 0;
    int updatedDataOutputCount = 0;
    YARP_DEBUG(log,"/ routine check of connections to this port begins");
    stateMutex.wait();
    if (!finished) {
    
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                YARP_DEBUG(log,String("| checking connection ") + unit->getRoute().toString() + " " + unit->getMode());
                if (unit->isFinished()) {
                    String con = unit->getRoute().toString();
                    YARP_DEBUG(log,String("|   removing connection ") + con);
                    unit->close();
                    unit->join();
                    delete unit;
                    units[i] = NULL;
                    YARP_DEBUG(log,String("|   removed connection ") + con);
                } else {
                    if (!unit->isDoomed()) {
                        if (unit->isOutput()) {
                            updatedOutputCount++;
                            if (unit->getMode()=="") {
                                updatedDataOutputCount++;
                            }
                        }
                        if (unit->isInput()) {
                            if (unit->getRoute().getFromName()!="admin") {
                                updatedInputCount++;
                            }
                        }
                    }
                }
            }
        }
        unsigned int rem = 0;
        for (unsigned int i2=0; i2<units.size(); i2++) {
            if (units[i2]!=NULL) {
                if (rem<i2) {
                    units[rem] = units[i2];
                    units[i2] = NULL;
                }
                rem++;
            }
        }
        for (unsigned int i3=0; i3<units.size()-rem; i3++) {
            units.pop_back();
        }
        //YMSG(("cleanUnits: there are now %d units\n", units.size()));
    }
    inputCount = updatedInputCount;
    outputCount = updatedOutputCount;
    dataOutputCount = updatedDataOutputCount;
    stateMutex.post();
    YARP_DEBUG(log,"\\ routine check of connections to this port ends");
}


// only called by manager, in running phase
void PortCore::addInput(InputProtocol *ip) {
    YARP_ASSERT(ip!=NULL);
    stateMutex.wait();
    PortCoreUnit *unit = new PortCoreInputUnit(*this,ip,autoHandshake);
    YARP_ASSERT(unit!=NULL);
    unit->start();
  
    units.push_back(unit);
    YMSG(("there are now %d units\n", units.size()));
    stateMutex.post();
}


void PortCore::addOutput(OutputProtocol *op) {
    YARP_ASSERT(op!=NULL);

    stateMutex.wait();
    if (!finished) {
        PortCoreUnit *unit = new PortCoreOutputUnit(*this,op);
        YARP_ASSERT(unit!=NULL);
    
        unit->start();
    
        units.push_back(unit);
        //YMSG(("there are now %d units\n", units.size()));
    }
    stateMutex.post();
}


bool PortCore::isUnit(const Route& route) {
    // not mutexed
    bool needReap = false;
    if (!finished) {
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                Route alt = unit->getRoute();
                String wild = "*";
                bool ok = true;
                if (route.getFromName()!=wild) {
                    ok = ok && (route.getFromName()==alt.getFromName());
                }
                if (route.getToName()!=wild) {
                    ok = ok && (route.getToName()==alt.getToName());
                }
                if (route.getCarrierName()!=wild) {
                    ok = ok && (route.getCarrierName()==alt.getCarrierName());
                }
	
                if (ok) {
                    needReap = true;
                    break;
                }
            }
        }
    }
    //printf("Reporting %s as %d\n", route.toString().c_str(), needReap);
    return needReap;
}


bool PortCore::removeUnit(const Route& route, bool synch) {
    // a request to remove a unit
    // this is the trickiest case, since any thread could here
    // affect any other thread

    YARP_DEBUG(log,String("asked to remove connection ") + route.toString());

    // how about waking up the manager to do this?
    stateMutex.wait();
    bool needReap = false;
    if (!finished) {
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                Route alt = unit->getRoute();
                String wild = "*";
                bool ok = true;
                if (route.getFromName()!=wild) {
                    ok = ok && (route.getFromName()==alt.getFromName());
                }
                if (route.getToName()!=wild) {
                    ok = ok && (route.getToName()==alt.getToName());
                }
                if (route.getCarrierName()!=wild) {
                    ok = ok && (route.getCarrierName()==alt.getCarrierName());
                }
	
                if (ok) {
                    YARP_DEBUG(log, 
                               String("removing connection ") + alt.toString());
					unit->setDoomed();
                    needReap = true;
                    if (route.getToName()!="*") {
						// not needed any more
                        //Companion::disconnectInput(alt.getToName().c_str(),
                        //                         alt.getFromName().c_str(),
                        //                       true);
						break;
                    }
                }
            }
        }
    }
    stateMutex.post();
    if (needReap) {
        YARP_DEBUG(log,"one or more connections need prodding to die");
        // death will happen in due course; we can speed it up a bit
        // by waking up the grim reaper
        OutputProtocol *op = face->write(address);
        if (op!=NULL) {
            op->close();
            delete op;
        }
        YARP_DEBUG(log,"sent message to prod connection death");
        
        if (synch) {
            YARP_DEBUG(log,"synchronizing with connection death");
            // wait until disconnection process is complete
            bool cont = false;
            do {
                stateMutex.wait();
                cont = isUnit(route);
                if (cont) {
                    connectionListeners++;
                }
                stateMutex.post();
                if (cont) {
                    connectionChange.wait();
                }
            } while (cont);
        }
    }
    return needReap;
}




////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// PortManager interface
//


void PortCore::addOutput(const String& dest, void *id, OutputStream *os) {
    YARP_DEBUG(log,String("asked to add output to ")+
               dest);

    BufferedConnectionWriter bw(true);

    Address parts = Name(dest).toAddress();
    Address address = NameClient::getNameClient().queryName(parts.getRegName());
    if (address.isValid()) {
        // as a courtesy, remove any existing connections between 
        // source and destination
        removeUnit(Route(getName(),address.getRegName(),"*"),true);

        Route r = Route(getName(),address.getRegName(),
                        parts.hasCarrierName()?parts.getCarrierName():"tcp");

        bool allowed = true;

        // apply any restrictions on the port
        int f = getFlags();
        bool allow_output = (f&PORTCORE_IS_OUTPUT);
        bool rpc = (f&PORTCORE_IS_RPC);
        Name name(r.getCarrierName() + String("://test"));
        String mode = name.getCarrierModifier("log");
        bool is_log = (mode!="");
        String err = "";
        if (!allow_output) {
            if (!is_log) {
                err = "Outputs not allowed";
                allowed = false;
            }
        } else if (rpc) {
            if (dataOutputCount>=1 && !is_log) {
                err = "RPC output already connected";
                allowed = false;
            }
        }
        
        if (!allowed) {
            bw.appendLine(err);
        } else {
            OutputProtocol *op = NULL;
            op = Carriers::connect(address);
            if (op!=NULL) {
                
                bool ok = op->open(r);
                if (!ok) {
                    YARP_DEBUG(log,"open route error");            
                    delete op;
                    op = NULL;
                }
            }
            if (op!=NULL) {
                addOutput(op);
                bw.appendLine(String("Added output connection from ") + getName() + " to " + dest);
            } else {
                bw.appendLine(String("Cannot connect to ") + dest);
            }
        }
    } else {
        bw.appendLine(String("Do not know how to connect to ") + dest);
    }

    if(os!=NULL) {
        bw.write(*os);
    }
    cleanUnits();
}

void PortCore::removeOutput(const String& dest, void *id, OutputStream *os) {
    BufferedConnectionWriter bw(true);
    if (removeUnit(Route("*",dest,"*"),true)) {
        bw.appendLine(String("Removed connection from ") + getName() +
                      " to " + dest);
    } else {
        bw.appendLine(String("Could not find an outgoing connection to ") +
                      dest);
    }
    if(os!=NULL) {
        bw.write(*os);
    }
    cleanUnits();
}

void PortCore::removeInput(const String& dest, void *id, OutputStream *os) {
    BufferedConnectionWriter bw(true);
    if (removeUnit(Route(dest,"*","*"),true)) {
        bw.appendLine(String("Removing connection from ") + dest + " to " +
                      getName());
    } else {
        bw.appendLine(String("Could not find an incoming connection from ") +
                      dest);
    }
    if(os!=NULL) {
        bw.write(*os);
    }
    cleanUnits();
}

void PortCore::describe(void *id, OutputStream *os) {
    cleanUnits();

    BufferedConnectionWriter bw(true);

    stateMutex.wait();

    bw.appendLine(String("This is ") + address.getRegName() + " at " + 
                  address.toString());

    int oct = 0;
    int ict = 0;
    for (unsigned int i=0; i<units.size(); i++) {
        PortCoreUnit *unit = units[i];
        if (unit!=NULL) {
            if (unit->isOutput()&&!unit->isFinished()) {
                Route route = unit->getRoute();
                String msg = "There is an output connection from " + 
                    route.getFromName() +
                    " to " + route.getToName() + " using " + 
                    route.getCarrierName();
                bw.appendLine(msg);
                oct++;
            }
        }
    }
    if (oct<1) {
        bw.appendLine("There are no outgoing connections");
    } 
    for (unsigned int i2=0; i2<units.size(); i2++) {
        PortCoreUnit *unit = units[i2];
        if (unit!=NULL) {
            if (unit->isInput()&&!unit->isFinished()) {
                Route route = unit->getRoute();
                String msg = "There is an input connection from " + 
                    route.getFromName() +
                    " to " + route.getToName() + " using " + 
                    route.getCarrierName();
                bw.appendLine(msg);
                ict++;
            }
        }
    }
    if (ict<1) {
        bw.appendLine("There are no incoming connections");
    } 

    stateMutex.post();

    if (os!=NULL) {
        bw.write(*os);
    } else {
        StringOutputStream sos;
        bw.write(sos);
        printf("%s\n",sos.toString().c_str());
    }
}


void PortCore::describe(PortReport& reporter) {
    cleanUnits();

    stateMutex.wait();

    PortInfo baseInfo;
    baseInfo.tag = yarp::os::PortInfo::PORTINFO_MISC;
    ConstString portName = address.getRegName().c_str();
    baseInfo.message = (String("This is ") + portName.c_str() + " at " + 
                        address.toString()).c_str();
    reporter.report(baseInfo);

    int oct = 0;
    int ict = 0;
    for (unsigned int i=0; i<units.size(); i++) {
        PortCoreUnit *unit = units[i];
        if (unit!=NULL) {
            if (unit->isOutput()&&!unit->isFinished()) {
                Route route = unit->getRoute();
                String msg = "There is an output connection from " + 
                    route.getFromName() +
                    " to " + route.getToName() + " using " + 
                    route.getCarrierName();
                PortInfo info;
                info.message = msg.c_str();
                info.tag = yarp::os::PortInfo::PORTINFO_CONNECTION;
                info.incoming = false;
                info.portName = portName;
                info.sourceName = route.getFromName().c_str();
                info.targetName = route.getToName().c_str();
                info.carrierName = route.getCarrierName().c_str();
                reporter.report(info);                
                oct++;
            }
        }
    }
    if (oct<1) {
        PortInfo info;
        info.tag = yarp::os::PortInfo::PORTINFO_MISC;
        info.message = "There are no outgoing connections";
        reporter.report(info);
    } 
    for (unsigned int i2=0; i2<units.size(); i2++) {
        PortCoreUnit *unit = units[i2];
        if (unit!=NULL) {
            if (unit->isInput()&&!unit->isFinished()) {
                Route route = unit->getRoute();
                String msg = "There is an input connection from " + 
                    route.getFromName() +
                    " to " + route.getToName() + " using " + 
                    route.getCarrierName();
                PortInfo info;
                info.message = msg.c_str();
                info.tag = yarp::os::PortInfo::PORTINFO_CONNECTION;
                info.incoming = true;
                info.portName = portName;
                info.sourceName = route.getFromName().c_str();
                info.targetName = route.getToName().c_str();
                info.carrierName = route.getCarrierName().c_str();
                reporter.report(info);                
                ict++;
            }
        }
    }
    if (ict<1) {
        PortInfo info;
        info.tag = yarp::os::PortInfo::PORTINFO_MISC;
        info.message = "There are no incoming connections";
        reporter.report(info);
    } 

    stateMutex.post();
}


void PortCore::setReportCallback(yarp::os::PortReport *reporter) {
   stateMutex.wait();
   if (reporter!=NULL) {
       eventReporter = reporter;
   }
   stateMutex.post();     
}


void PortCore::report(const PortInfo& info) {
    // we are in the context of one of the input or output threads,
    // so our contact with the PortCore must be absolutely minimal.
    //
    // it is safe to pick up the address of the reporter if this is 
    // kept constant over the lifetime of the input/output threads.

    if (eventReporter!=NULL) {
        eventReporter->report(info);
    }
}




bool PortCore::readBlock(ConnectionReader& reader, void *id, OutputStream *os) {
    bool result = true;
    // pass the data on out

    // we are in the context of one of the input threads,
    // so our contact with the PortCore must be absolutely minimal.
    //
    // it is safe to pick up the address of the reader since this is 
    // constant over the lifetime of the input threads.

    if (this->reader!=NULL) {
        interruptible = false; // no mutexing; user of interrupt() has to be
                               // careful

        bool haveOutputs = (outputCount!=0); // no mutexing, but failure
        // modes give fine behavior

        if (logNeeded&&haveOutputs) {
            // Normally, yarp doesn't pay attention to the content of 
            // messages received by the client.  Likewise, the content
            // of replies are not monitored.  However it may sometimes 
            // be useful this traffic.

            ConnectionRecorder recorder;
            recorder.init(&reader);
            result = this->reader->read(recorder);
            recorder.fini();
            // send off a log of this transaction to whoever wants it
            sendHelper(recorder,PORTCORE_SEND_LOG);
        } else {
            // YARP is not needed as a middleman
            result = this->reader->read(reader);
        }

        interruptible = true;
    } else {
        // read and ignore
        YARP_DEBUG(Logger::get(),"data received in PortCore, no reader for it");
        Bottle b;
        result = b.read(reader);
    }
    return result;
}


bool PortCore::send(Writable& writer, Readable *reader, Writable *callback) {
    if (!logNeeded) {
        return sendHelper(writer,PORTCORE_SEND_NORMAL,reader,callback);
    }
    // logging is desired, so we need to wrap up and log this send
    // (and any reply it gets)

    // NOT IMPLEMENTED YET

    return sendHelper(writer,PORTCORE_SEND_NORMAL,reader,callback);
}

bool PortCore::sendHelper(Writable& writer, 
                          int mode, Readable *reader, Writable *callback) {

    int logCount = 0;
    String envelopeString = envelope;

    // pass the data to all output units.
    // for efficiency, it should be converted to block form first.
    // some ports may want text-mode, some may want binary, so there
    // may need to be two caches.

    // for now, just doing a sequential send with no caching.
    // (mcast protocol can be used to avoid duplicated effort)
    YMSG(("------- send in real\n"));

    writer.onCommencement();

    stateMutex.wait();

    YMSG(("------- send in\n"));
    // The whole darned port is blocked on this operation.
    // How long the operation lasts will depend on these flags:
    //   waitAfterSend and waitBeforeSend,
    // set by setWaitAfterSend() and setWaitBeforeSend()
    if (!finished) {
        packetMutex.wait();
        PortCorePacket *packet = packets.getFreePacket();
        packet->setContent(&writer,false,callback);
        packetMutex.post();
        YARP_ASSERT(packet!=NULL);
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                if (unit->isOutput() && !unit->isFinished()) {
                    bool log = (unit->getMode()!="");
                    bool ok = (mode==PORTCORE_SEND_NORMAL)?(!log):(log);
                    if (log) {
                        logCount++;
                    }
                    if (ok) {
                        YMSG(("------- -- inc\n"));
                        packetMutex.wait();
                        packet->inc();
                        packetMutex.post();
                        YMSG(("------- -- presend\n"));
                        void *out = unit->send(writer,reader,
                                               (callback!=NULL)?callback:(&writer),
                                               (void *)packet,
                                               envelopeString,
                                               waitAfterSend,waitBeforeSend);
                        YMSG(("------- -- send\n"));
                        if (out!=NULL) {
                            packetMutex.wait();
                            ((PortCorePacket *)out)->dec();
                            packets.checkPacket((PortCorePacket *)out);
                            packetMutex.post();
                        }
                        YMSG(("------- -- dec\n"));
                    }
                }
            }
        }
        YMSG(("------- pack check\n"));
        packetMutex.wait();
        packet->dec();
        packets.checkPacket(packet);
        packetMutex.post();
        YMSG(("------- packed\n"));
    }
    YMSG(("------- send out\n"));
    if (mode==PORTCORE_SEND_LOG) {
        if (logCount==0) {
            logNeeded = false;
        }
    }
    stateMutex.post();
    YMSG(("------- send out real\n"));

    return true;
}



bool PortCore::isWriting() {
    bool writing = false;

    stateMutex.wait();

    if (!finished) {
        for (unsigned int i=0; i<units.size(); i++) {
            PortCoreUnit *unit = units[i];
            if (unit!=NULL) {
                if (unit->isOutput() && !unit->isFinished()) {
                    if (unit->isBusy()) {
                        writing = true;
                    }
                }
            }
        }
    }

    stateMutex.post();

    return writing;
}


int PortCore::getInputCount() {
    cleanUnits();
    stateMutex.wait();
    int result = inputCount;
    stateMutex.post();
    return result;
}

int PortCore::getOutputCount() {
    cleanUnits();
    stateMutex.wait();
    int result = outputCount;
    stateMutex.post();
    return result;
}



void PortCore::notifyCompletion(void *tracker) {
    YMSG(("starting notifyCompletion\n"));
    packetMutex.wait();
    if (tracker!=NULL) {
        ((PortCorePacket *)tracker)->dec();
        packets.checkPacket((PortCorePacket *)tracker);
    }
    packetMutex.post();
    YMSG(("stopping notifyCompletion\n"));
}


bool PortCore::setEnvelope(Writable& envelope) {
    BufferedConnectionWriter buf(true);
    bool ok = envelope.write(buf);
    if (ok) {
        setEnvelope(buf.toString());
    }
    return ok;
}


void PortCore::setEnvelope(const String& envelope) {
    this->envelope = envelope;
    for (unsigned int i=0; i<envelope.length(); i++) {
        if (this->envelope[i]<32) {
            this->envelope = this->envelope.substr(0,i);
            break;
        }
    }
    YARP_DEBUG(log,String("set envelope to ") + this->envelope);
}

String PortCore::getEnvelope() {
    return envelope;
}

bool PortCore::getEnvelope(Readable& envelope) {
    StringInputStream sis;
    sis.add(this->envelope.c_str());
    sis.add("\r\n");
    StreamConnectionReader sbr;
    Route route;
    sbr.reset(sis,NULL,route,0,true);
    return envelope.read(sbr);
}

#define STANZA(name,tag,val) Bottle name; name.addString(tag); name.addString(val.c_str());

bool PortCore::adminBlock(ConnectionReader& reader, void *id, 
                          OutputStream *os) {
    Bottle cmd, result;
    cmd.read(reader);

    StringOutputStream cache;
    switch (cmd.get(0).asVocab()) {
    case VOCAB4('h','e','l','p'):
        result.addString("[help] # give this help");
        result.addString("[add] $targetPort # add an output connection");
        result.addString("[add] $targetPort $carrier # add an output with a given protocol");
        result.addString("[del] $targetPort # remove an output connection");
        result.addString("[list] [in] # list input connections");
        result.addString("[list] [out] # list output connections");
        result.addString("[list] [in] $sourcePort # give details for input");
        result.addString("[list] [out] $targetPort # give details for output");
        result.addString("[ver] # report protocol version information");
        //result.addString("[get] # list property values available");
        //result.addString("[get] $prop # get value of property");
        //result.addString("[set] $prop # set value of property");
        break;
    case VOCAB3('v','e','r'):
        // This version number is for the network protocol.
        // It is distinct from the YARP library versioning.
        result.addVocab(Vocab::encode("ver"));
        result.addInt(1);
        result.addInt(2);
        result.addInt(3);
        break;
    case VOCAB3('a','d','d'):
        {
            String output = cmd.get(1).asString().c_str();
            String carrier = cmd.get(2).asString().c_str();
            if (carrier!="") {
                output = carrier + ":/" + output;
            }
            addOutput(output,id,&cache);
            result.addString(cache.toString().c_str());
        }
        break;
    case VOCAB3('d','e','l'):
        removeOutput(String(cmd.get(1).asString().c_str()),id,&cache);
        result.addString(cache.toString().c_str());
        break;
    case VOCAB4('l','i','s','t'):
        switch (cmd.get(1).asVocab()) {
        case VOCAB2('i','n'):
            {
                ConstString target = cmd.get(2).asString();
                stateMutex.wait();
                for (unsigned int i2=0; i2<units.size(); i2++) {
                    PortCoreUnit *unit = units[i2];
                    if (unit!=NULL) {
                        if (unit->isInput()&&!unit->isFinished()) {
                            Route route = unit->getRoute();
                            if (target=="") {
                                result.addString(route.getFromName().c_str());
                            } else if (route.getFromName()==target.c_str()) {
                                STANZA(bfrom,"from",route.getFromName());
                                STANZA(bto,"to",route.getToName());
                                STANZA(bcarrier,"carrier",
                                       route.getCarrierName());
                                result.addList() = bfrom;
                                result.addList() = bto;
                                result.addList() = bcarrier;
                            }
                        }
                    }
                }
                stateMutex.post();
            }
            break;
        case VOCAB3('o','u','t'):
        default:
            {
                ConstString target = cmd.get(2).asString();
                stateMutex.wait();
                for (unsigned int i=0; i<units.size(); i++) {
                    PortCoreUnit *unit = units[i];
                    if (unit!=NULL) {
                        if (unit->isOutput()&&!unit->isFinished()) {
                            Route route = unit->getRoute();
                            if (target=="") {
                                result.addString(route.getToName().c_str());
                            } else if (route.getToName()==target.c_str()) {
                                STANZA(bfrom,"from",route.getFromName());
                                STANZA(bto,"to",route.getToName());
                                STANZA(bcarrier,"carrier",
                                       route.getCarrierName());
                                result.addList() = bfrom;
                                result.addList() = bto;
                                result.addList() = bcarrier;
                            }
                        }
                    }
                }
                stateMutex.post();
            }
        }
        break;
    default:
        result.addVocab(Vocab::encode("fail"));
        result.addString("send [help] for list of valid commands");
        break;
    }

    ConnectionWriter *writer = reader.getWriter();
    if (writer!=NULL) {
        result.write(*writer);
    }

    return true;
}

void PortCore::reportUnit(PortCoreUnit *unit, bool active) {
    if (unit!=NULL) {
        bool isLog = (unit->getMode()!="");
        if (isLog) {
            logNeeded = true;
        }
    }
}
