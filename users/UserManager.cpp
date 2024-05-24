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
# include <UserManager.h>
# include <FileOperations.h>
# include <IODescriptorTable.h>
# include <StringUtil.h>
# include <GenericUtil.h>
# include <uniq_ptr.h>

UserManager::UserManager() : _userListFileName(upan::string(OSIN_PATH) + ".user.lst")
{
	if(!FileOperations::Instance().fileExists(_userListFileName)) {
    CreateNewUserList();
	} else {
    if(!LoadUserList()) {
      printf("\n User List File Corrupted/Not Found. Using Default root User\n");
      InitializeDefaultUserList();
    }
  }
}

void UserManager::CreateNewUserList() {
	unsigned short usPerm = S_OWNER((ATTR_READ | ATTR_WRITE)) | S_GROUP(ATTR_READ) | S_OTHERS(ATTR_READ);

  FileOperations::Instance().create(_userListFileName, ATTR_TYPE_FILE, usPerm);

	InitializeDefaultUserList();
	WriteUserList();
}

void UserManager::InitializeDefaultUserList() {
  _users.insert(UserMap::value_type("root", new User("root", "root123", FS_ROOT_DIR, SUPER_USER)));
}

void UserManager::WriteUserList() {
  auto& file = FileOperations::Instance().open(_userListFileName, O_RDWR | O_TRUNC).value();

  for(auto u : _users) {
    const User& user = *u.second;

    int buf_size = user.Name().length() + user.Password().length() + user.HomeDirPath().length() + 1 /*type*/ + 4 /* 4 new lines */;
    upan::uniq_ptr<char[]> buffer(new char[buf_size + 1]);
    sprintf(buffer.get(), "%s\n%s\n%s\n%d\n", user.Name().c_str(), user.Password().c_str(), user.HomeDirPath().c_str(), user.Type());

    try {
      file.write((const char*)buffer.get(), buf_size);
    } catch(const upan::exception&) {
      FileOperations::Instance().close(file.id());
      throw;
    }
  }

	if(!FileOperations::Instance().close(file.id())) {
    throw upan::exception(XLOC, "erroring closing fd for user file list");
  }
}

bool UserManager::LoadUserList() {
  auto& file = FileOperations::Instance().open(_userListFileName.c_str(), O_RDONLY).value();

  upan::string name;
  while(FileOperations_ReadLine(file.id(), name))
  {
    upan::string password, homeDirPath, szType;
    if(!FileOperations_ReadLine(file.id(), password)
      || !FileOperations_ReadLine(file.id(), homeDirPath)
      || !FileOperations_ReadLine(file.id(), szType))
      throw upan::exception(XLOC, "user list file is corrupted");
    
    USER_TYPES type = NO_USER;
    if(szType == "1")
      type = SUPER_USER;
    else if(szType == "2")
      type = NORMAL_USER;
    else if(szType != "0")
      throw upan::exception(XLOC, "invalid user type (%s) while reading user list file", szType.c_str());
    _users.insert(UserMap::value_type(name, new User(name, password, homeDirPath, type)));
  }

  return FileOperations::Instance().close(file.id());
}

bool UserManager::Create(const upan::string& name, const upan::string& password, const upan::string& homeDirPath, USER_TYPES type)
{
  if(!ValidateName(name))
    return false;

  if(!ValidateName(password))
    return false;

  if(homeDirPath.length() > MAX_HOME_DIR_LEN)
  {
    printf("\n home dir path of length %d is longer than %d", homeDirPath.length(), MAX_HOME_DIR_LEN);
    return false;
  }

  if(!FileOperations::Instance().directoryExists(homeDirPath)) {
    printf("\n invalid home dir path %s", homeDirPath.c_str());
    return false;
  }

  _users.insert(UserMap::value_type(name, new User(name, password, homeDirPath, type)));

	WriteUserList();

  return true;
}

bool UserManager::Delete(const upan::string& name) {
  auto it = _users.find(name);
  if(it == _users.end())
  {
    printf("\n no such user %s to delete", name.c_str());
    return false;
  }
  delete it->second;
  _users.erase(it);
	WriteUserList();
  return true;
}

const User* UserManager::GetUserEntryByName(const upan::string& name) const {
  auto i = _users.find(name);
  if(i == _users.end())
    return nullptr;
  return i->second;
}

bool UserManager::ValidateName(const upan::string& name) {
  if(GetUserEntryByName(name) != nullptr) {
    printf("\n User %s already exists", name.c_str());
    return false;
  }

	if(name.length() > MAX_USER_LENGTH) {
    printf("\n User name length (%d) is too long (MAX: %d)", name.length(), MAX_USER_LENGTH);
		return false;
  }

	if(name.length() < MIN_USER_LENGTH) {
    printf("\n User name length (%d) is too short (MIN: %d)", name.length(), MIN_USER_LENGTH);
		return false;
  }

  for(int i = 0; i < name.length(); ++i) {
		if(!isalnum(name[i]))
    {
      printf("\n only alpha numeric characters allowed in user name (invalid char -> %c)", name[i]);
			return false;
    }
	}

	return true;
}
