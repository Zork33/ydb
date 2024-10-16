#include "../mkql_match_recognize_matched_vars.h"
#include <library/cpp/testing/unittest/registar.h>

namespace NKikimr::NMiniKQL::NMatchRecognize {

Y_UNIT_TEST_SUITE(MatchRecognizeMatchedVars) {
    TMemoryUsageInfo memUsage("MatchedVars");
    Y_UNIT_TEST(MatchedRange) {
        TScopedAlloc alloc(__LOCATION__);
        THolderFactory holderFactory(alloc.Ref(), memUsage);
        {
            TMatchedRange r{10, 20};
            const auto value = ToValue(holderFactory, r);
            const auto elems = value.GetElements();
            UNIT_ASSERT(elems);
            UNIT_ASSERT_VALUES_EQUAL(10, elems[0].Get<ui64>());
            UNIT_ASSERT_VALUES_EQUAL(20, elems[1].Get<ui64>());
        }
    }

    Y_UNIT_TEST(MatchedRangeListEmpty) {
        TScopedAlloc alloc(__LOCATION__);
        THolderFactory holderFactory(alloc.Ref(), memUsage);
        {
            const auto value = ToValue(holderFactory, std::vector<TMatchedRange>{});
            UNIT_ASSERT(value);
            UNIT_ASSERT(!value.HasListItems());
            UNIT_ASSERT(value.HasFastListLength());
            UNIT_ASSERT_VALUES_EQUAL(0, value.GetListLength());
            const auto iter = value.GetListIterator();
            UNIT_ASSERT(iter);
            NUdf::TUnboxedValue noValue;
            UNIT_ASSERT(!iter.Next(noValue));
            UNIT_ASSERT(!noValue);
        }
    }

    Y_UNIT_TEST(MatchedRangeList) {
        TScopedAlloc alloc(__LOCATION__);
        THolderFactory holderFactory(alloc.Ref(), memUsage);
        {
            const auto value = ToValue(holderFactory, {
                TMatchedRange{10, 30},
                TMatchedRange{40, 45},

            });
            UNIT_ASSERT(value);
            UNIT_ASSERT(value.HasListItems());
            UNIT_ASSERT(value.HasFastListLength());
            UNIT_ASSERT_VALUES_EQUAL(2, value.GetListLength());
            const auto iter = value.GetListIterator();
            UNIT_ASSERT(iter);
            NUdf::TUnboxedValue elem;
            //[0]
            UNIT_ASSERT(iter.Next(elem));
            UNIT_ASSERT(elem);
            UNIT_ASSERT(elem.GetElements());
            UNIT_ASSERT_VALUES_EQUAL(30, elem.GetElements()[1].Get<ui64>());
            //[1]
            UNIT_ASSERT(iter.Next(elem));
            UNIT_ASSERT(elem);
            UNIT_ASSERT(elem.GetElements());
            UNIT_ASSERT_VALUES_EQUAL(40, elem.GetElements()[0].Get<ui64>());
            //
            UNIT_ASSERT(!iter.Next(elem));
        }
    }

    Y_UNIT_TEST(MatchedVars) {
        TScopedAlloc alloc(__LOCATION__);
        THolderFactory holderFactory(alloc.Ref(), memUsage);
        {
            const auto value = ToValue(holderFactory, {
                    {},
                    {TMatchedRange{20, 25}},
                    {TMatchedRange{10, 30}, TMatchedRange{40, 45}},
            });
            UNIT_ASSERT(value);
            const auto varElems = value.GetElements();
            UNIT_ASSERT(varElems);
            UNIT_ASSERT(varElems[0]);
            UNIT_ASSERT(varElems[1]);
            UNIT_ASSERT(varElems[2]);
            const auto lastVar = varElems[2];
            UNIT_ASSERT(lastVar.HasFastListLength());
            UNIT_ASSERT_VALUES_EQUAL(2, lastVar.GetListLength());
            const auto iter = lastVar.GetListIterator();
            UNIT_ASSERT(iter);
            NUdf::TUnboxedValue elem;
            //[0]
            UNIT_ASSERT(iter.Next(elem));
            UNIT_ASSERT(elem);
            UNIT_ASSERT(elem.GetElements());
            UNIT_ASSERT_VALUES_EQUAL(30, elem.GetElements()[1].Get<ui64>());
            //[1]
            UNIT_ASSERT(iter.Next(elem));
            UNIT_ASSERT(elem);
            UNIT_ASSERT(elem.GetElements());
            UNIT_ASSERT_VALUES_EQUAL(40, elem.GetElements()[0].Get<ui64>());
            //
            UNIT_ASSERT(!iter.Next(elem));
        }
    }
}

Y_UNIT_TEST_SUITE(MatchRecognizeMatchedVarsByRef) {
        TMemoryUsageInfo memUsage("MatchedVarsByRef");
        Y_UNIT_TEST(MatchedVarsEmpty) {
            TScopedAlloc alloc(__LOCATION__);
            {
                TMatchedVars vars{};
                NUdf::TUnboxedValue value(NUdf::TUnboxedValuePod(new TMatchedVarsValue(&memUsage, vars)));
                UNIT_ASSERT(value.HasValue());
            }
        }
        Y_UNIT_TEST(MatchedVars) {
            TScopedAlloc alloc(__LOCATION__);
            {
                TMatchedVar A{{1, 4}, {7, 9}, {100, 200}};
                TMatchedVar B{{1, 6}};
                TMatchedVars vars{A, B};
                NUdf::TUnboxedValue value(NUdf::TUnboxedValuePod(new TMatchedVarsValue(&memUsage, vars)));
                UNIT_ASSERT(value.HasValue());
                auto a = value.GetElement(0);
                UNIT_ASSERT(a.HasValue());
                UNIT_ASSERT_VALUES_EQUAL(3, a.GetListLength());
                auto iter = a.GetListIterator();
                UNIT_ASSERT(iter.HasValue());
                NUdf::TUnboxedValue last;
                while (iter.Next(last))
                    ;
                UNIT_ASSERT(last.HasValue());
                UNIT_ASSERT_VALUES_EQUAL(100, last.GetElement(0).Get<ui64>());
                UNIT_ASSERT_VALUES_EQUAL(200, last.GetElement(1).Get<ui64>());
                auto b = value.GetElement(1);
                UNIT_ASSERT(b.HasValue());
                UNIT_ASSERT_VALUES_EQUAL(1, b.GetListLength());
            }
        }
}
}//namespace NKikimr::NMiniKQL::TMatchRecognize
