#ifndef __SOFTOPID_H
#define __SOFTOPID_H


#include "SizeUtils.h"

#include "TaskMgr.h"

#include "BackgroundSubstraction.h"
#include "RoiCounter.h"


namespace lima
{
  class SoftOpBaseClass
  {
    friend class SoftOpExternalMgr;
  protected:
    static const int DEFAULT_HISTORY_SIZE = 128;

    virtual ~SoftOpBaseClass() {};

    virtual void addTo(TaskMgr&,int stage) = 0;
    virtual void prepare() = 0;
  };

  enum SoftOpId
    {
      UNDEF,
      BACKGROUNDSUBSTRACTION,
      BINNING,
      BPM,
      FLATFIELDCORRECTION,
      FLIP,
      MASK,
      ROICOUNTERS,
      SOFTROI
    };

  struct SoftOpKey
  {
    SoftOpKey() : m_id(UNDEF),m_name(NULL) {};
    SoftOpKey(SoftOpId id,const char *name) : m_id(id),m_name(name) {}

    SoftOpId	m_id;
    const char *m_name;
  };

  struct SoftOpInstance
  {
    SoftOpInstance() : m_linkable(false),
		       m_opt(NULL) {};

    SoftOpInstance(const SoftOpKey &akey,
		   const std::string &anAlias) :
      m_key(akey),
      m_alias(anAlias),m_linkable(false),m_opt(NULL) {}
    SoftOpKey		m_key;
    std::string		m_alias;
    bool		m_linkable;
    SoftOpBaseClass*	m_opt;
  };
  
  class SoftOpBackgroundSubstraction : public SoftOpBaseClass
  {
  public:
    SoftOpBackgroundSubstraction();
    ~SoftOpBackgroundSubstraction();
    
    void setBackgroundImage(Data &aData);
    
  protected:
    virtual void addTo(TaskMgr&,int stage);
    virtual void prepare() {};
  private:
    Tasks::BackgroundSubstraction *m_opt;
  };

  class SoftOpRoiCounter : public SoftOpBaseClass
  {
  public:
    typedef std::pair<int,std::list<Tasks::RoiCounterResult> > RoiIdAndResults;
    typedef std::pair<int,Roi> RoiIdAndRoi;
    SoftOpRoiCounter();
    ~SoftOpRoiCounter();

    void add(const std::list<Roi> &rois); 
    void set(const std::list<Roi> &rois); 
    void get(std::list<Roi>&) const;
    void del(const std::list<int> &roiIds);
    void clearAllRoi();		/* clear all roi */
    
    void setMask(Data &aMask);

    void setBufferSize(int size);
    void getBufferSize(int &size) const;
    
    void readCounters(int from,std::list<RoiIdAndResults> &result) const;
  protected:
    virtual void addTo(TaskMgr&,int stage);
    virtual void prepare();
  private:
    typedef std::pair<Tasks::RoiCounterManager*,Tasks::RoiCounterTask*> ManagerNCounter;

    std::list<ManagerNCounter>  m_manager_tasks;
    int				m_history_size;
    Data			m_mask;
  };

}
#endif
