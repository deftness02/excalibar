/******************************************************************************
Cheyenne: a real-time packet analyzer/sniffer for Dark Age of Camelot
Copyright (C) 2003, the Cheyenne Developers

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
******************************************************************************/
#include <sstream> // for stringstream defs
#include "CSLCommand.h"
#include "CSLScriptHost.h" // for EXECUTE_PARAMS members
#include "CSLKeyboard.h" // for EXECUTE_PARAMS members
#include "..\Utils\Logger.h"
#include "..\Utils\times.h" // for time
#include "..\Database\database.h" // for database
#include "..\Utils\Mapinfo.h" // for the map info

extern logger_t Logger; // logger
extern MapInfo Zones; // zone info

namespace csl
{
bool CheckTargetHealthAndCall::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws
               >> Health1 >> std::ws >> Cmd1 >> std::ws
               >> Health2 >> std::ws >> Cmd2 >> std::ws
               >> Health3 >> std::ws >> Cmd3 >> std::ws;
    
    if(!Health1)
        {
        ::Logger << "[CheckHealthAndCall::Extract] expected non-zero Health1!" << std::endl;
        return(false);
        }
    else if(!Health2)
        {
        ::Logger << "[CheckHealthAndCall::Extract] expected non-zero Health2!" << std::endl;
        return(false);
        }
    else if(!Health3)
        {
        ::Logger << "[CheckHealthAndCall::Extract] expected non-zero Health3!" << std::endl;
        return(false);
        }
    else if(!Cmd1.length())
        {
        ::Logger << "[CheckHealthAndCall::Extract] expected non-zero Cmd1!" << std::endl;
        return(false);
        }
    else if(!Cmd2.length())
        {
        ::Logger << "[CheckHealthAndCall::Extract] expected non-zero Cmd2!" << std::endl;
        return(false);
        }
    else if(!Cmd3.length())
        {
        ::Logger << "[CheckHealthAndCall::Extract] expected non-zero Cmd3!" << std::endl;
        return(false);
        }
    else
        {
        // done
        bInvoked=false;
        return(true);
        }
} // end CheckTargetHealthAndCall::Extract

csl::CSLCommandAPI::EXECUTE_STATUS CheckTargetHealthAndCall::Execute(csl::EXECUTE_PARAMS& params)
{
    // check for valid target or if we have been invoked
    if(params.targetted_actor->GetInfoId() == 0 || bInvoked)
        {
        // clear invoked flag
        bInvoked=false;
        // invalid target, done and go on to next command
        return(std::make_pair(true,true));
        }
    else if(params.targetted_actor->GetHealth() <= Health3)
        {
        // invoke now, stay on command
        bInvoked=true;
        // call Cmd3
        return(std::make_pair(params.script_host->CallScript(Cmd3,params),false));
        }
    else if(params.targetted_actor->GetHealth() <= Health2)
        {
        // invoke now, stay on command
        bInvoked=true;
        // call Cmd2
        return(std::make_pair(params.script_host->CallScript(Cmd2,params),false));
        }
    else if(params.targetted_actor->GetHealth() <= Health1)
        {
        // invoke now, stay on command
        bInvoked=true;
        // call Cmd1
        return(std::make_pair(params.script_host->CallScript(Cmd1,params),false));
        }
    else
        {
        // target is healthy, done and go on to next command
        return(std::make_pair(true,true));
        }
} // end CheckTargetHealthAndCall::Execute

bool SetReferenceActor::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name >> std::ws;
    
    if(Name.length() == 0)
        {
        ::Logger << "[SetReferenceActor::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        return(true);
        }
} // end SetReferenceActor::Extract

csl::CSLCommandAPI::EXECUTE_STATUS SetReferenceActor::Execute(csl::EXECUTE_PARAMS& params)
{
    Actor NewReferenceActor;
    
    if(!params.database->CopyActorByName(Name,NewReferenceActor))
        {
        // did not find it
        ::Logger << "[SetReferenceActor::Execute] Could not find actor with name \"" << Name << "\"" << std::endl;
        return(std::make_pair(false,true));
        }
    else
        {
        // assign via scripthost
        params.script_host->SetReferenceActor(NewReferenceActor);
        
        // done, success
        return(std::make_pair(true,true));
        }
} // end SetReferenceActor::Execute

bool MoveToPoint::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> x >> std::ws >> y >> std::ws >> time_limit >> std::ws;
    
    if(x<0 || x > 65535)
        {
        ::Logger << "[MoveToPoint::Extract] expected x to be [0,65535]" << std::endl;
        return(false);
        }
    else if(y<0 || y > 65535)
        {
        ::Logger << "[MoveToPoint::Extract] expected y to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToPoint::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }

    // init to -
    start_time=-1.0;
    moving=false;
    turning=false;
    close=false;
    
    if(!head_point)
        {
        // create our head to point proxy
        head_point=new csl::HeadPoint;
        }
    
    // make arguments for headpoint, always give it 10 seconds
    std::stringstream ss;
    ss << x << " " << y << " " << 10.0 << std::endl;
    
    // done, return extract status for head_point
    return(head_point->Extract(ss));
} // end MoveToPoint::Extract

float MoveToPoint::GetDistanceFromGoal(const Actor* reference)const
{
    // see if we are there yet
    unsigned int curr_x,curr_y;
    unsigned short curr_z;
    MapInfo::ZoneIndexType zone;
    
    // get zone relative current coordinates
    ::Zones.GetZoneFromGlobal
        (
        reference->GetRegion(),
        unsigned int(reference->GetMotion().GetXPos()),
        unsigned int(reference->GetMotion().GetYPos()),
        unsigned short(reference->GetMotion().GetZPos()),
        curr_x,
        curr_y,
        curr_z,
        zone
        );
        
    // get distance from target
    const float delta_x=float(curr_x)-float(x);
    const float delta_y=float(curr_y)-float(y);
    
    // return distance 
    return(sqrt(delta_x*delta_x + delta_y*delta_y));
} // end GetDistanceFromGoal

void MoveToPoint::Reinit(csl::EXECUTE_PARAMS& params)
{
    // clear these
    start_time=-1.0;
    last_heading_check=0.0;

    if(moving)
        {
        // release move key
        params.keyboard->PressAndReleaseVK("VK_UP");
        }
    
    if(turning)
        {
        // release any turning keys
        params.keyboard->PressAndReleaseVK("VK_LEFT");
        params.keyboard->PressAndReleaseVK("VK_RIGHT");
        }

    // clear flag
    moving=false;
    turning=false;
    close=false;
    
    // done with this
    delete head_point;
    head_point=0;
} // end Reinit

