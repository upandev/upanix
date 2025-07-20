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
#include <PIC.h>
#include <PortCom.h>
#include <PS2KeyboardDriver.h>
#include <PS2MouseDriver.h>
#include <PS2Controller.h>
#include <option.h>

static constexpr int STAT_OBF = 0x01;
static constexpr int STAT_IBF = 0x02;
static constexpr int CMD_REBOOT	= 0xFE;
static constexpr int CMD_DISABLE_PORT1 = 0xAD;
static constexpr int CMD_ENABLE_PORT1 = 0xAE;
static constexpr int CMD_DISABLE_PORT2 = 0xA7;
static constexpr int CMD_ENABLE_PORT2 = 0xA8;
static constexpr int CMD_WRITE_NEXT_PORT2 = 0xD4;

PS2Controller::PS2Controller() {
  try {
    Initialize();
    KC::MConsole().LoadMessage("PS2 Controller Initialization", Success);
  } catch (const upan::exception &e) {
    printf("\n Failed to initialize PS2 controller: %s", e.ErrorMsg().c_str());
    KC::MConsole().LoadMessage("PS2 Controller Initialization", Failure);
  }
}

void PS2Controller::Initialize() {
  IrqManager::Instance().DisableIRQ(StdIRQ::Instance().KEYBOARD_IRQ);
  IrqManager::Instance().DisableIRQ(StdIRQ::Instance().MOUSE_IRQ);

  printf("\nInitializing PS2 controller");
  SendCommand(COMMAND_PORT, CMD_DISABLE_PORT1, "");
  SendCommand(COMMAND_PORT, CMD_DISABLE_PORT2, "");
  ClearOutputBuffer();

  SendCommand(COMMAND_PORT, 0x20, "Read status");

  auto status = ReceiveData().valueOrThrow(XLOC, "failed read status");
  //check bit 5 to verify if second port is disabled. If it is not set - then we are dealing with an old ps2 controller that doesn't support second port
  bool dualChannelController = (status & 0x20);
  if (!dualChannelController) {
    printf("PS2 controller doesn't support second port");
  }
  printf("\n current config: %x", status);
  // clear bit 0 --> disable port1 interrupt,
  // clear bit 1 --> disable port2 interrupt,
  // clear bit 4 --> enable first port,
  // clear bit 5 --> enable second port
  status &= ~(0x33);
  printf("\n new config: %x", status);

  printf("\n update status");
  SendCommand(COMMAND_PORT, 0x60, "status");
  SendCommand(DATA_PORT, status, "modified status");

  printf("\n controller self test");
  SendCommand(COMMAND_PORT, 0xAA, "controller self test");
  auto selfTestResp = ReceiveData().valueOrThrow(XLOC, "failed to read self test response");
  if (selfTestResp != 0x55) {
    printf("\n self test response is %x (!= 0x55)", selfTestResp);
    printf("\n restoring to previous status");
    SendCommand(COMMAND_PORT, 0x60, "status");
    SendCommand(DATA_PORT, status, "restore Status");
    throw upan::exception(XLOC, "PS2 controller self test failed");
  }

  if (dualChannelController) {
    //confirm controller supports 2 channels (ports)
    SendCommand(COMMAND_PORT, CMD_ENABLE_PORT2, "Status");
    //read config byte again
    SendCommand(COMMAND_PORT, 0x20, "Init Command");
    status = ReceiveData().valueOrThrow(XLOC, "failed to read status");
    printf("\n config after init: %x", status);
    dualChannelController = !(status & 0x20);
    if (dualChannelController) {
      //disable second port again
      SendCommand(COMMAND_PORT, CMD_DISABLE_PORT2, "Status");
    }
  }

  printf("\n testing port 1");
  SendCommand(COMMAND_PORT, 0xAB, "test port 1");
  const bool port1Good = ReceiveData().valueOrThrow(XLOC, "failed to test ack from port1") == 0;

  printf("\n testing port 2");
  SendCommand(COMMAND_PORT, 0xA9, "test port 2");
  const bool port2Good = ReceiveData().valueOrThrow(XLOC, "failed to test ack from port2") == 0;

  if (!port1Good && !port2Good) {
    throw upan::exception(XLOC, "PS2 controller doesn't have any working ports.");
  }

  if (port1Good) {
    //enable IRQ1
    status |= 0x1;
    printf("\n enable port1");
    SendCommand(COMMAND_PORT, CMD_ENABLE_PORT1, "enabled port1");

    SendCommand(DATA_PORT, 0xF5, "disable scanning");
    WaitForAck();
    SendCommand(DATA_PORT, 0xF2, "identify command");
    WaitForAck();
    auto devCode1 = ReceiveData().valueOrThrow(XLOC, "failed to read first byte of device id response");
    auto devCode2 = ReceiveData();
    printf("\n Port1 device code: %x", devCode1);
    if (!devCode2.isEmpty())
      printf("\n Port1 device code2: %x", devCode2.value());

    SendCommand(DATA_PORT, 0xF4, "enable scanning");
    WaitForAck();

    InitDriver(devCode1);
  }

  ClearOutputBuffer();
  
  if (port2Good) {
    //enable IRQ12
    status |= 0x2;
    printf("\n enable port2");
    SendCommand(COMMAND_PORT, CMD_ENABLE_PORT2, "enabled port2");

    SendCommand2(0xF5, "disable scanning");
    WaitForAck();
    printf("\n sendPacket id command");
    SendCommand2(0xF2, "identify command");
    WaitForAck();
    auto devCode1 = ReceiveData().valueOrThrow(XLOC, "failed to read first byte of device id response");
    auto devCode2 = ReceiveData();
    printf("\n Port2 device code: %x", devCode1);
    if (!devCode2.isEmpty())
      printf("\n Port2 device code2: %x", devCode2.value());

    SendCommand2(0xF4, "enable scanning");
    WaitForAck();

    InitDriver(devCode1);
  }

  printf("\n PS2 controller - enable irqs");
  SendCommand(COMMAND_PORT, 0x60, "status");
  SendCommand(DATA_PORT, status, "enable IRQs");
}

