#ifndef QRDataReducer_h
#define QRDataReducer_h 1

/** @class DataReducer
    This class provides a way to reduce the data in a HitCollection.
*/

#include "HitData.hh"

#include <functional>
#include <string>
#include <vector>
#include <map>

class G4VAnalsisManager;

namespace QArray
{
  typedef G4THitsCollection<HitData> QRHitsCollection;

  struct DataMapper
  {
    // Define a map function as a function that takes in a HitData pointer and returns a double
    typedef std::function<double(HitData *)> MapFunc;

    MapFunc func;

    inline double operator()(HitData *hit) const { return func(hit); }

    DataMapper(MapFunc Func) : func(Func) {}
  };

  struct DataReducer
  {
    DataReducer();
    DataReducer(const std::vector<DataMapper> &vecMappers);

    std::vector<DataMapper> vecMappers;
    // Reducer needs to know the level of information to be saved
    HitData::REDUCELEVEL reducelvl;

    /// Add a mapping to be combined with all others, returning the total size of the mapping vector
    int AddMapping(const DataMapper &mapper);

    /// Write out the reduced data in `hits`
    int Write(G4VAnalysisManager *AMan, int iNtupleID,
              QRHitsCollection *hits, bool bSort) const;
  };
}

#endif