csl::CSLCommandAPI::EXECUTE_STATUS MoveToPoint::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we are following someone
    if(!params.followed_actor->GetInfoId())
        {
        ::Logger << "[MoveToPoint::Execute] move to <" << x << "," << y << "> failed to complete in "
                 << time_limit << " seconds because there is no followed (reference) actor" << std::endl;
        
        // reinit
        Reinit(params);
        
        // return error
        return(std::make_pair(false,true));
        } // end if no followed actor
    
    // check start time
    if(start_time==-1.0)
        {
        // we have not been executed before, init start time to now
        start_time=params.current_time->Seconds();
        
        // start turning
        turning=true;
        } // end if first iteration
        
    // check to see if time limit expired
    if(start_time+time_limit < params.current_time->Seconds())
        {
        // time limit expired!

        // reinit
        Reinit(params);
        
        // print alert in log file
        ::Logger << "[MoveToPoint::Execute] move to <" << x << "," << y << "> failed to complete in "
                 << time_limit << " seconds!" << std::endl;
                 
        // return error
        return(std::make_pair(false,true));
        } // end if time limit expired

    // status for current action
    csl::CSLCommandAPI::EXECUTE_STATUS status;
    
    // see how far away we are
    const float distance=GetDistanceFromGoal(params.followed_actor);
    if(distance < 1000.0f)
        {
        // see if we were close before
        if(close)
            {
            if(distance < 100.0f)
                {
                // reinit
                Reinit(params);
                
                // return success and go to next command
                return(std::make_pair(true,true));
                } // we're done!
            } // end if we're close now, and we were close before
        else
            {
            // stop moving
            if(moving)
                {
                // release move key
                params.keyboard->PressAndReleaseVK("VK_UP");
                // clear flag
                moving=false;
                } // end if moving

            // do final heading while standing still
            
            // proxy heading change
            status=head_point->Execute(params);
            
            if(!status.first)
                {
                // error!
                // reinit
                Reinit(params);
                // return error
                return(std::make_pair(false,true));
                } // end if turn error
            else if(status.second)
                {
                // clear turning flag
                turning=false;
                
                // set close flag, we will go straight in
                // from here
                close=true;
                
                // turn complete, start moving
                moving=true;
                // press move key
                params.keyboard->PressVK("VK_UP");
                } // end else turn completed
            } // end else we're close now, but weren't close before
        } // end if we are close
    else
        {
        if(turning)
            {
            // proxy heading change
            status=head_point->Execute(params);
            
            if(!status.first)
                {
                // error

                // reinit
                Reinit(params);

                // return error
                return(std::make_pair(false,true));
                } // end if turn error
            else if(status.second)
                {
                turning=false;
                } // end else turn completed
            } // end if turning
        else if(last_heading_check+3.0 < params.current_time->Seconds())
            {
            // store last heading check time
            last_heading_check=params.current_time->Seconds();
            
            // make arguments for headpoint, always give it 10 seconds
            std::stringstream ss;
            ss << x << " " << y << " " << 10.0 << std::endl;
            
            // extract head_point
            head_point->Extract(ss);
            
            // set turning flag
            turning=true;
            } // end else not turning and periodic heading check time
            
        if(!moving)
            {
            // start moving
            moving=true;

            // press move key
            params.keyboard->PressVK("VK_UP");
            } // end if not moving
        } // end else we aren't close

    // done, we need to execute again
    return(std::make_pair(true,false));
} // end MoveToPoint::Execute

bool MoveToActor::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name >> std::ws >> time_limit >> std::ws;
    
    if(Name.length() == 0)
        {
        ::Logger << "[MoveToActor::Extract] expected a non zero name argument.\n";
        return(false);
        }
    else if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToActor::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        proxy=0; // init to 0
        return(true);
        }
} // end MoveToActor::Extract

