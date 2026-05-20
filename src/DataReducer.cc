#include "DataReducer.hh"

#include "G4VAnalysisManager.hh"

namespace QArray
{
  DataReducer::DataReducer() : reducelvl(HitData::kFull) {}

  DataReducer::DataReducer(const std::vector<DataMapper> &vecMappers) : reducelvl(HitData::kMiminal)
  {
    for (const auto &mapping : vecMappers)
    {
      AddMapping(mapping);
    }
  }

  int DataReducer::AddMapping(const DataMapper &mapper)
  {
    vecMappers.push_back(mapper);
    return vecMappers.size();
  }

  // Writes the data in `hits` to the ntuple with id `ntupleid`
  // Returns the number of hits written
  int DataReducer::Write(G4VAnalysisManager *AMan, int ntupleid,
                         QRHitsCollection *hits, bool bSort) const
  {
    auto &hitvec = *(hits->GetVector());
    if (hitvec.empty()) // no-op
      return 0;
    int nwritten = 0;
    if (vecMappers.empty())
    {
      // this is special case, first sort, then write every step
      std::sort(hitvec.begin(), hitvec.end());
      for (HitData *hit : hitvec)
      {
        hit->FillNtuple(AMan, ntupleid, reducelvl);
      }
      nwritten = hitvec.size();
    }
    else
    {
      // first we need to do the map-reduce
      std::map<std::vector<double>, HitData> hitmap;
      for (HitData *hit : hitvec)
      {
        std::vector<double> mappedVal(vecMappers.size());
        std::transform(vecMappers.begin(), vecMappers.end(), mappedVal.begin(),
                       [hit](const DataMapper &m)
                       { return m(hit); });
        hitmap[std::move(mappedVal)] += *hit;
      }
      // now write out the reduced hits

      // Sort the hitmap by its value
      if (bSort)
      {
        std::vector<std::pair<std::vector<double>, HitData>> hitmapvec(hitmap.begin(), hitmap.end());
        std::sort(hitmapvec.begin(), hitmapvec.end(),
                  [](const std::pair<std::vector<double>, HitData> &h1,
                     const std::pair<std::vector<double>, HitData> &h2)
                  { return h1.second < h2.second; });
        for (auto &mhit : hitmapvec)
        {
          mhit.second.FillNtuple(AMan, ntupleid, reducelvl);
        }
        nwritten = hitmap.size();
      }
      else
      {
        for (auto &mhit : hitmap)
        {
          mhit.second.FillNtuple(AMan, ntupleid, reducelvl);
        }
        nwritten = hitmap.size();
      }
    }
    return nwritten;
  }
}