void PS2Controller::InitDriver(uint32_t devCode) {
  if (devCode == 0xAB) {
    PS2KeyboardDriver::Instance();
  } else if (devCode == 0x00 || devCode == 0x03 || devCode == 0x04) {
    PS2MouseDriver::Instance();
  } else {
    printf("\n Unsupported PS2 device ID: %u", devCode);
  }
}

bool PS2Controller::WaitForWrite() {
  int iTimeOut = 0xFFFFF ;
  while((iTimeOut > 0) && (PortCom_ReceiveByte(COMMAND_PORT) & STAT_IBF))
    iTimeOut--;
  return (iTimeOut > 0) ;
}

bool PS2Controller::WaitForRead() {
  int iTimeOut = 0xFFFFF;
  while((iTimeOut > 0) && !(PortCom_ReceiveByte(COMMAND_PORT) & STAT_OBF))
    iTimeOut--;
  return (iTimeOut > 0) ;
}

void PS2Controller::Reboot() {
  WaitForWrite();
  PortCom_SendByte(COMMAND_PORT, CMD_REBOOT);
  __asm__ __volatile__("cli; hlt");
}

void PS2Controller::SendCommand(uint16_t port, uint8_t cmd, const upan::string& opName) {
  if(!WaitForWrite()) {
    throw upan::exception(XLOC, "PS2 command failed. Port: %x, Command: %x, Action: %s", port, cmd, opName.c_str());
  }
  PortCom_SendByte(port, cmd) ;
}

void PS2Controller::SendCommand2(byte command, const upan::string& opName) {
  SendCommand(COMMAND_PORT, CMD_WRITE_NEXT_PORT2, "prep port2 command");
  SendCommand(DATA_PORT, command, opName);
}

void PS2Controller::WaitForAck() {
  const auto data = ReceiveData();
  if (data.valueOrThrow(XLOC, "ack timed out") != 0xFA) {
    throw upan::exception(XLOC, "invalid ack: %x", data.value());
  }
}

upan::option<uint8_t> PS2Controller::ReceiveData() {
  if(!WaitForRead()) {
    return upan::option<uint8_t>::empty();
  }
	return upan::option<uint8_t>(PortCom_ReceiveByte(DATA_PORT));
}

void PS2Controller::ClearOutputBuffer() {
  int i = 0;
  do {
    if (!WaitForRead()) {
      break;
    }
    auto data = PortCom_ReceiveByte(DATA_PORT);
    //printf("\n Clearing Output buffer data: %x", data);
    ++i;
  } while(i < 1000);
}