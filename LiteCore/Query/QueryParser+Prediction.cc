//
// QuerParser+Prediction.cc
//
// Copyright © 2018 Couchbase. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "QueryParser.hh"
#include "QueryParser+Private.hh"
#include "FleeceImpl.hh"
#include "StringUtil.hh"
#include "SecureDigest.hh"

#ifdef COUCHBASE_ENTERPRISE

using namespace std;
using namespace fleece;
using namespace fleece::impl;
using namespace litecore::qp;

namespace litecore {


    // Scans the entire query for PREDICTION() calls and adds join tables for ones that are indexed.
    void QueryParser::findPredictionCalls(const Value *root) {
        findNodes(root, kPredictionFnNameWithParens, 1, [this](const Array *pred) {
            predictiveJoinTableAlias(pred, true);
        });
    }


    // Looks up or adds a join alias for a predictive index table.
    const string& QueryParser::predictiveJoinTableAlias(const Value *predictionExpr, bool canAdd) {
        string table = predictiveTableName(predictionExpr);
        if (canAdd && !_delegate.tableExists(table))
            canAdd = false; // not indexed
        return indexJoinTableAlias(table, (canAdd ? "pred" : nullptr));
    }


    // Constructs a unique identifier of a specific PREDICTION() call, from a digest of its JSON.
    string QueryParser::predictiveIdentifier(const Value *expression) const {
        Array::iterator array(expression->asArray());
        if (array.count() < 2 || !array[0]->asString().caseEquivalent(kPredictionFnNameWithParens))
            fail("Invalid PREDICTION() call");
        alloc_slice name = array[1]->toJSON(true);
        alloc_slice input = array[2]->toJSON(true);

        uint8_t digest[20];
        sha1Context ctx;
        sha1_begin(&ctx);
        sha1_add(&ctx, name.buf, name.size);
        sha1_add(&ctx, input.buf, input.size);
        sha1_end(&ctx, &digest);
        return slice(&digest, sizeof(digest)).base64String();
    }


    // Returns the name of the index table for a PREDICTION() call expression.
    string QueryParser::predictiveTableName(const Value *expression) const {
        return _delegate.predictiveTableName(predictiveIdentifier(expression));
    }


    bool QueryParser::writeIndexedPrediction(const Array *node) {
        auto alias = predictiveJoinTableAlias(node);
        if (alias.empty())
            return false;
        if (node->count() >= 4) {
            slice property = requiredString(node->get(3), "PREDICTION() property name");
            _sql << kValueFnName << "(" << alias << ".body, ";
            writeSQLString(_sql, propertyFromString(property));
            _sql << ")";
        } else {
            _sql << kRootFnName << "(" << alias << ".body)";
        }
        return true;
    }

}

#endif // COUCHBASE_ENTERPRISE