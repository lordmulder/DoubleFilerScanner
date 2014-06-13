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

#include "Taskbar.h"
#include "Config.h"

#include <QWidget>
#include <QIcon>
#include <QMutexLocker>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShObjIdl.h>

class TaskbarData
{
	friend class Taskbar;

protected:
	TaskbarData(void)
	{
		winMsg = 0;
		ptbl = NULL;
	}

	UINT winMsg;
	ITaskbarList3 *ptbl;
};

QMutex Taskbar::s_lock;
TaskbarData *Taskbar::s_data = NULL;

//===================================================================
// Constructor & Destructor
//===================================================================

Taskbar::Taskbar(void)
{
	throw std::runtime_error("Cannot create instance of this class!");
}

Taskbar::~Taskbar(void)
{
}

//===================================================================
// Public Functions
//===================================================================

void Taskbar::init(void)
{
	QMutexLocker lock(&s_lock);

	if(!s_data)
	{
		s_data = new TaskbarData();
		s_data->winMsg = RegisterWindowMessageW(L"TaskbarButtonCreated");
		
		if(s_data->winMsg == 0)
		{
			qWarning("RegisterWindowMessageW has failed!");
		}
	}
}
	
void Taskbar::uninit(void)
{
	QMutexLocker lock(&s_lock);

	if(s_data)
	{
		if(s_data->ptbl)
		{
			s_data->ptbl->Release();
			s_data->ptbl = NULL;
		}
		MY_DELETE(s_data);
	}
}

bool Taskbar::handleWinEvent(void *message, long *result)
{
	QMutexLocker lock(&s_lock);
	bool stopEvent = false;

	if(s_data && (s_data->winMsg != 0))
	{
		if(((MSG*)message)->message == s_data->winMsg)
		{
			if(!s_data->ptbl) createInterface();
			*result = (s_data->ptbl) ? S_OK : S_FALSE;
			stopEvent = true;
		}
	}

	return stopEvent;
}

bool Taskbar::setTaskbarState(QWidget *window, TaskbarState state)
{
	QMutexLocker lock(&s_lock);
	bool result = false;
	
	if(s_data && s_data->ptbl && window)
	{
		HRESULT hr = HRESULT(-1);

		switch(state)
		{
		case TaskbarNoState:
			hr = s_data->ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_NOPROGRESS);
			break;
		case TaskbarNormalState:
			hr = s_data->ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_NORMAL);
			break;
		case TaskbarIndeterminateState:
			hr = s_data->ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_INDETERMINATE);
			break;
		case TaskbarErrorState:
			hr = s_data->ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_ERROR);
			break;
		case TaskbarPausedState:
			hr = s_data->ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_PAUSED);
			break;
		}

		result = SUCCEEDED(hr);
	}

	return result;
}

void Taskbar::setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue)
{
	QMutexLocker lock(&s_lock);

	if(s_data && s_data->ptbl && window)
	{
		s_data->ptbl->SetProgressValue(reinterpret_cast<HWND>(window->winId()), currentValue, maximumValue);
	}
}

void Taskbar::setOverlayIcon(QWidget *window, QIcon *icon)
{
	QMutexLocker lock(&s_lock);

	if(s_data && s_data->ptbl && window)
	{
		s_data->ptbl->SetOverlayIcon(window->winId(), (icon ? icon->pixmap(16,16).toWinHICON() : NULL), L"DoubeFileScanner");
	}
}

//===================================================================
// Private Functions
//===================================================================

void Taskbar::createInterface(void)
{
	if(s_data && (!s_data->ptbl))
	{
		ITaskbarList3 *ptbl = NULL;
		const HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));

		if(SUCCEEDED(hr))
		{
			const HRESULT hr2 = ptbl->HrInit();
			if(SUCCEEDED(hr2))
			{
				s_data->ptbl = ptbl;
			}
			else
			{
				ptbl->Release();
				qWarning("ITaskbarList3::HrInit() has failed!");
			}
		}
		else
		{
			qWarning("ITaskbarList3 could not be created!");
		}
	}
}
