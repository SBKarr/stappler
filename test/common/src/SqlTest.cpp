// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPCommon.h"
#include "SPSql.h"
#include "Test.h"

NS_SP_BEGIN

struct SqlTest : Test {
	SqlTest() : Test("SqlTest") { }

	virtual bool run() override {
		using namespace sql;
		using Field = Query<SimpleBinder>::Field;

		StringStream stream;
		Query<SimpleBinder> query;
		query.select()
				.fields("field", Field("field").as("alias")).field(Field("database", "field").as("alias"))
				.from("table").from("table")
				.where("alias", sql::Comparation::Equal, "value")
				.where(sql::Operator::And, "field", sql::Comparation::Equal, data::Value(1234))
				.where(sql::Operator::Or, "field", sql::Comparation::NotEqual, data::Value(false))
				.where(sql::Operator::Or, "time", sql::Comparation::BetweenValues, data::Value(1234), data::Value(123400))
				.order(sql::Ordering::Descending, "field")
				.limit(12, 16)
				.finalize();
		stream << query.getStream().str() << "\n";

		query = sql::Query<sql::SimpleBinder>();
		query.with("query", [] (sql::Query<sql::SimpleBinder>::GenericQuery &q) {
			q.select().all().from("sqtable");
		}).with("query", [] (sql::Query<sql::SimpleBinder>::GenericQuery &q) {
			q.select().all().from("sqtable");
		}).select().fields("field1", "field2").from("table")
			.where("alias", sql::Comparation::Equal, "value")
			.parenthesis(sql::Operator::And, [] (sql::Query<sql::SimpleBinder>::WhereBegin &q) {
				q.where("field", sql::Comparation::Equal, "value")
						.where(sql::Operator::Or, "field", sql::Comparation::Equal, "value");
		}).finalize();

		stream << query.getStream().str() << "\n";

		sql::Query<sql::SimpleBinder> insertQuery;
		insertQuery.insert("table")
				.field("field1").field("field2")
				.values().value("test1").value("test2")
				.values().value("test3").value("test4")
				.finalize();

		stream << insertQuery.getStream().str() << "\n";

		insertQuery = sql::Query<sql::SimpleBinder>();
		insertQuery.insert("table")
				.field("field1").field("field2")
				.values().value("test1").value("test2")
				.onConflictDoNothing()
				.returning().all()
				.finalize();
		stream << insertQuery.getStream().str() << "\n";

		insertQuery = sql::Query<sql::SimpleBinder>();
		insertQuery.insert("table")
				.field("field1").field("field2").field("field3")
				.values().value("test1").value("test2").value("test3")
				.onConflict("field1").doUpdate().excluded("field2").def("field3")
				.where("field1", sql::Comparation::Equal, "alias")
				.returning().all()
				.finalize();
		stream << insertQuery.getStream().str() << "\n";

		sql::Query<sql::SimpleBinder> updateQuery;
		updateQuery.update("table", "alias")
				.set("field1", "value1").set("field1", "value2")
				.where("field3", sql::Comparation::NotEqual, data::Value(false))
				.returning().all()
				.finalize();
		stream << updateQuery.getStream().str() << "\n";

		sql::Query<sql::SimpleBinder> deleteQuery;
		deleteQuery.remove("table", "alias")
				.where("field3", sql::Comparation::NotEqual, data::Value(false))
				.returning().all()
				.finalize();
		stream << deleteQuery.getStream().str() << "\n";

		_desc = stream.str();

		return true;
	}

} _SqlTest;

NS_SP_END
