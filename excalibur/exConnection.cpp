/*
 * Copyright 2002 the Excalibur contributors (http://excalibar.sourceforge.net/)
 *
 * Portions of this software are based on the work of Slicer/Hackersquest.
 * Those portions, Copyright 2001 Slicer/Hackersquest <slicer@hackersquest.org)
 * 
 * This file is part of Excalibur.
 *
 * Excalibur is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Excalibur is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/netfilter_ipv4.h>
#include <arpa/inet.h>
#include <qstring.h>
#include <qfile.h>
#include <qsplitter.h>
#include <qframe.h>
#include <qaction.h>
#include <qlabel.h>
#include <qprocess.h>
#include <math.h>
#include <qmessagebox.h>
#include "exConnection.h"
#include "exItem.h"

exConnection::exConnection(exNet * s, bool dolink)
{
    QString fname;
    QString from;
    QString to;
    struct in_addr addr;

    setup();
    sniff = s;

    addr.s_addr = sniff->n_client_addr;
    from = inet_ntoa(addr);
    addr.s_addr = sniff->n_server_addr;
    to = inet_ntoa(addr);
    fname = "Capture-" + from + "-" + to + "-" + QDateTime::currentDateTime().toString("yyyy.MM.dd-hh:mm:ss");

    file->setName(fname);
    file->open(IO_WriteOnly);
    ds = new QDataStream(file);

    if (dolink)
      link = new exLink();
}

exConnection::exConnection(QString * f)
{
    setup();
    replayfile = f;
}

void exConnection::setup()
{
    replayfile = NULL;
    selectedid = 0;
    replayfile = NULL;
    sniff = NULL;

    playerzone = 0;

    file = new QFile;

    ex = new FormExcalibur;
    ex->Map->setConnection(this);
    connect(ex->ListViewMobs, SIGNAL(selectionChanged(QListViewItem *)), this, SLOT(listSelectionChanged(QListViewItem *)));

    alive = true;
    mobs.setAutoDelete(true);
    players.setAutoDelete(true);
    mobinfo.setAutoDelete(false);
    playerzones.setAutoDelete(true);
    playerrealms.setAutoDelete(true);

    ex->show();

    connect(&timer, SIGNAL(timeout()), this, SLOT(replaytimer()));

    link = NULL;
}

exConnection::~exConnection()
{
    mobs.clear();
    players.clear();
    mobinfo.clear();

    delete ex;

    if (ds)
	delete ds;

    if (file)
	delete file;

    if (link)
	delete link;

}

void exConnection::customEvent(QCustomEvent * e)
{
    exPacketEvent *pe;
    exPacket *p;
    exPacket np;

    if (e->type() != EXSNIFFER_EVENT_PACKET) {
	QObject::customEvent(e);
	return;
    }

    pe = (exPacketEvent *) e;

    p = pe->getPacket();

    if (!p)
	return;

    if (sniff) {
	*ds << *p;
	ds->device()->flush();
    }

    updateTick();

    processPacket(p);

    return;
}

void exConnection::replaytimer()
{
    exTimeType delay;

    updateTick();

    if (nextpacket)
	qApp->postEvent(this, new exPacketEvent(nextpacket));

    if (!ds->atEnd()) {
	nextpacket = new exPacket();
	(*ds) >> (*nextpacket);
	delay = nextpacket->tick - exTick;
	if (delay < 0)
	    delay = 0;
        if (delay > 1000)
            delay = 1000;
	timer.start(delay, TRUE);
    }
}

void exConnection::replay()
{
    file->setName(*replayfile);

    if (!file->open(IO_ReadOnly)) {
	qFatal("Couldn't open replayfile");
    }

    ds = new QDataStream(file);

    replaytimer();
}

void exConnection::shutdown(QString why)
{
    if (!alive) {
	return;
    }

    alive = false;

    if (why) {
	qWarning(why);
    }
}

void exConnection::listSelectionChanged(QListViewItem * i)
{
    exMob *mob = (exMob *) i;

    if (mob)
	selectedid = mob->getInfoID();
    else
	selectedid = 0;
    ex->Map->dirty();
}

void exConnection::processPacket(exPacket * p)
{
    unsigned int id;
    unsigned int infoid;
    unsigned int srcid, destid;
    unsigned char bigver;
    unsigned char minver;
    unsigned int head;
    unsigned int linenum;
    unsigned int num;
    unsigned int speed;
    QString name;
    QString surname;
    QString guild;
    QString info;
    QString title;
    QByteArray data;
    unsigned int level;
    exMob *mob;
    Realm mobrealm;
    unsigned int x, y, z;
    unsigned int hp;
    bool ismob;
    unsigned int command;
    unsigned int seq;
    exMapInfo *mi;
    int what;
    int tp;
    int zone;
    int *intptr;
    Realm *rptr;
    unsigned int i;

    if (!(p->is_udp && !p->from_server)) {
	p->decrypt(cryptkey);
    }

    if (p->is_udp && p->from_server) {
	seq = p->getShort();
	command = p->getByte();
	switch (command) {
	  case 9:
	      speed = p->getShort();
	      head = p->getShort();
	      x = p->getLong();
	      y = p->getLong();
	      p->skip(8);
	      z = p->getShort();
	      id = p->getShort();
	      p->skip(2);
	      hp = p->getByte();
	      mob = mobs.find((void *) ((unsigned int) id));
	      if (mob) {
		  mob->setPosition(x, y, z);
		  mob->setHead(head);
                  mob->setSpeed(speed);
		  mob->setHP(hp);
		  ex->Map->dirty();
		  if (prefs.sort_when == exPrefs::sortAlways)
		      ex->ListViewMobs->sort();
	      }
	      break;
	  case 1:
	      id = p->getShort();
	      speed = p->getShort();
	      z = p->getShort();
	      p->skip(2);
	      x = p->getLong();
	      y = p->getLong();
	      head = p->getShort();
	      mob = players.find((void *) ((unsigned int) id));
	      if (mob) {
		  mob->setPosition(x, y, z);
		  mob->setHead(head);
                  mob->setSpeed(speed);
		  ex->Map->dirty();
		  if (prefs.sort_when == exPrefs::sortAlways)
		      ex->ListViewMobs->sort();
	      }
	      break;
	  case 0x12:
	      id = p->getShort();
	      head = p->getShort();
	      p->skip(4);
	      hp = p->getByte();
	      mob = players.find((void *) ((unsigned int) id));
	      if (mob) {
		  mob->setHead(head);
		  if (hp < 100)
		      mob->setHP(hp);
		  ex->Map->dirty();
		  if (prefs.sort_when == exPrefs::sortAlways)
		      ex->ListViewMobs->sort();
	      }
	      break;
	}
    } else if (!p->is_udp && !p->from_server) {
	seq = p->getShort();
	srcid = p->getShort();
	p->skip(2);
	command = p->getShort();
	destid = p->getShort();
	switch (command) {
	  case 0x01:
	      p->skip(2);
	      playerz = p->getShort();
	      p->skip(2);
	      playerx = p->getLong();
	      playery = p->getLong();
	      playerhead = (p->getShort()) & 0xfff;
	      mi = ex->Map->getMap();
	      if (mi && !mi->right(playerzone, playerx, playery)) {
		  ex->Map->setMap(NULL);
		  mi = NULL;
	      }
	      if (!mi) {
		  mi = exMapInfo::get(playerzone, playerx, playery);
		  if (mi) {
		      ex->Map->setMap(mi);
		      ex->ListViewMobs->triggerUpdate();
		  }
	      }
	      ex->X->setNum((mi) ? playerx - mi->getBaseX() : playerx);
	      ex->Y->setNum((mi) ? playery - mi->getBaseY() : playery);
	      ex->Z->setNum(playerz);
	      ex->Zone->setNum(playerzone);
	      ex->Map->dirty();
	      if (prefs.sort_when == exPrefs::sortPlayer || prefs.sort_when == exPrefs::sortAlways)
		  ex->ListViewMobs->sort();
              if (link && mi)
                link->send(QString("%1 %2 %3").arg(playerx - mi->getBaseX()).arg(playery - mi->getBaseY()).arg(playerz));
	      break;
	  case 0x12:
	      playerhead = (p->getShort()) & 0xfff;
	      ex->Map->dirty();
	      break;
	  case 0x18:
	      if (ex->AutoSelectTarget->isOn()) {
		  selectID(destid);
		  ex->Map->dirty();
	      }
	      break;
	}
    } else if (!p->is_udp && p->from_server) {
	command = p->getByte();
	switch (command) {
	  case 0x8a:
	      p->skip(2);
	      bigver = p->getByte();
	      minver = p->getByte();
	      p->skip(1);
	      cryptkey = p->getBytes(12);
	      break;
	  case 0x55:
	      p->skip(24);
	      do {
		  name = p->getZeroString(48);
		  p->skip(74);
                  mobrealm = (Realm) p->getByte();
                  p->skip(3);
		  zone = p->getByte();
		  p->skip(57);
		  if ((name.length() > 0) && (zone != 0)) {
		      intptr = new int;

		      *intptr = zone;
		      playerzones.replace(name, intptr);
                      rptr = new Realm;
                      *rptr = mobrealm;
                      playerrealms.replace(name, rptr);
		  }
	      } while (name.length() > 0);
	      break;
	  case 0x88:
	      selfid = id = p->getShort();
	      p->skip(2);
	      playerx = p->getLong();
	      playery = p->getLong();
	      mobinfo.clear();
	      mobs.clear();
	      players.clear();
	      ex->Map->dirty();
	      break;
	  case 0x0a:
	      id = p->getShort();
	      mob = mobinfo.take((void *) ((unsigned int) id));
	      if (mob) {
		  if (mob->isMob())
		      mobs.remove((void *) ((unsigned int) mob->getID()));
		  else
		      players.remove((void *) ((unsigned int) mob->getID()));
	      }
	      ex->Map->dirty();
	      break;
	  case 0x7c:
	  case 0x72:
	      infoid = 0;
	      head = 0;
              mobrealm = rFriend;
	      if (command == 0x7c) {
		  id = p->getShort();
		  infoid = p->getShort();
		  x = p->getLong();
		  y = p->getLong();
		  z = p->getShort();
		  head = p->getShort();
		  p->skip(4);
                  mobrealm = (Realm) p->getByte();
		  level = p->getByte();
		  p->skip(2);
		  name = p->getPascalString();
		  guild = p->getPascalString();
		  surname = p->getPascalString();
		  ismob = false;
	      } else {
		  infoid = id = p->getShort();
		  p->skip(2);
		  head = p->getShort();
		  z = p->getShort();
		  x = p->getLong();
		  y = p->getLong();
		  p->skip(5);
		  level = p->getByte();
		  p->skip(2);
		  name = p->getPascalString();
		  guild = p->getPascalString();
		  surname = p->getPascalString();
		  ismob = true;
	      }

	      mob = NULL;
	      if (ismob)
		  mob = mobs.take((void *) ((unsigned int) id));
	      else
		  mob = players.take((void *) ((unsigned int) id));
	      if (mob) {
		  mobinfo.remove((void *) ((unsigned int) mob->getInfoID()));
		  delete mob;
	      }
	      mobinfo.remove((void *) ((unsigned int) infoid));

	      mob = new exMob(ex->ListViewMobs, this, ismob, id, infoid, name, level, x, y, z, 100);
	      mob->setHead(head);
              mob->setRealm(mobrealm);

	      if (ismob) {
		  mobs.insert((void *) ((unsigned int) id), mob);
	      } else {
		  players.insert((void *) ((unsigned int) id), mob);
              }
	      mobinfo.insert((void *) ((unsigned int) infoid), mob);
	      ex->Map->dirty();

	      break;
	  case 0x14:
	      p->skip(2);
	      infoid = p->getShort();
	      p->skip(7);
	      hp = p->getByte();
	      mob = mobinfo.find((void *) ((unsigned int) infoid));
	      if (mob) {
		  mob->setHP(hp);
	      }
	      break;
	  case 0x1f:
	      playerzone = p->getShort();
	      intptr = playerzones.find(playername);
	      if (intptr) {
		*intptr = playerzone;
	      }
	      break;
          case 0xaa:
	      num=p->getByte();
              p->skip(3);
              for (i=0;i<num;i++) {
  	        data=p->getBytes(18);
                name=p->getPascalString();
		if (name.length()>0)
                  exItem::seen(name,data);
              }
              break;         
	  case 0x6c:
	      name=p->getPascalString();
              info="";
              do {
                linenum=p->getByte();
                if (linenum != 0) 
                  info=info.append(p->getPascalString()).append("\n");
              } while (linenum != 0);              
              exItem::seen(name,info);              
              break;
	  case 0xbe:
	      what = p->getByte();
	      p->skip(1);
	      tp = p->getByte();
	      if ((what == 3) && (tp == 0)) {
		  p->skip(1);
		  playerlevel = p->getByte();
		  playername = p->getPascalString();
                  title = QString("Excalibur -- ").append(playername);
                  if (link)
                    title = title.append("  ").append(link->descr());
		  ex->setCaption(title);
		  if (playerzone == 0) {
		      intptr = playerzones.find(playername);
		      if (intptr) {
			  playerzone = *intptr;
		      }
		      rptr = playerrealms.find(playername);
		      if (rptr) {
			  playerrealm = *rptr;
		      }
		  }
	      }
	      break;
	  default:
	      break;
	}
    }
}

void exConnection::selectID(unsigned int id)
{
    exMob *m;

    selectedid = id;

    m = mobinfo.find((void *) id);
    if (m) {
	m->touch();
	ex->ListViewMobs->setSelected(m, true);
	ex->ListViewMobs->ensureItemVisible(m);
    }
    ex->Map->dirty();
}

const QPtrDict < exMob > &exConnection::getMobs() const
{
    return mobinfo;
}

void exConnection::spawnEditor() 
{
  exMapInfo *mi;

  if (! link) {
    QMessageBox::critical(ex, "No Link", "You need to start Excalibur with the --link argument to enable\nposition passing to the external editor");
    return;
  }
  mi=ex->Map->getMap();
  if (! mi) {
    QMessageBox::critical(ex, "No Map Information", "The area you are in must be named in\nmapinfo.txt before you can enable editing.");
    return;
  }
  QProcess *p=new QProcess();
  p->addArgument("java");
  p->addArgument("-jar");
  p->addArgument("editor.jar");
  p->addArgument(link->hostport());
  QFile f;
  f.setName(QString("usermaps/").append(mi->getName()));
  if (f.exists()) {
    p->addArgument(f.name());
  } else {
    p->addArgument(QString("maps/").append(mi->getName()));
  }
  p->addArgument(QString("usermaps/").append(mi->getName()));
  p->start();
}