#include "SoftOpExternalMgr.h"
using namespace lima;

static SoftOpKey SoftOpTable[] = {
  SoftOpKey(BACKGROUNDSUBSTRACTION,"Background substraction"),
  //{BINNING,"Binning"},
  //{BPM,"Bpm"},
  //{FLATFIELDCORRECTION,"Flat field correction"},
  //{FLIP,"Flip"},
  //{MASK,"Mask"},
  SoftOpKey(ROICOUNTERS,"Roi counters"),
  //{SOFTROI,"Software roi"},
  SoftOpKey()
};

static SoftOpKey getSoftOpKey(SoftOpId anId)
{
  for(unsigned int i = 0;i < sizeof(SoftOpTable);++i)
    {
      if(SoftOpTable[i].m_id == anId)
	return SoftOpTable[i];
    }
  return SoftOpKey();
}

SoftOpExternalMgr::SoftOpExternalMgr() :
  m_end_link_callback(NULL),
  m_end_sink_callback(NULL)
{
}

SoftOpExternalMgr::~SoftOpExternalMgr()
{
  if(m_end_link_callback)
    m_end_link_callback->unref();
  if(m_end_sink_callback)
    m_end_sink_callback->unref();

  for(Stage2Instance::iterator i = m_stage2instance.begin();
      i != m_stage2instance.end();++i)
    for(std::list<SoftOpInstance>::iterator k = i->second.begin();
	k != i->second.end();k = i->second.erase(k))
      delete k->m_opt;
}

void SoftOpExternalMgr::getAvailableOp(const SoftOpKey* &available) const
{
  available = SoftOpTable;
}

void SoftOpExternalMgr::getActiveOp(std::map<stage,std::list<alias> > &activeOp) const
{
  for(Stage2Instance::const_iterator i = m_stage2instance.begin();
      i != m_stage2instance.end();++i)
    {
      std::pair<std::map<stage,std::list<alias> >::iterator,bool> result =
	activeOp.insert(std::pair<stage,std::list<alias> >(i->first,std::list<alias>()));
      std::list<alias> &aliasList = result.first->second;

      for(std::list<SoftOpInstance>::const_iterator k = i->second.begin();
	  k != i->second.end();++k)
	aliasList.push_back(k->m_alias);
    }
}

void SoftOpExternalMgr::getActiveStageOp(stage aStage,std::list<alias> &activeOp) const
{
  Stage2Instance::const_iterator i = m_stage2instance.find(aStage);
  if(i != m_stage2instance.end())
    for(std::list<SoftOpInstance>::const_iterator k = i->second.begin();
	  k != i->second.end();++k)
      activeOp.push_back(k->m_alias);
}

void SoftOpExternalMgr::addOp(SoftOpId aSoftOpId,
			      const alias &anAlias,int aStage)
{
  _checkIfPossible(aSoftOpId,aStage);
  SoftOpInstance newInstance(getSoftOpKey(aSoftOpId),anAlias);
  
  switch(aSoftOpId)
    {
    case ROICOUNTERS:
      newInstance.m_opt = new SoftOpRoiCounter();
      break;
    default:
      throw LIMA_CTL_EXC(InvalidValue,"Not yet managed");
    }
  std::pair<Stage2Instance::iterator,bool> aResult = 
    m_stage2instance.insert(std::pair<stage,std::list<SoftOpInstance> >(aStage,std::list<SoftOpInstance>()));
  aResult.first->second.push_back(newInstance);
}

void SoftOpExternalMgr::delOp(const alias &anAlias)
{
   for(Stage2Instance::iterator i = m_stage2instance.begin();
      i != m_stage2instance.end();++i)
    {
      for(std::list<SoftOpInstance>::iterator k = i->second.begin();
	  k != i->second.end();++k)
	{
	  if(k->m_alias == anAlias)
	    {
	      delete k->m_opt;
	      i->second.erase(k);
	      if(i->second.empty())
		m_stage2instance.erase(i);
	      return;
	    }
	}
    }
}

void SoftOpExternalMgr::getOpClass(const alias &anAlias,
				   SoftOpInstance &aSoftOpInstance) const
{
  for(Stage2Instance::const_iterator i = m_stage2instance.begin();
      i != m_stage2instance.end();++i)
    {
      for(std::list<SoftOpInstance>::const_iterator k = i->second.begin();
	  k != i->second.end();++k)
	{
	  if(k->m_alias == anAlias)
	    {
	      aSoftOpInstance = *k;
	      return;
	    }
	}
    }
}

void SoftOpExternalMgr::setEndLinkTaskCallback(TaskEventCallback *aCbk)
{
  if(m_end_link_callback)
    m_end_link_callback->unref();
  m_end_link_callback = aCbk;
  if(m_end_link_callback)
    m_end_link_callback->ref();
}

void SoftOpExternalMgr::setEndSinkTaskCallback(TaskEventCallback *aCbk)
{
  if(m_end_sink_callback)
    m_end_sink_callback->unref();
  m_end_sink_callback = aCbk;
  if(m_end_sink_callback)
    m_end_sink_callback->ref();
}

void SoftOpExternalMgr::addTo(TaskMgr &aTaskMgr,
			      int begin_stage,
			      int &last_link_task,int &last_sink_task)
{
  last_link_task = last_sink_task = begin_stage;
  int nextStage = begin_stage + 1;
  for(Stage2Instance::iterator i = m_stage2instance.begin();
      i != m_stage2instance.end();++i,++nextStage)
    {
      for(std::list<SoftOpInstance>::const_iterator k = i->second.begin();
	  k != i->second.end();++k)
	{
	  if(k->m_linkable)
	    last_link_task = nextStage;
	  else
	    last_sink_task = nextStage;
	  k->m_opt->addTo(aTaskMgr,nextStage);
	}
    }
  std::pair<int,LinkTask*> aLastLink(0,NULL);
  std::pair<int,SinkTaskBase*> aLastSink(0,NULL);

  aTaskMgr.getLastTask(aLastLink,aLastSink);

  if(aLastLink.first > begin_stage)
    aLastLink.second->setEventCallback(m_end_link_callback);
  if(aLastSink.first > begin_stage)
    aLastSink.second->setEventCallback(m_end_sink_callback);
}

void SoftOpExternalMgr::_checkIfPossible(SoftOpId aSoftOpId,
					 int stage)
{
  bool checkLinkable = false;
  switch(aSoftOpId) 
    {
    case ROICOUNTERS:
    case BPM:
      break;			// always possible
    default:
      throw LIMA_CTL_EXC(InvalidValue,"Not yet managed");
    }

  if(checkLinkable)
    {
      //todo
    }
}

void SoftOpExternalMgr::isTaskActive(bool &linkTaskFlag,bool &sinkTaskFlag) const
{
  linkTaskFlag = sinkTaskFlag = false;
  for(Stage2Instance::const_iterator i = m_stage2instance.begin();
      i != m_stage2instance.end() && (!linkTaskFlag || !sinkTaskFlag);++i)
    {
      for(std::list<SoftOpInstance>::const_iterator k = i->second.begin();
	  k != i->second.end() && (!linkTaskFlag || !sinkTaskFlag);++k)
	{
	 if(k->m_linkable)
	    linkTaskFlag = true;
	  else
	    sinkTaskFlag = true;
	}
    }
}


void SoftOpExternalMgr::prepare()
{
  for(Stage2Instance::iterator i = m_stage2instance.begin();
      i != m_stage2instance.end();++i)
    for(std::list<SoftOpInstance>::iterator k = i->second.begin();
	k != i->second.end();++k)
      k->m_opt->prepare();
}
