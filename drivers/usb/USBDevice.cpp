/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
#include <USBDevice.h>
#include <set.h>

USBDevice::USBDevice() : _usLangID(0), _pArrConfigDesc(nullptr), _pStrDescZero(nullptr)
{
}

void USBDevice::SetLangId()
{
  static const upan::set<unsigned short> SUPPORTED_LAND_IDS {0x409};

  for(int i = 0; i < (_pStrDescZero->bLength - 2) / 2; ++i)
  {
    if(SUPPORTED_LAND_IDS.exists(_pStrDescZero->usLangID[i]))
    {
      _usLangID = _pStrDescZero->usLangID[i];
      break;
    }
  }
}

void USBDevice::SetConfigIndex(byte bConfigValue)
{
	_iConfigIndex = 0 ;
	for(int i = 0; i < _deviceDesc.bNumConfigs; i++)
	{
		if(_pArrConfigDesc[ i ].bConfigurationValue == bConfigValue)
		{
			_iConfigIndex = i;
			break;
		}
	}
}

void USBDevice::PrintDeviceStringDetails() const
{
	printf("\n USBDevDetails: Manufac: %s,Prod: %s,SlNo: %s\n", _manufacturer.c_str(), _product.c_str(), _serialNum.c_str()) ;
}
