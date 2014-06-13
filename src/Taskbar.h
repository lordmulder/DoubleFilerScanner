///////////////////////////////////////////////////////////////////////////////
// Double File Scanner
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QMutex>

class QWidget;
class QIcon;
class TaskbarData;

class Taskbar
{
public:
	Taskbar(void);
	~Taskbar(void);

	//Taskbar states
	enum TaskbarState
	{
		TaskbarNoState = 0,
		TaskbarNormalState = 1,
		TaskbarIndeterminateState = 2,
		TaskbarPausedState = 3,
		TaskbarErrorState = 4
	};
	
	//Public interface
	static bool handleWinEvent(void *message, long *result);
	static bool setTaskbarState(QWidget *window, TaskbarState state);
	static void setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue);
	static void setOverlayIcon(QWidget *window, QIcon *icon);

	static void init(void);
	static void uninit(void);

private:
	static void createInterface(void);
	static QMutex s_lock;
	static TaskbarData *s_data;
};
