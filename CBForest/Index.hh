//
//  Index.hh
//  CBForest
//
//  Created by Jens Alfke on 5/14/14.
//  Copyright (c) 2014 Couchbase. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software distributed under the
//  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
//  either express or implied. See the License for the specific language governing permissions
//  and limitations under the License.

#ifndef __CBForest__Index__
#define __CBForest__Index__

#include "DocEnumerator.hh"
#include "Collatable.hh"

namespace forestdb {
    
    class Index;

    struct KeyRange {
        Collatable start;
        Collatable end;
        bool inclusiveEnd;

        KeyRange(Collatable s, Collatable e, bool inclusive =true)
                                                :start(s), end(e), inclusiveEnd(inclusive) { }
        KeyRange(Collatable single)             :start(single), end(single), inclusiveEnd(true) { }
        KeyRange(const KeyRange &r)             :start(r.start), end(r.end),
                                                 inclusiveEnd(r.inclusiveEnd) { }
        bool isKeyPastEnd(slice key) const;
    };

    
    /** A key-value store used as an index. */
    class Index : protected KeyStore {
    public:
        Index(Database*, std::string name);

        alloc_slice getEntry(slice docID, sequence docSequence,
                             Collatable key,
                             unsigned emitIndex);

    private:
        friend class IndexWriter;
        friend class IndexEnumerator;
    };


    /** A transaction to update an index. */
    class IndexWriter : protected KeyStoreWriter {
    public:
        IndexWriter(Index* index, Transaction& t);

        /** Updates the index entry for a document with the given keys and values.
            Adjusts the value of rowCount by the number of rows added or removed.
            Returns true if the index may have changed as a result. */
        bool update(slice docID,
                    sequence docSequence,
                    std::vector<Collatable> keys,
                    std::vector<Collatable> values,
                    uint64_t &rowCount);

    private:
        int64_t removeOldRowsForDoc(slice docID);

        friend class Index;
        friend class MapReduceIndex;
    };


    /** Index query enumerator. */
    class IndexEnumerator {
    public:
        IndexEnumerator(Index*,
                        Collatable startKey, slice startKeyDocID,
                        Collatable endKey, slice endKeyDocID,
                        const DocEnumerator::Options&);

        IndexEnumerator(Index*,
                        std::vector<KeyRange> keyRanges,
                        const DocEnumerator::Options&);

        CollatableReader key() const            {return CollatableReader(_key);}
        CollatableReader value() const          {return CollatableReader(_value);}
        slice docID() const                     {return _docID;}
        sequence sequence() const               {return _sequence;}

        bool next();

    protected:
        virtual bool approve(slice key)         {return true;}
        bool read();

    private:
        friend class Index;
        void nextKeyRange();

        Index* _index;
        DocEnumerator::Options _options;
        alloc_slice _startKey;
        alloc_slice _endKey;
        bool _inclusiveStart;
        bool _inclusiveEnd;
        std::vector<KeyRange> _keyRanges;
        int _currentKeyIndex;

        DocEnumerator _dbEnum;
        slice _key;
        slice _value;
        alloc_slice _docID;
        ::forestdb::sequence _sequence;
    };

}

#endif /* defined(__CBForest__Index__) */
