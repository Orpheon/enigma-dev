/********************************************************************************\
**                                                                              **
**  Copyright (C) 2008 Josh Ventura                                             **
**  Copyright (C) 2012 Alasdair Morrison                                        **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/

#include <map>
#include <math.h>
#include <string>
//#include "reflexive_types.h"
//#include "EGMstd.h"
#include "object.h"

#include "instance_system.h"
#include "instance.h"

namespace enigma
{
  int destroycalls = 0, createcalls = 0;
}

typedef std::pair<int,enigma::inst_iter*> inode_pair;
void instance_deactivate_all(bool notme) {
    for (enigma::iterator it = enigma::instance_list_first(); it; ++it) {
        if (notme && (*it)->id == enigma::instance_event_iterator->inst->id) continue;

        ((enigma::object_basic*)*it)->deactivate();
        enigma::instance_deactivated_list.insert(inode_pair((*it)->id,it.it));
    }
}


void instance_activate_all() {

    std::map<int,enigma::inst_iter*>::iterator iter = enigma::instance_deactivated_list.begin();
    while (iter != enigma::instance_deactivated_list.end()) {
        ((enigma::object_basic*)(iter->second->inst))->activate();
        enigma::instance_deactivated_list.erase(iter++);
    }
}

void instance_deactivate_object(int obj) {
    for (enigma::iterator it = enigma::fetch_inst_iter_by_int(obj); it; ++it) {
        ((enigma::object_basic*)*it)->deactivate();
        enigma::instance_deactivated_list.insert(inode_pair((*it)->id,it.it));
    }
}

void instance_activate_object(int obj) {
    std::map<int,enigma::inst_iter*>::iterator iter = enigma::instance_deactivated_list.begin();
    while (iter != enigma::instance_deactivated_list.end()) {
        enigma::object_basic* const inst = ((enigma::object_basic*)(iter->second->inst));
        if (obj==all ||(obj<100000 && inst->object_index==obj)|| (obj>100000 && inst->id == obj)) {
            inst->activate();
            enigma::instance_deactivated_list.erase(iter++);
        }
        else {
            iter++;
        }
    }
}

void instance_destroy(int id, bool dest_ev)
{
  enigma::object_basic* who = enigma::fetch_instance_by_id(id);
  if (who and enigma::cleanups.find(who) == enigma::cleanups.end()) {
    if (dest_ev)
        who->myevent_destroy();
    if (enigma::cleanups.find(who) == enigma::cleanups.end())
        who->unlink();
  }
}
#include <stdio.h>
void instance_destroy()
{
  enigma::object_basic* const a = enigma::instance_event_iterator->inst;
  if (enigma::cleanups.find(a) == enigma::cleanups.end()) {
    enigma::instance_event_iterator->inst->myevent_destroy();
    if (enigma::cleanups.find(a) == enigma::cleanups.end())
        enigma::instance_event_iterator->inst->unlink();
    if (enigma::cleanups.find(a) == enigma::cleanups.end())
    printf("FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK! FUCK!\nFFFFFFFFFFFFFFFFFFFFFUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUCK!\nFUCK! %p ISN'T ON THE GOD DAMNED MOTHER FUCKING STACK!",a);
    if (a != (enigma::object_basic*)enigma::instance_event_iterator->inst)
    printf("FUCKING DAMN IT! THE ITERATOR CHANGED FROM POINTING TO %p TO POINTING TO %p\n",a,(enigma::object_basic*)enigma::instance_event_iterator->inst);
  }
}

bool instance_exists(int obj)
{
  return enigma::fetch_instance_by_int(obj) != NULL;
}

int instance_find(int obj, int num)
{
  int nth=0;
  for (enigma::iterator it = enigma::fetch_inst_iter_by_int(obj); it; ++it)
  {
    nth++;
    if (nth>num)
    return (int) it->id;
  }
  return noone;
}

int instance_number(int obj)
{
  return enigma::objects[obj].count;
}

//TODO: Move these to an instance_planar
#include "planar_object.h"

//TODO: replace all these fucking enigma::instance_iterators with enigma::institer_t i or something

int instance_nearest(int x,int y,int obj,bool notme)
{
  double dist_lowest=-1;
  int retid=-4;
  double xl,yl;

  for (enigma::iterator it = enigma::fetch_inst_iter_by_int(obj); it; ++it)
  {
    if (notme && (*it)->id == enigma::instance_event_iterator->inst->id) continue;
    xl = ((enigma::object_planar*)*it)->x - x;
    yl = ((enigma::object_planar*)*it)->y - y;
    const double dstclc = hypot(xl,yl);
    if (dstclc < dist_lowest or dist_lowest == -1)
    {
      dist_lowest = dstclc;
      retid = it->id;
    }
  }

  return retid;
}

int instance_furthest(int x,int y,int obj,bool notme)
{
  double dist_highest = -1;
  int retid = noone;
  double xl,yl;
  double dstclc;

  for (enigma::iterator it = enigma::fetch_inst_iter_by_int(obj); it; ++it)
  {
    if (notme && (*it)->id == enigma::instance_event_iterator->inst->id) continue;
    xl=((enigma::object_planar*)*it)->x - x;
    yl=((enigma::object_planar*)*it)->y - y;
    dstclc = hypot(xl,yl);
    if (dstclc > dist_highest)
    {
      dist_highest = dstclc;
      retid = it->id;
    }
  }

  return retid;
}