csl::CSLCommandAPI::EXECUTE_STATUS MoveToActor::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[MoveToActor::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        Actor MoveTo;
        if(!params.database->CopyActorByName(Name,MoveTo))
            {
            // did not find it
            ::Logger << "[MoveToActor::Execute] Could not find actor with name \"" << Name << "\"" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // init the proxy
            unsigned int x,y;
            unsigned short z;
            MapInfo::ZoneIndexType zone;
            
            // get zone relative current coordinates
            ::Zones.GetZoneFromGlobal
                (
                MoveTo.GetRegion(),
                unsigned int(MoveTo.GetMotion().GetXPos()),
                unsigned int(MoveTo.GetMotion().GetYPos()),
                unsigned short(MoveTo.GetMotion().GetZPos()),
                x,
                y,
                z,
                zone
                );
            
            // make a stream with command arguments for move to point
            std::stringstream ss;
            ss << x << " " << y << " " << time_limit << std::endl;
            csl::MoveToPoint* mtp=new csl::MoveToPoint;
            if(!mtp->Extract(ss))
                {
                delete mtp;
                // move to point extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, mtp will do the rest of my job for me
            proxy=mtp;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end MoveToActor::Execute

bool MoveToTarget::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> time_limit >> std::ws;
    
    if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToTarget::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        proxy=0; // init to 0
        return(true);
        }
} // end MoveToTarget::Extract

csl::CSLCommandAPI::EXECUTE_STATUS MoveToTarget::Execute(csl::EXECUTE_PARAMS& params)
{
    if(!proxy)
        {
        if(!params.targetted_actor->GetInfoId())
            {
            // did not find it
            ::Logger << "[MoveToTarget::Execute] no target!" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // make a stream with command arguments for move to actor
            std::stringstream ss;
            ss << params.targetted_actor->GetName() << " " << time_limit << std::endl;
            csl::MoveToActor* mta=new csl::MoveToActor;
            if(!mta->Extract(ss))
                {
                delete mta;
                // move to actor extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, mta will do the rest of my job for me
            proxy=mta;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end MoveToTarget::Execute

bool MoveToPointRelative::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> x_relative >> std::ws >> y_relative >> std::ws >> time_limit >> std::ws;
    
    if(x_relative<=0 || x_relative > 65535)
        {
        ::Logger << "[MoveToPointRelative::Extract] expected x to be (0,65535]" << std::endl;
        return(false);
        }
    else if(y_relative<=0 || y_relative > 65535)
        {
        ::Logger << "[MoveToPointRelative::Extract] expected y to be (0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToPointRelative::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
        
    // done
    return(true);
} // end MoveToPointRelative::Extract

csl::CSLCommandAPI::EXECUTE_STATUS MoveToPointRelative::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[MoveToPointRelative::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        // init the proxy
        unsigned int x,y;
        unsigned short z;
        MapInfo::ZoneIndexType zone;
        
        // get zone relative current coordinates
        ::Zones.GetZoneFromGlobal
            (
            params.followed_actor->GetRegion(),
            unsigned int(params.followed_actor->GetMotion().GetXPos()),
            unsigned int(params.followed_actor->GetMotion().GetYPos()),
            unsigned short(params.followed_actor->GetMotion().GetZPos()),
            x,
            y,
            z,
            zone
            );

        // add offsets
        x+=x_relative;
        y+=y_relative;
        
        // make a stream with command arguments for move to point
        std::stringstream ss;
        ss << x << " " << y << " " << time_limit << std::endl;
        csl::MoveToPoint* mtp=new csl::MoveToPoint;
        if(!mtp->Extract(ss))
            {
            delete mtp;
            // move to point extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, mtp will do the rest of my job for me
        proxy=mtp;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if no proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end MoveToPointRelative::Execute

bool MoveToActorRelative::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name 
               >> std::ws >> angle_radians 
               >> std::ws >> distance 
               >> std::ws >> time_limit
               >> std::ws;
               
    if(Name.length() == 0)
        {
        ::Logger << "[MoveToActorRelative::Extract] expected a non zero length name.\n";
        return(false);
        }
    else if(angle_radians<0.0f || angle_radians >= 360.0f)
        {
        ::Logger << "[MoveToActorRelative::Extract] expected angle to be [0,360)" << std::endl;
        return(false);
        }
    else if(distance < 0.0f || distance > 65535.0f)
        {
        ::Logger << "[MoveToActorRelative::Extract] expected distance to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToActorRelative::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    
    // convert to radians
    angle_radians *= 3.1415926535897932384626433832795f/180.0f;
    
    return(true);
} // end MoveToActorRelative::Extract

csl::CSLCommandAPI::EXECUTE_STATUS MoveToActorRelative::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[MoveToActorRelative::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        Actor MoveTo;
        if(!params.database->CopyActorByName(Name,MoveTo))
            {
            // did not find it
            ::Logger << "[MoveToActorRelative::Execute] Could not find actor with name \"" << Name << "\"" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // init the proxy
            unsigned int x,y;
            unsigned short z;
            MapInfo::ZoneIndexType zone;
            float rel_x;
            float rel_y;
            
            // get point relative to actor
            MoveTo.GetMotion().GetPointRelative(angle_radians,distance,rel_x,rel_y);
            
            // get zone relative offset coordinates
            ::Zones.GetZoneFromGlobal
                (
                MoveTo.GetRegion(),
                unsigned int(rel_x),
                unsigned int(rel_y),
                unsigned short(MoveTo.GetMotion().GetZPos()),
                x,
                y,
                z,
                zone
                );
            
            // we want to move to <x,y>
            // make a stream with command arguments for move to point
            std::stringstream ss;
            ss << x << " " << y << " " << time_limit << std::endl;
            csl::MoveToPoint* mtp=new csl::MoveToPoint;
            if(!mtp->Extract(ss))
                {
                delete mtp;
                // move to point extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, mtp will do the rest of my job for me
            proxy=mtp;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end MoveToActorRelative::Execute

bool MoveToTargetRelative::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> angle_degrees 
               >> std::ws >> distance 
               >> std::ws >> time_limit
               >> std::ws;
               
    if(angle_degrees<0.0f || angle_degrees >= 360.0f)
        {
        ::Logger << "[MoveToTargetRelative::Extract] expected angle to be [0,360)" << std::endl;
        return(false);
        }
    else if(distance < 0.0f || distance > 65535.0f)
        {
        ::Logger << "[MoveToTargetRelative::Extract] expected distance to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToTargetRelative::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    
    // don't convert to radians, we want degrees here
    
    return(true);
} // end Extract

csl::CSLCommandAPI::EXECUTE_STATUS MoveToTargetRelative::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[MoveToTargetRelative::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        if(!params.targetted_actor->GetInfoId())
            {
            // did not find it
            ::Logger << "[MoveToTargetRelative::Execute] no target!" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // init the proxy
            // make a stream with command arguments for move to actor relative
            std::stringstream ss;
            ss << params.targetted_actor->GetName() 
               << " " << angle_degrees 
               << " " << distance
               << " " << time_limit 
               << std::endl;
            csl::MoveToActorRelative* mtar=new csl::MoveToActorRelative;
            if(!mtar->Extract(ss))
                {
                delete mtar;
                // move to point extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, mtar will do the rest of my job for me
            proxy=mtar;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end Execute

bool HeadTo::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> heading_radians 
               >> std::ws >> time_limit
               >> std::ws;
    
    if(heading_radians<0.0f || heading_radians>=360.0f)
        {
        ::Logger << "[HeadTo::Extract] expected heading to be [0,360)" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[HeadTo::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    
    // convert to radians
    heading_radians *= 3.1415926535897932384626433832795f/180.0f;
    
    // init to initial value
    start_time=-1.0;
    
    return(true);
} // end Extract

float HeadTo::SetTurnDir(const float final_heading,const float current_heading)
{
    // final heading and current heading must be [0,2pi)
    const float pi=3.1415926535897932384626433832795f;
    const float two_pi=pi*2.0f;
    float delta_heading=final_heading-current_heading;
    
    if(fabs(delta_heading) > pi)
        {
        if(delta_heading < 0.0f)
            {
            delta_heading+=two_pi;
            }
        else
            {
            delta_heading-=two_pi;
            }
        } // end if north crossing check
    
    if(delta_heading < 0.0f)
        {
        turn_left=true;
        }
    else
        {
        turn_left=false;
        }

    ::Logger << "final heading: " << final_heading*180.0f/pi
             << " current heading: " << current_heading*180.0f/pi
             << " turn left: " << turn_left
             << " delta heading: " << (180.0f/pi)*fabs(delta_heading) << "\n";
    
    // return radians to turn
    return(fabs(delta_heading));
} // end SetTurnDir

csl::CSLCommandAPI::EXECUTE_STATUS HeadTo::Execute(csl::EXECUTE_PARAMS& params)
{
    // angular tolerance (5�) in radians
    const float tolerance=5.0f*3.1415926535897932384626433832795f/180.0f;
    
    // radians per second for turn rate 135�/second
    const float radians_sec=135.0f*3.1415926535897932384626433832795f/180.0f;
    
    // check start time
    if(start_time==-1.0)
        {
        // save it
        start_time=params.current_time->Seconds();
        }
    
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;
        start_time=-1.0;
        ::Logger << "[HeadTo::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }
    
    // check for proxy: this determines if we are just starting a turn,
    // or if we are waiting for an in-progress turn to complete
    if(!proxy)
        {
        // check time limit expired
        if(start_time + time_limit < params.current_time->Seconds())
            {
            // reinit
            start_time=-1.0;

            // time limit expired, bail out
            ::Logger << "[HeadTo::Execute] time limit expired!" << std::endl;
            return(std::make_pair(false,true));
            } // end if time limit expired
        
        // we currently are not turning, check to see which 
        // way to turn would be fastest
        const float delta_heading=SetTurnDir
            (
            heading_radians,
            params.followed_actor->GetMotion().GetHeading()
            );
        
        // check if we are within tolerance, if so
        // then we are done -- delta_heading
        // has already been fabs()'ed so we don't need
        // to check the sign
        if(delta_heading <= tolerance)
            {
            // we're already there!

            // set to initial value
            start_time=-1.0;
            
            // return success and go to next command
            return(std::make_pair(true,true));
            }
        
        // start turn
        if(turn_left)
            {
            // press left key and don't release it
            params.keyboard->PressVK("VK_LEFT");
            }
        else
            {
            // press right key and don't release it
            params.keyboard->PressVK("VK_RIGHT");
            }
        
        // see how long this should take
        const double time_to_go=delta_heading/radians_sec;
        
        //::Logger << "turning for " << time_to_go << " seconds\n";
        
        // make a stream with command arguments for delay
        std::stringstream ss;
        ss << time_to_go << std::endl; 
        // set up a delay proxy to wait that long
        csl::Delay* dly=new csl::Delay;
        if(!dly->Extract(ss))
            {
            delete dly;
            start_time=-1.0;
            // delay extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, dly will do the rest of my job for me
        proxy=dly;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        if(!status.first)
            {
            // release turn key

            // release left key (use press and release, just
            // release doesn't always work)
            params.keyboard->PressAndReleaseVK("VK_LEFT");
            // release right key (use press and release, just
            // release doesn't always work)
            params.keyboard->PressAndReleaseVK("VK_RIGHT");

            // there was a problem and we will not execute anymore
            // so we have to reinit
            // init to initial value
            start_time=-1.0;

            // donw with proxy, there was a problem
            delete proxy;
            proxy=0;
            } // end if problem
        else if(status.second)
            {
            // turn completed

            // release turn key

            // release left key (use press and release, just
            // release doesn't always work)
            params.keyboard->PressAndReleaseVK("VK_LEFT");
            // release right key (use press and release, just
            // release doesn't always work)
            params.keyboard->PressAndReleaseVK("VK_RIGHT");

            // proxy complete
            // init to initial value
            start_time=-1.0;
            delete proxy;
            proxy=0;
            } // end if proxy finished executing
        
        // done, return proxy fail status in case there was a problem
        // if there was, we have already deleted the proxy and don't have to worry
        // about that or about reinitializing
        return(status);
        } // end else proxy
} // end Execute

bool HeadPoint::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> x 
               >> std::ws >> y
               >> std::ws >> time_limit
               >> std::ws;
               
    if(x<0 || x > 65535)
        {
        ::Logger << "[HeadPoint::Extract] expected x to be [0,65535]" << std::endl;
        return(false);
        }
    else if(y < 0 || y > 65535)
        {
        ::Logger << "[HeadPoint::Extract] expected y to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[HeadPoint::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }

    return(true);
} // end HeadPoint::Extract

csl::CSLCommandAPI::EXECUTE_STATUS HeadPoint::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadPoint::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        // calculate the heading
        unsigned int zone_x,zone_y;
        unsigned short zone_z;
        MapInfo::ZoneIndexType zone;
        
        // get zone relative reference actor position
        ::Zones.GetZoneFromGlobal
            (
            params.followed_actor->GetRegion(),
            unsigned int(params.followed_actor->GetMotion().GetXPos()),
            unsigned int(params.followed_actor->GetMotion().GetYPos()),
            unsigned short(params.followed_actor->GetMotion().GetZPos()),
            zone_x,
            zone_y,
            zone_z,
            zone
            );
        
        // get deltas from current position
        float delta_x=(float)x-zone_x;
        float delta_y=(float)y-(float)zone_y;
        
        // get heading from current position
        // we use -delta_y here because +y is south
        // in DAoC
        float heading=atan2(delta_x,-delta_y);
        
        // make sure its positive
        if(heading < 0.0f)
            {
            heading += 2.0f*3.1415926535897932384626433832795f;
            }
        
        // make a stream with command arguments for head to
        std::stringstream ss;
        ss << heading*180.0f/3.1415926535897932384626433832795f << " " << time_limit << std::endl; 
        // set up a headto proxy
        csl::HeadTo* ht=new csl::HeadTo;
        if(!ht->Extract(ss))
            {
            delete ht;
            // head to extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, ht will do the rest of my job for me
        proxy=ht;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end HeadPoint::Execute

bool HeadActor::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name >> std::ws >> time_limit >> std::ws;
    
    if(Name.length() == 0)
        {
        ::Logger << "[HeadActor::Extract] expected a non zero name argument.\n";
        return(false);
        }
    else if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[HeadActor::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        proxy=0; // init to 0
        return(true);
        }
} // end HeadActor::Extract

csl::CSLCommandAPI::EXECUTE_STATUS HeadActor::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadActor::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        Actor HeadToActor;
        if(!params.database->CopyActorByName(Name,HeadToActor))
            {
            ::Logger << "[HeadActor::Execute] Could not find actor with name \"" << Name << "\"" << std::endl;
            return(std::make_pair(false,true));
            } // end if failed to find actor by name
            
        // calculate the heading
        unsigned int zone_x,zone_y;
        unsigned short zone_z;
        MapInfo::ZoneIndexType zone;
        
        // get zone relative reference actor position
        ::Zones.GetZoneFromGlobal
            (
            HeadToActor.GetRegion(),
            unsigned int(HeadToActor.GetMotion().GetXPos()),
            unsigned int(HeadToActor.GetMotion().GetYPos()),
            unsigned short(HeadToActor.GetMotion().GetZPos()),
            zone_x,
            zone_y,
            zone_z,
            zone
            );
        
        // make a stream with command arguments for head point
        std::stringstream ss;
        ss << zone_x << " " << zone_y << " " << time_limit << std::endl; 
        // set up a head point proxy
        csl::HeadPoint* hp=new csl::HeadPoint;
        if(!hp->Extract(ss))
            {
            delete hp;
            // head to extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, hp will do the rest of my job for me
        proxy=hp;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end HeadActor::Execute

bool HeadTarget::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> time_limit >> std::ws;
    
    if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[HeadTarget::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        proxy=0; // init to 0
        return(true);
        }
} // end HeadTarget::Extract

csl::CSLCommandAPI::EXECUTE_STATUS HeadTarget::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadTarget::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    // make sure we have a target
    if(!params.targetted_actor->GetInfoId())
        {
        // no targetted actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadTarget::Execute] no targetted actor!" << std::endl;
        return(std::make_pair(false,true));
        }
        
    if(!proxy)
        {
        // make a stream with command arguments for head actor
        std::stringstream ss;
        ss << params.targetted_actor->GetName() << " " << time_limit << std::endl; 
        // set up a head actor proxy
        csl::HeadActor* ha=new csl::HeadActor;
        if(!ha->Extract(ss))
            {
            delete ha;
            // head to extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, ha will do the rest of my job for me
        proxy=ha;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end HeadTarget::Execute

bool HeadPointRelative::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> x 
               >> std::ws >> y
               >> std::ws >> time_limit
               >> std::ws;
               
    if(x<0 || x > 65535)
        {
        ::Logger << "[HeadPointRelative::Extract] expected x to be [0,65535]" << std::endl;
        return(false);
        }
    else if(y < 0 || y > 65535)
        {
        ::Logger << "[HeadPointRelative::Extract] expected y to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[HeadPointRelative::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }

    return(true);
} // end HeadPointRelative::Extract

csl::CSLCommandAPI::EXECUTE_STATUS HeadPointRelative::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadPointRelative::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        // get heading from current position
        float heading=atan2((float)x,(float)-y);
        
        // make a stream with command arguments for head to
        std::stringstream ss;
        ss << heading*180.0f/3.1415926535897932384626433832795f << " " << time_limit << std::endl; 
        // set up a headto proxy
        csl::HeadTo* ht=new csl::HeadTo;
        if(!ht->Extract(ss))
            {
            delete ht;
            // head to extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, ht will do the rest of my job for me
        proxy=ht;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end HeadPointRelative::Execute

bool HeadActorRelative::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name 
               >> std::ws >> angle_radians 
               >> std::ws >> distance 
               >> std::ws >> time_limit
               >> std::ws;
               
    if(Name.length() == 0)
        {
        ::Logger << "[HeadActorRelative::Extract] expected a non zero length name.\n";
        return(false);
        }
    else if(angle_radians<0.0f || angle_radians >= 360.0f)
        {
        ::Logger << "[HeadActorRelative::Extract] expected angle to be [0,360)" << std::endl;
        return(false);
        }
    else if(distance < 0.0f || distance > 65535.0f)
        {
        ::Logger << "[HeadActorRelative::Extract] expected distance to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[MoveToActorRelative::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    
    // convert to radians
    angle_radians *= 3.1415926535897932384626433832795f/180.0f;
    
    return(true);
} // end HeadActorRelative::Extract

csl::CSLCommandAPI::EXECUTE_STATUS HeadActorRelative::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadActorRelative::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    if(!proxy)
        {
        Actor To;
        if(!params.database->CopyActorByName(Name,To))
            {
            // did not find it
            ::Logger << "[HeadActorRelative::Execute] Could not find actor with name \"" << Name << "\"" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // init the proxy
            unsigned int x,y;
            unsigned short z;
            MapInfo::ZoneIndexType zone;
            float rel_x;
            float rel_y;
            
            // get point relative to actor
            To.GetMotion().GetPointRelative(angle_radians,distance,rel_x,rel_y);
            
            // get zone relative offset coordinates
            ::Zones.GetZoneFromGlobal
                (
                To.GetRegion(),
                unsigned int(rel_x),
                unsigned int(rel_y),
                unsigned short(To.GetMotion().GetZPos()),
                x,
                y,
                z,
                zone
                );
            
            // we want to head to <x,y>
            // make a stream with command arguments for head to point
            std::stringstream ss;
            ss << x << " " << y << " " << time_limit << std::endl;
            csl::HeadPoint* hp=new csl::HeadPoint;
            if(!hp->Extract(ss))
                {
                delete hp;
                // move to point extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, hp will do the rest of my job for me
            proxy=hp;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end HeadActorRelative::Execute

bool HeadTargetRelative::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> angle_degrees
               >> std::ws >> distance 
               >> std::ws >> time_limit
               >> std::ws;
               
    if(angle_degrees<0.0f || angle_degrees >= 360.0f)
        {
        ::Logger << "[HeadTargetRelative::Extract] expected angle to be [0,360)" << std::endl;
        return(false);
        }
    else if(distance < 0.0f || distance > 65535.0f)
        {
        ::Logger << "[HeadTargetRelative::Extract] expected distance to be [0,65535]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0f || time_limit > 3600.0f)
        {
        ::Logger << "[HeadTargetRelative::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }

    // don't convert to radians
    
    return(true);
} // end HeadTargetRelative::Extract

csl::CSLCommandAPI::EXECUTE_STATUS HeadTargetRelative::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we have someone to follow (reference)
    if(!params.followed_actor->GetInfoId())
        {
        // no followed actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadTargetRelative::Execute] no followed (reference) actor!" << std::endl;
        return(std::make_pair(false,true));
        }

    // make sure we have a target
    if(!params.targetted_actor->GetInfoId())
        {
        // no targetted actor!
        
        // reinit
        delete proxy;
        proxy=0;

        ::Logger << "[HeadTargetRelative::Execute] no targetted actor!" << std::endl;
        return(std::make_pair(false,true));
        }
        
    if(!proxy)
        {
        // make a stream with command arguments for head actor
        std::stringstream ss;
        ss << params.targetted_actor->GetName() << " " 
           << angle_degrees << " "
           << distance << " "
           << time_limit << std::endl; 
        // set up a head actor proxy
        csl::HeadActorRelative* har=new csl::HeadActorRelative;
        if(!har->Extract(ss))
            {
            delete har;
            // head to relative extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, har will do the rest of my job for me
        proxy=har;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end HeadTargetRelative::Execute

bool KeyboardPress::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws >> ch;

    if(ch=='\0')
        {
        // woops
        ::Logger << "[KeyboardPress::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        return(true);
        }
} // end KeyboardPress::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardPress::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->PressAndRelease(ch),true));
} // end KeyboardPress::Execute

bool KeyboardHold::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws >> ch;
    
    if(ch=='\0')
        {
        // woops
        ::Logger << "[KeyboardHold::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        return(true);
        }
} // end KeyboardHold::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardHold::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->Press(ch),true));
} // end KeyboardHold::Execute

bool KeyboardRelease::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws >> ch;
    
    if(ch=='\0')
        {
        // woops
        ::Logger << "[KeyboardRelease::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        return(true);
        }
} // end KeyboardRelease::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardRelease::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->Release(ch),true));
} // end KeyboardRelease::Execute

bool KeyboardString::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws;
    
    // store output
    std::getline(arg_stream,Output);
    
    if(Output.length() == 0)
        {
        // woops
        ::Logger << "[KeyboardString::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        // done
        return(true);
        }
} // end KeyboardString::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardString::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->String(Output),true));
} // end KeyboardString::Execute

bool KeyboardVKey::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws;
    
    // store output
    std::getline(arg_stream,VKey);
    
    if(VKey.length() == 0)
        {
        // woops
        ::Logger << "[KeyboardVKey::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        // done
        return(true);
        }
} // end KeyboardVKey::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardVKey::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->PressAndReleaseVK(VKey),true));
} // end KeyboardVKey::Execute

bool KeyboardVKeyHold::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws;
    
    // store output
    std::getline(arg_stream,VKey);
    
    if(VKey.length() == 0)
        {
        // woops
        ::Logger << "[KeyboardVKeyHold::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        // done
        return(true);
        }
} // end KeyboardVKeyHold::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardVKeyHold::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->PressVK(VKey),true));
} // end KeyboardVKeyHold::Execute

bool KeyboardVKeyRelease::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws;
    
    // store output
    std::getline(arg_stream,VKey);
    
    if(VKey.length() == 0)
        {
        // woops
        ::Logger << "[KeyboardVKeyRelease::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        // done
        return(true);
        }
} // end KeyboardVKeyRelease::Extract

csl::CSLCommandAPI::EXECUTE_STATUS KeyboardVKeyRelease::Execute(csl::EXECUTE_PARAMS& params)
{
    return(std::make_pair(params.keyboard->ReleaseVK(VKey),true));
} // end KeyboardVKeyRelease::Execute

bool CallScript::Extract(std::istream& arg_stream)
{
    bInvoked=false;
    
    arg_stream >> ToCall >> std::ws;
    
    if(ToCall.length() == 0)
        {
        ::Logger << "[CallScript::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        return(true);
        }
} // end CallScript::Extract

csl::CSLCommandAPI::EXECUTE_STATUS CallScript::Execute(csl::EXECUTE_PARAMS& params)
{
    if(!bInvoked)
        {
        // invoke now, stay on this command
        bInvoked=true;
        return(std::make_pair(params.script_host->CallScript(ToCall,params),false));
        }
    else
        {
        // reset, we may be called again and in this
        // case, we must act like we were never executed
        bInvoked=false;
        
        // move on to next command
        return(std::make_pair(true,true));
        }
} // end CallScript::Execute

bool ExecuteScript::Extract(std::istream& arg_stream)
{
    bInvoked=false;
    
    arg_stream >> ToExecute >> std::ws;
    
    if(ToExecute.length() == 0)
        {
        ::Logger << "[ExecuteScript::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        return(true);
        }
    return(true);
} // end ExecuteScript::Extract

csl::CSLCommandAPI::EXECUTE_STATUS ExecuteScript::Execute(csl::EXECUTE_PARAMS& params)
{
    if(!bInvoked)
        {
        // invoke now, stay on this command
        bInvoked=true;
        return(std::make_pair(params.script_host->ExecuteScript(ToExecute),false));
        }
    else
        {
        // reset, we may be called again and in this
        // case, we must act like we were never executed
        bInvoked=false;
        
        // move on to next command
        return(std::make_pair(true,true));
        }
} // end ExecuteScript::Execute

bool RestartScript::Extract(std::istream& arg_stream)
{
    // no parameters
    return(true);
} // end RestartScript::Extract

csl::CSLCommandAPI::EXECUTE_STATUS RestartScript::Execute(csl::EXECUTE_PARAMS& params)
{
    // we use "stay on this command" to 
    // cause the CSLSubroutine to not advance
    // to next: we have already (via the call
    // to the script host) set the command_iterator
    return(std::make_pair(params.script_host->RestartScript(),false));
} // end RestartScript::Execute

bool DebugString::Extract(std::istream& arg_stream)
{
    // skip leading whitespace
    arg_stream >> std::ws;
    
    // store output
    std::getline(arg_stream,Output);
    
    if(Output.length() == 0)
        {
        // woops
        ::Logger << "[DebugString::Extract] expected a non zero length argument.\n";
        return(false);
        }
    else
        {
        // done
        return(true);
        }
} // end Extract

csl::CSLCommandAPI::EXECUTE_STATUS DebugString::Execute(csl::EXECUTE_PARAMS& params)
{
    // output string
    ::Logger << Output << std::endl;
    
    // move on to next command
    return(std::make_pair(true,true));
} // end OutputDebugString

bool Delay::Extract(std::istream& arg_stream)
{
    // get argument
    arg_stream >> param >> std::ws;
    
    if(param==0.0)
        {
        // woops
        ::Logger << "[Delay::Extract] expected a non zero argument.\n";
        return(false);
        }
    else
        {
        //::Logger << "[Delay::Extract] I will delay " << param << " seconds" << std::endl;
        // init
        start_time=-1.0;
        return(true);
        }
} // end Delay::Extract

csl::CSLCommandAPI::EXECUTE_STATUS Delay::Execute(csl::EXECUTE_PARAMS& params)
{
    if(start_time==-1.0)
        {
        // this is our first execution, set start time
        start_time=params.current_time->Seconds();
        
        // success, but stay on this command
        return(std::make_pair(true,false));
        }

    if(params.current_time->Seconds() - start_time < param)
        {
        // we are not done yet:
        // success, but stay on this command
        return(std::make_pair(true,false));
        }
    
    // reset start time, we may be called again and in this
    // case, we must act like we were never executed
    start_time=-1.0;
    
    //::Logger << "[Delay::Execute] I have delayed " << param << " seconds" << std::endl;
    // success, move on to next command
    return(std::make_pair(true,true));
} // end Delay::Execute

bool InterceptActorOffset::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name 
               >> std::ws >> reference_velocity 
               >> std::ws >> heading_offset
               >> std::ws >> distance
               >> std::ws >> time_limit 
               >> std::ws;
    
    if(Name.length() == 0)
        {
        ::Logger << "[InterceptActorOffset::Extract] expected a non zero name argument.\n";
        return(false);
        }
    else if(reference_velocity<=0.0 || reference_velocity > 3600.0f)
        {
        ::Logger << "[InterceptActorOffset::Extract] expected reference velicity to be (0,3600]" << std::endl;
        return(false);
        }
    else if(heading_offset<0.0 || heading_offset >= 360.0f)
        {
        ::Logger << "[InterceptActorOffset::Extract] expected heading offset to be [0,360)" << std::endl;
        return(false);
        }
    else if(distance<0.0 || distance > 10000.0f)
        {
        ::Logger << "[InterceptActorOffset::Extract] expected distance to be [0,10000)" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[InterceptActorOffset::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        start_time=-1.0;
        last_target_check_time=-10.0;
        // go ahead and init these to something (as long
        // as its a legal float, it doesn't matter)
        intercept_x=0.0f;
        intercept_y=0.0f;
        return(true);
        }
} // end Extract

void InterceptActorOffset::Reinit(csl::EXECUTE_PARAMS& params)
{
    
    start_time=-1.0;
    last_target_check_time=-10.0;

    // go ahead and clear this out too, if it exists
    // then we are probably moving or turning so we
    // need to stop that too
    if(move_point)
        {
        delete move_point;
        move_point=0;
        
        // stop moving
        params.keyboard->PressAndReleaseVK("VK_UP");

        // release any turning keys
        params.keyboard->PressAndReleaseVK("VK_LEFT");
        params.keyboard->PressAndReleaseVK("VK_RIGHT");
        }
    
    // init to a legal float
    intercept_x=0.0f;
    intercept_y=0.0f;
    
    // done
    return;
} // end Reinit

bool InterceptActorOffset::DoIntercept(csl::EXECUTE_PARAMS& params,const Actor& Target)
{
    // turn heading and distance offset into 
    // x,y offsets (we have to do this every
    // time since Target may be turning
    float x_offset;
    float y_offset;
    Target.GetMotion().GetPointRelative(heading_offset,distance,x_offset,y_offset);
    
    // create intercept data
    INTERCEPT_DATA<float> id;
    id.params.t0x=x_offset;
    id.params.t0y=-y_offset;
    id.params.a0x=params.followed_actor->GetMotion().GetXPos();
    id.params.a0y=-(params.followed_actor->GetMotion().GetYPos());
    id.params.s=float(reference_velocity);
    id.params.tvx=Target.GetMotion().GetSpeed() * sin(Target.GetMotion().GetHeading());
    id.params.tvy=-(Target.GetMotion().GetSpeed() * cos(Target.GetMotion().GetHeading()));
    
    if(!FindIntercept(id))
        {
        // no intercept
        ::Logger << "[InterceptActorOffset::DoIntercept] no intercept possible for \"" 
                 << Target.GetName() << "\"" << std::endl;
        
        return(false);
        } // end if no intercept
    else
        {
        // this is wierd: the heading stored in Actor is opposite
        // of what we would expect. A heading of 0 causes Y to 
        // decrease. For this reason, we negate Y here. The heading is correct though
        id.YDotToIntercept=-id.YDotToIntercept;
        id.YIntercept=-id.YIntercept;
        
        // debug 
        ::Logger
            << "t0x=" << id.params.t0x << "\n"
            << "t0y=" << id.params.t0y << "\n"
            << "a0x=" << id.params.a0x << "\n"
            << "a0y=" << id.params.a0y << "\n"
            << "tvx=" << id.params.tvx << "\n"
            << "tvy=" << id.params.tvy << "\n"
            << "s=" << id.params.s << "\n"
            << "intercept heading=" << id.HeadingRadiansToIntercept*180.0f/3.1415926535897932384626433832795f << "\n"
            << "time to intercept=" << id.TimeToIntercept << "\n"
            << "V<" << id.XDotToIntercept << "," << id.YDotToIntercept << ">\n"
            << "P<" << id.XIntercept << "," << id.YIntercept << ">\n";
        // end debug
        // check to see if we need to recreate the move_point command proxy
        if(not_near(id.XIntercept,intercept_x,50.0f) || not_near(id.YIntercept,intercept_y,50.0f) || !move_point)
            {
            // intercept has changed or this is the first 
            // time we have computed an intercept, kick off
            // move_to with new parameters
            intercept_x=id.XIntercept; // these are in global coordinates
            intercept_y=id.YIntercept; // move-to needs local coordinates
            
            unsigned int move_to_x;
            unsigned int move_to_y;
        
            unsigned short curr_z;
            MapInfo::ZoneIndexType zone;
            
            // get zone-relative (local) coordinates
            ::Zones.GetZoneFromGlobal
                (
                Target.GetRegion(),
                unsigned int(intercept_x),
                unsigned int(intercept_y),
                unsigned short(Target.GetMotion().GetZPos()), // this doesnt really matter here
                move_to_x,
                move_to_y,
                curr_z, // don't care
                zone // don't care
                );
                
            // recreate move_to -- yes this is a waste, deleting and recreating
            // all the time :-/
            delete move_point; // yes, we can delete a null pointer safely...
            move_point=new csl::MoveToPoint;
            
            std::stringstream ss;
            ss << move_to_x << " " << move_to_y << " " << time_limit << std::endl;
            
            // return extract status
            return(move_point->Extract(ss));
            } // end if intercept changed or we need to create move_to
        
        // done, previous calculations are still valid
        // so we keep using them
        return(true);
        }
} // end DoIntercept

csl::CSLCommandAPI::EXECUTE_STATUS InterceptActorOffset::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we are following someone
    if(!params.followed_actor->GetInfoId())
        {
        ::Logger << "[InterceptActorOffset::Execute] intercept \"" << Name << "\" failed to complete in "
                 << time_limit << " seconds because there is no followed (reference) actor" << std::endl;
        
        // reinit
        Reinit(params);
        
        // return error
        return(std::make_pair(false,true));
        } // end if no followed actor
    
    // see if first time through
    if(start_time==-1.0)
        {
        // save start time
        start_time=params.current_time->Seconds();
        
        // debug
        ::Logger << "iao: reference_velocity=" << reference_velocity
                 << " heading_offset=" << heading_offset
                 << " distance=" << distance
                 << " time_limit=" << time_limit << std::endl;
        // end debug
        }
    
    // see if we are over our time limit
    if(start_time+time_limit < params.current_time->Seconds())
        {
        // we exceeded out time limit
        ::Logger << "[InterceptActorOffset::Execute] intercept \"" << Name << "\" failed to complete in "
                 << time_limit << " seconds" << std::endl;
        
        // reinit
        Reinit(params);
        
        // return error
        return(std::make_pair(false,true));
        } // end if time limit exceeded
    
    // see if we need to recheck for target existance
    // and a redo of the intercept algorithm
    // since last_target_check_time is initialized negative
    // we will always execute this the first time through.
    
    // the addend for last check time dictates how often
    // we can detect target velocity changes -- 
    // we will do this 3 or 4 times a second
    if(last_target_check_time+0.25 < params.current_time->Seconds())
        {
        // check for target existance
        Actor Target;
        if(!params.database->CopyActorByName(Name,Target))
            {
            // we lost the target
            ::Logger << "[InterceptActorOffset::Execute] intercept \"" << Name << "\" failed to complete in "
                    << time_limit << " seconds, we lost the target!" << std::endl;
            
            // reinit
            Reinit(params);
            
            // return error
            return(std::make_pair(false,true));
            } // end if we lost the target
        else if(!DoIntercept(params,Target))
            {
            // we are unable to intercept the target
            // bail out
            Reinit(params);
            
            // return error
            return(std::make_pair(false,true));
            } // end else if target not interceptable
        } // end if we need to check for target existance
    
    // if we get here, the move_point proxy must exist!
    // proxy the execute command
    csl::CSLCommandAPI::EXECUTE_STATUS status=move_point->Execute(params);
    
    // see if we need to delete the proxy
    if(!status.first || status.second)
        {
        // we need to reinit now, we
        // are done, success or not
        Reinit(params);
        } // end if proxy done, or proxy failed
    
    // return proxy's status
    return(status);
} // end Execute

bool InterceptActor::Extract(std::istream& arg_stream)
{
    arg_stream >> std::ws >> Name 
               >> std::ws >> reference_velocity 
               >> std::ws >> time_limit 
               >> std::ws;
    
    if(Name.length() == 0)
        {
        ::Logger << "[InterceptActor::Extract] expected a non zero name argument.\n";
        return(false);
        }
    else if(reference_velocity<=0.0 || reference_velocity > 3600.0f)
        {
        ::Logger << "[InterceptActor::Extract] expected reference velicity to be (0,3600]" << std::endl;
        return(false);
        }
    else if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[InterceptActor::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        return(true);
        }
} // end Extract

csl::CSLCommandAPI::EXECUTE_STATUS InterceptActor::Execute(csl::EXECUTE_PARAMS& params)
{
    // make sure we are following someone
    if(!params.followed_actor->GetInfoId())
        {
        ::Logger << "[InterceptActor::Execute] intercept \"" << Name << "\" failed to complete in "
                 << time_limit << " seconds because there is no followed (reference) actor" << std::endl;
        
        // return error
        return(std::make_pair(false,true));
        } // end if no followed actor
    
    if(!proxy)
        {
        // make a stream with command arguments for intercept actor offset
        std::stringstream ss;
        ss << params.targetted_actor->GetName() << " " 
           << reference_velocity << " "
           << "0 " // heading offset 0
           << "0 " // distance offset 0 
           << time_limit << std::endl;
        csl::InterceptActorOffset* iao=new csl::InterceptActorOffset;
        if(!iao->Extract(ss))
            {
            delete iao;
            // intercept actor offset extract failed
            return(std::make_pair(false,true));
            }
        
        // save as proxy, iao will do the rest of my job for me
        proxy=iao;
        
        // done, stay on this command
        return(std::make_pair(true,false));
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end Execute

bool InterceptTargetOffset::Extract(std::istream& arg_stream)
{
    arg_stream >> reference_velocity >> std::ws
               >> std::ws >> heading_offset
               >> std::ws >> distance
               >> std::ws >> time_limit;
    
    if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[InterceptTargetOffset::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else if(heading_offset<0.0 || heading_offset >= 360.0f)
        {
        ::Logger << "[InterceptTargetOffset::Extract] expected heading offset to be [0,360)" << std::endl;
        return(false);
        }
    else if(distance<0.0 || distance > 10000.0f)
        {
        ::Logger << "[InterceptTargetOffset::Extract] expected distance to be [0,10000)" << std::endl;
        return(false);
        }
    else if(reference_velocity<=0.0 || reference_velocity > 3600.0f)
        {
        ::Logger << "[InterceptTargetOffset::Extract] expected reference velicity to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        proxy=0; // init to 0
        return(true);
        }
} // end Extract

csl::CSLCommandAPI::EXECUTE_STATUS InterceptTargetOffset::Execute(csl::EXECUTE_PARAMS& params)
{
    if(!proxy)
        {
        if(!params.targetted_actor->GetInfoId())
            {
            // did not find it
            ::Logger << "[InterceptTargetOffset::Execute] no target!" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // make a stream with command arguments for intercept actor offset
            std::stringstream ss;
            ss << params.targetted_actor->GetName() << " " 
               << reference_velocity << " " 
               << heading_offset << " "
               << distance << " "
               << time_limit << std::endl;
            csl::InterceptActorOffset* iao=new csl::InterceptActorOffset;
            if(!iao->Extract(ss))
                {
                delete iao;
                // intercept actor extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, iao will do the rest of my job for me
            proxy=iao;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end Execute

bool InterceptTarget::Extract(std::istream& arg_stream)
{
    arg_stream >> reference_velocity >> std::ws
               >> std::ws >> time_limit;
    
    if(time_limit<=0.0 || time_limit > 3600.0f)
        {
        ::Logger << "[InterceptTarget::Extract] expected time limit to be (0,3600]" << std::endl;
        return(false);
        }
    else if(reference_velocity<=0.0 || reference_velocity > 3600.0f)
        {
        ::Logger << "[InterceptTarget::Extract] expected reference velicity to be (0,3600]" << std::endl;
        return(false);
        }
    else
        {
        proxy=0; // init to 0
        return(true);
        }
} // end Extract

csl::CSLCommandAPI::EXECUTE_STATUS InterceptTarget::Execute(csl::EXECUTE_PARAMS& params)
{
    if(!proxy)
        {
        if(!params.targetted_actor->GetInfoId())
            {
            // did not find it
            ::Logger << "[InterceptTarget::Execute] no target!" << std::endl;
            return(std::make_pair(false,true));
            }
        else
            {
            // make a stream with command arguments for intercept actor
            std::stringstream ss;
            ss << params.targetted_actor->GetName() << " " << reference_velocity << " " << time_limit << std::endl;
            csl::InterceptActor* ia=new csl::InterceptActor;
            if(!ia->Extract(ss))
                {
                delete ia;
                // intercept actor extract failed
                return(std::make_pair(false,true));
                }
            
            // save as proxy, ia will do the rest of my job for me
            proxy=ia;
            
            // done, stay on this command
            return(std::make_pair(true,false));
            }
        } // end if not proxy
    else
        {
        // proxy the execute command
        csl::CSLCommandAPI::EXECUTE_STATUS status=proxy->Execute(params);
        
        // see if we need to delete the proxy
        if(!status.first || status.second)
            {
            delete proxy;
            proxy=0;
            }
        
        // return proxy's status
        return(status);
        } // end else proxy
} // end Execute

csl::CSLCommandAPI::EXECUTE_STATUS CSLSubroutine::Execute(csl::EXECUTE_PARAMS& params)
{
    if(command_iterator != Commands.end())
        {
        //::Logger << __FUNCTION__ << " executing command from script " << GetName() << std::endl;
        // execute current command
        csl::CSLCommandAPI::EXECUTE_STATUS status=(*command_iterator)->Execute(params);
        
        // check command done status
        if(status.second)
            {
            // this command done, advance to next command
            //::Logger << __FUNCTION__ << " moving to next command from script " << GetName() << std::endl;
            ++command_iterator;
            }
        
        // in this case, "stay on this command" means
        // this subroutine is still executing
        return(std::make_pair(status.first,false));
        }
    else
        {
        // in this case, "move to next command" means
        // this subroutine is complete
        //::Logger << __FUNCTION__ << " subroutine complete from script " << GetName() << std::endl;
        return(std::make_pair(true,true));
        }
} // end CSLSubroutine::Execute

} // end namespace csl