///////////////////////////////////////////////////////////////////////////////
// Double File Scanner
// Copyright (C) 2014-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#define RELEASE(X) do \
{ \
	(X)->Release(); \
	(X) = NULL; \
} \
while(0)

//===================================================================
// Taskbar Data
//===================================================================

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

//===================================================================
// Constructor & Destructor
//===================================================================

QMutex Taskbar::s_lock;
TaskbarData *Taskbar::s_data = NULL;

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
			qWarning("RegisterWindowMessage() has failed!");
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
			RELEASE(s_data->ptbl);
			CoUninitialize();
		}
		MY_DELETE(s_data);
	}
}

bool Taskbar::handleWinEvent(void *message, long *result)
{
	QMutexLocker lock(&s_lock);

	if(s_data && (s_data->winMsg != 0))
	{
		if(((MSG*)message)->message == s_data->winMsg)
		{
			if(!s_data->ptbl)
			{
				*result = createInterface() ? S_OK : S_FALSE;
			}
			return true;
		}
	}

	return false;
}

bool Taskbar::setTaskbarState(QWidget *window, TaskbarState state)
{
	QMutexLocker lock(&s_lock);
	
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

		return SUCCEEDED(hr);
	}

	return false;
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

bool Taskbar::createInterface(void)
{
	if(s_data && (!s_data->ptbl))
	{
		const HRESULT hrComInit = CoInitialize(NULL);

		if((hrComInit != S_OK) && (hrComInit != S_FALSE))
		{
			qWarning("CoInitialize() has failed!");
			return false;
		}

		ITaskbarList3 *tmp = NULL;
		const HRESULT hrCreate = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&tmp));

		if(!SUCCEEDED(hrCreate))
		{
			qWarning("ITaskbarList3 interface could not be created!");
			CoUninitialize();
			return false;
		}

		const HRESULT hrInitTaskbar = tmp->HrInit();

		if(!SUCCEEDED(hrInitTaskbar))
		{
			qWarning("ITaskbarList3::HrInit() has failed!");
			RELEASE(tmp);
			CoUninitialize();
			return false;
		}

		s_data->ptbl = tmp;
		return true;
	}

	qWarning("Interface was already created!");
	return false;
}
