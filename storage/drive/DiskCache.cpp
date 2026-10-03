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
# include <DiskCache.h>
# include <Floppy.h>
# include <ATADeviceController.h>
# include <DMM.h>
# include <stdio.h>
# include <ProcessManager.h>

LFUSectorManager::LFUSectorManager(DiskCache& mCache) :
  _mCache(mCache),
	m_bReleaseListBuilt(false),
	m_uiMaxRelListSize(0.1 * _mCache.MAX_CACHE_SECTORS),
	m_uiBuildBreak(0.05 * _mCache.MAX_CACHE_SECTORS),
	m_bAbort(false)
{
}

bool LFUSectorManager::IsCacheFull()
{
	return _mCache.Full();
}

void LFUSectorManager::Run()
{
  upan::mutex_guard g(lruMutex);
  // this is accessing the tree after cache is full there is no need to lock/unlock the drive mutex
	// because the tree is not altered. And this can be altered only when release cache list is built
	// but if release cache list is built it will not access the tree till release cache list is empty
	if(!IsCacheFull())
		return ;

	if(m_bReleaseListBuilt)
		return ;

  _currentTsc = TscClock::instance().rdtsc();
	m_uiBuildCount = 0 ;

	_mCache._tree.InOrderTraverse(*this) ;

  if(m_bAbort)
		return ;

	if(m_mReleaseList.size() == m_uiMaxRelListSize)
		m_bReleaseListBuilt = true ;

	//debug log
//	if(m_bReleaseListBuilt)
//		printf("\n Release List is Built") ;
}

bool LFUSectorManager::ReplaceCache(unsigned uiSectorID, byte* bDataBuffer)
{
  upan::mutex_guard g(lruMutex);
	if(!m_bReleaseListBuilt)
	{
		//printf("\n Release List is not built yet!") ;
		return true ;
	}

	if(m_mReleaseList.empty())
	{
		m_bReleaseListBuilt = false ;
		//printf("\n Release List is empty, not rebuilt yet!") ;
		return true ;
	}

	bool bFound = false ;
	CacheRankNode node ;
	while(!m_mReleaseList.empty()) {
    node = m_mReleaseList.front();
    m_mReleaseList.pop_front();
		// Skip dirty sectors from replacing
    DiskCache::SecKeyCacheValue skey(node.m_uiSectorID, NULL);
    if(upan::find(_mCache._dirtyCacheList.begin(), _mCache._dirtyCacheList.end(), skey) != _mCache._dirtyCacheList.end())
			continue;
		bFound = true;
		break ;
	}

	if(!bFound)
	{
		printf(" All lfu ranked nodes were marked dirty. Need to rebuild LFU list\n") ;
		m_bReleaseListBuilt = false ;
		return true ;
	}

  if(!(_mCache._tree.Delete(DiskCacheKey(node.m_uiSectorID))))
	{
		printf("\n Failed to delete ranked sector: %u from the Cache BTree", node.m_uiSectorID) ;
		ProcessManager::Instance().Sleep(10000000);
		return false ;
	}

	DiskCacheKey* pKey = _mCache.CreateKey(uiSectorID) ;
	DiskCacheValue* pVal = _mCache.CreateValue(bDataBuffer) ;

	if(!_mCache._tree.Insert(pKey, pVal))
	{
		printf("\n Disk Cache Insert failed!!! %s:%d", __FILE__, __LINE__) ;
		return false ;
	}

	if(m_mReleaseList.empty())
		m_bReleaseListBuilt = false ;

	return true ;
}

void LFUSectorManager::operator()(const BTreeKey& rKey, BTreeValue* pValue)
{
	const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
	const DiskCacheValue* value = static_cast<const DiskCacheValue*>(pValue) ;

	// Skip dirty sectors from lfu ranking
  DiskCache::SecKeyCacheValue skey(key.GetSectorID(), NULL);
  if(upan::find(_mCache._dirtyCacheList.begin(), _mCache._dirtyCacheList.end(), skey) != _mCache._dirtyCacheList.end())
		return;

	uint64_t uiTimeDiff = _currentTsc - value->GetLastAccess() ;
	double dRank = 	(double)uiTimeDiff / (double)value->GetHitCount() ;
  CacheRankNode node;
	if(m_mReleaseList.size() == m_uiMaxRelListSize)
	{
    node = m_mReleaseList.back();
		if(dRank <= node.m_dRank)
			return;
		m_mReleaseList.pop_back();
	}
	node.m_uiSectorID = key.GetSectorID() ;
	node.m_dRank = dRank ;
	m_mReleaseList.sorted_insert_asc(node);
}

DiskCache::DiskCache() :
	_cacheKeyMemPool(MAX_CACHE_SECTORS, 32),
	_cacheValueMemPool(MAX_CACHE_SECTORS, 32),
  _tree(MAX_CACHE_SECTORS),
  _LFUSectorManager(*this)
{
	_destroyKeyValue = new DestroyDiskCacheKeyValue(*this);
	_tree.SetDestoryKeyValueCallBack(_destroyKeyValue);
}

DiskCache::~DiskCache()
{
  delete _destroyKeyValue;
  delete &_cacheKeyMemPool;
  delete &_cacheValueMemPool;
}

void DiskCache::InsertToDirtyList(const DiskCache::SecKeyCacheValue& v)
{
  if(upan::find(_dirtyCacheList.begin(), _dirtyCacheList.end(), v) == _dirtyCacheList.end())
    _dirtyCacheList.push_back(v);
}

DiskCacheKey* DiskCache::CreateKey(unsigned uiSectorID)
{
	auto object = _cacheKeyMemPool.allocate() ;
  if (object.isEmpty()) {
    throw upan::exception(XLOC, "disk cache-key pool is full");
  }
  auto pKey = &object.value();
	pKey->SetSectorID(uiSectorID) ;
	return pKey ;
}

DiskCacheValue* DiskCache::CreateValue(const byte* pSrc)
{
  auto object = _cacheValueMemPool.allocate() ;
  if (object.isEmpty()) {
    throw upan::exception(XLOC, "disk cache-value pool is full");
  }
  auto pValue = &object.value();
	pValue->Write(pSrc) ;
	return pValue ;
}

bool DiskCache::Get(SecKeyCacheValue& v)
{
  if(_dirtyCacheList.empty())
    return false;
  v = _dirtyCacheList.front();
  _dirtyCacheList.pop_front();
  return true;
}
