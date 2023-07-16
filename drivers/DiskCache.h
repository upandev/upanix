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
#ifndef _DISK_CACHE_H_
#define _DISK_CACHE_H_

#include <BTree.h>
#include <list.h>
#include <PIT.h>
#include <StringUtil.h>
#include <MemPool.h>
#include <mutex.h>

#define DiskCache_SUCCESS 0
#define DiskCache_FAILURE 1

class DiskDrive;

class DiskCacheKey : public BTreeKey
{
	private:
		unsigned m_uiSectorID ;

	public:
		DiskCacheKey() : m_uiSectorID(0) { }
		DiskCacheKey(const unsigned uiSectorID) : m_uiSectorID(uiSectorID) { }

		virtual bool operator<(const BTreeKey& rKey) const
		{
			const DiskCacheKey& rhs = static_cast<const DiskCacheKey&>(rKey) ;

			return m_uiSectorID < rhs.GetSectorID() ;
		}

		unsigned GetSectorID() const { return m_uiSectorID ; }
		void SetSectorID(const unsigned& uiSectorID) { m_uiSectorID = uiSectorID ; }
} ;

class DiskCacheValue : public BTreeValue
{
	private:
		static const unsigned SEC_SIZE = 512 ;
		unsigned m_uiLastAccess ;
		unsigned m_uiHitCount ;
		byte m_bSectorBuffer[ SEC_SIZE ] ;

	public:
		DiskCacheValue() : m_uiLastAccess(PIT::Instance().GetClockCount()), m_uiHitCount(1)
    { 
    }

		byte* GetSectorBuffer() { return m_bSectorBuffer ; }

		inline unsigned GetHitCount() const { return m_uiHitCount ; }
		inline unsigned GetLastAccess() const { return m_uiLastAccess ; }

		void Read(byte* pDest)
		{
			memcpy(pDest, m_bSectorBuffer, SEC_SIZE) ;
			m_uiHitCount++ ;
			m_uiLastAccess = PIT::Instance().GetClockCount() ;
		}

		void Write(const byte* pSrc)
		{
			memcpy(m_bSectorBuffer, pSrc, SEC_SIZE) ;
			m_uiHitCount++ ;
			m_uiLastAccess = PIT::Instance().GetClockCount() ;
		}
} ;

class DiskCache;

class LFUSectorManager : public BTree::InOrderVisitor
{
	private:
		class CacheRankNode	
		{
			public:
				unsigned m_uiSectorID ;
				double m_dRank ;

				inline bool operator<(const CacheRankNode& rhs) const 
				{
					return m_dRank < rhs.m_dRank ;
				}
		} ;

		upan::list<CacheRankNode> m_mReleaseList ;

    DiskCache& _mCache;
		unsigned m_bReleaseListBuilt ; 
		unsigned m_uiMaxRelListSize ;
		unsigned m_uiBuildBreak ;
		bool m_bAbort ;

		unsigned m_uiCurrent ;
		unsigned m_uiBuildCount ;

		upan::mutex lruMutex;

	private:
		inline bool IsCacheFull() ;

	public:
		LFUSectorManager(DiskCache&);

		void Run() ;
		bool ReplaceCache(unsigned uiSectorID, byte* bDataBuffer) ;
		void operator()(const BTreeKey& rKey, BTreeValue* pValue) ;

		bool Abort() const { return m_bAbort ; }
} ;

class DestroyDiskCacheKeyValue ;

class DiskCache
{
	public:
    DiskCache();
    ~DiskCache();
		class SecKeyCacheValue
		{
			public:
				SecKeyCacheValue() { } 
				SecKeyCacheValue(unsigned uiSectorID, byte* pBuffer) : m_uiSectorID(uiSectorID), m_pSectorBuffer(pBuffer) { }

				inline bool operator==(const SecKeyCacheValue& rhs)
				{
					return m_uiSectorID == rhs.m_uiSectorID ;
				}

				inline SecKeyCacheValue& operator=(const SecKeyCacheValue& rhs)
				{
					m_uiSectorID = rhs.m_uiSectorID ;
					m_pSectorBuffer = rhs.m_pSectorBuffer ;
					return *this ;
				}

				unsigned m_uiSectorID ;
				byte* m_pSectorBuffer ;
		};

    bool Full() const 
    { 
      return _tree.GetTotalElements() >= MAX_CACHE_SECTORS; 
    }
		bool ReplaceCache(unsigned uiSectorID, byte* bDataBuffer)
    {
      return _LFUSectorManager.ReplaceCache(uiSectorID, bDataBuffer);
    }
    DiskCacheValue* Find(unsigned uiSectorID)
    {
      return static_cast<DiskCacheValue*>(_tree.Find(DiskCacheKey(uiSectorID)));
    }
    DiskCacheValue* Add(unsigned uiSectorID, byte* bDataBuffer)
    {
      DiskCacheValue* val = CreateValue(bDataBuffer);
      if(!_tree.Insert(CreateKey(uiSectorID), val))
        return nullptr;
      return val;
    }
    void LFUCacheCleanUp()
    {
      _LFUSectorManager.Run();
    }
    void InsertToDirtyList(const DiskCache::SecKeyCacheValue& v);
    bool Get(SecKeyCacheValue& v);

	private:
    DiskCacheKey* CreateKey(unsigned uiSectorID);
    DiskCacheValue* CreateValue(const byte* pSrc);

		//static const int MAX_CACHE_SECTORS = 16384;
		//has to be a multiple of 1024
    static const int MAX_CACHE_SECTORS = 32;
		DestroyDiskCacheKeyValue* _destroyKeyValue;
		MemPool<DiskCacheKey>& _cacheKeyMemPool;
		MemPool<DiskCacheValue>& _cacheValueMemPool;
		upan::list<SecKeyCacheValue> _dirtyCacheList;
		BTree _tree;
		LFUSectorManager _LFUSectorManager;

    friend class DestroyDiskCacheKeyValue;
    friend class LFUSectorManager;
};

class DestroyDiskCacheKeyValue : public BTree::DestroyKeyValue
{
	private:
		DiskCache& _cache;

	public:
		DestroyDiskCacheKeyValue(DiskCache& cache) : _cache(cache) { }

		void DestroyKey(BTreeKey* pKey)
		{
			_cache._cacheKeyMemPool.Release(static_cast<DiskCacheKey*>(pKey)) ;
		}

		void DestroyValue(BTreeValue* pValue)
		{
			_cache._cacheValueMemPool.Release(static_cast<DiskCacheValue*>(pValue)) ;
		}
} ;

void DiskCache_Setup(DiskDrive& diskDrive) ;
void DiskCache_StopReleaseCacheTask(DiskDrive* pDiskDrive) ;
void DiskCache_StartReleaseCacheTask(DiskDrive& pDiskDrive) ;

#endif 
