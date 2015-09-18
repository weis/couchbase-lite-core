﻿//
//  C4Test.cs
//
//  Author:
//  	Jim Borden  <jim.borden@couchbase.com>
//
//  Copyright (c) 2015 Couchbase, Inc All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
using System;
using NUnit.Framework;
using System.IO;


namespace CBForest.Tests
{
    public unsafe class C4Test
    {
        protected const string BODY = "{\"name\":007}";
        protected const string DOC_ID = "mydoc";
        protected const string REV_ID = "1-abcdef";
        
        protected C4Database *_db;
        
        [SetUp]
        public virtual void SetUp()
        {
            Directory.CreateDirectory(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData));
            var dbPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "forest_temp.fdb");
            C4Error error;
            _db = Native.c4db_open(dbPath, false, &error);
            Assert.IsFalse(_db == null);
        }
        
        [TearDown]
        public virtual void TearDown()
        {
            C4Error error;
            Assert.IsTrue(Native.c4db_delete(_db, &error));
        }
        
        public string ToJSON(C4KeyReader r)
        {
            return Native.c4key_toJSON(&r);
        }
        
        public string ToJSON(C4Key *key)
        {
            return ToJSON(Native.c4key_read(key));   
        }
        
        protected void CreateRev(string docID, string revID, string body)
        {
            using(var t = new TransactionHelper(_db)) {
                C4Error error;
                var doc = Native.c4doc_get(_db, docID, false, &error);
                Assert.IsTrue(doc != null);
                Assert.IsTrue(Native.c4doc_insertRevision(doc, revID, body, false, false, false, &error));
                Assert.IsTrue(Native.c4doc_save(doc, 20, &error));
                Native.c4doc_free(doc);
            }
        }
    }
}

