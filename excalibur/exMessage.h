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

class exMessage;

#ifndef __EX_MSG_H__
#define __EX_MSG_H_

#include <qstring.h>
#include "exPrefs.h"

class exMessage{
	
	public:
		exMessage::exMessage( QString*);
		void exMessage::parseMsg();
		QString getFormattedText() { return FormattedText; }
		QString getMsgType()       { return MsgType; }
		int exMessage::getType();

	private:
		QString Msg,
		MsgType,
		MsgText,
		Sender,
		Recvr,
		FormattedText;
		
};
		
#endif