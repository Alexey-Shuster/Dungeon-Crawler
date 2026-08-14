#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <cbor.h>
#include <common/serialization.h>
#include <cstdint>
#include <memory>
#include <vector>

using namespace network;

struct CborTestDeleter {
    void operator()(cbor_item_t* item) const {
        if (item) {
            cbor_decref(&item);
        }
    }
};

using CborTestPtr = std::unique_ptr<cbor_item_t, CborTestDeleter>;

// ============================================================================
// Тесты для addArgToArray (без изменений)
// ============================================================================

class AddArgToArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        array_.reset(cbor_new_definite_array(1));
        ASSERT_NE(array_, nullptr);
    }

    CborTestPtr array_;
};

TEST_F(AddArgToArrayTest, AddUint64Value) {
    EXPECT_TRUE(addArgToArray(array_.get(), 12345));

    EXPECT_EQ(cbor_array_size(array_.get()), 1);

    CborTestPtr item(cbor_array_get(array_.get(), 0));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_uint(item.get()));
    EXPECT_EQ(cbor_get_uint64(item.get()), 12345);
}

TEST_F(AddArgToArrayTest, AddZeroValue) {
    EXPECT_TRUE(addArgToArray(array_.get(), 0));

    CborTestPtr item(cbor_array_get(array_.get(), 0));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_uint(item.get()));
    EXPECT_EQ(cbor_get_uint64(item.get()), 0);
}

TEST_F(AddArgToArrayTest, AddMaxUint64) {
    EXPECT_TRUE(addArgToArray(array_.get(), UINT64_MAX));

    CborTestPtr item(cbor_array_get(array_.get(), 0));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_uint(item.get()));
    EXPECT_EQ(cbor_get_uint64(item.get()), UINT64_MAX);
}

TEST_F(AddArgToArrayTest, MultipleValues) {
    array_.reset(cbor_new_definite_array(3));
    ASSERT_NE(array_, nullptr);

    EXPECT_TRUE(addArgToArray(array_.get(), 100));
    EXPECT_TRUE(addArgToArray(array_.get(), 200));
    EXPECT_TRUE(addArgToArray(array_.get(), 300));

    EXPECT_EQ(cbor_array_size(array_.get()), 3);

    for (size_t i = 0; i < 3; ++i) {
        CborTestPtr item(cbor_array_get(array_.get(), i));
        ASSERT_NE(item, nullptr);
        EXPECT_TRUE(cbor_isa_uint(item.get()));
        EXPECT_EQ(cbor_get_uint64(item.get()), (i + 1) * 100);
    }
}

// ============================================================================
// Тесты для getMsgArgsFromCborArray (без изменений)
// ============================================================================

class GetMsgArgsFromCborArrayTest : public ::testing::Test {
protected:
    // Создаёт массив, где первый элемент — тип (uint8_t, но мы храним его как uint16_t?
    // В наших новых тестах мы используем uint16_t, но в старых тестах мы использовали uint8_t.
    // Однако getMsgArgsFromCborArray не смотрит на тип, он просто пропускает первый элемент.
    // Поэтому мы можем оставить создание с uint8_t для совместимости.
    CborTestPtr createDefiniteArray(uint16_t msg_type, const std::vector<uint64_t>& args = {}) {
        size_t total_size = 1 + args.size();
        CborTestPtr array(cbor_new_definite_array(total_size));
        if (!array) {
            return nullptr;
        }

        // Используем cbor_build_uint16, потому что теперь тип хранится как uint16_t
        CborTestPtr type_item(cbor_build_uint16(msg_type));
        if (!type_item) {
            return nullptr;
        }
        cbor_incref(type_item.get());
        cbor_array_set(array.get(), 0, type_item.get());

        for (size_t i = 0; i < args.size(); ++i) {
            CborTestPtr arg_item(cbor_build_uint64(args[i]));
            if (!arg_item) {
                return nullptr;
            }
            cbor_incref(arg_item.get());
            cbor_array_set(array.get(), 1 + i, arg_item.get());
        }

        return array;
    }
};

TEST_F(GetMsgArgsFromCborArrayTest, ArrayWithOnlyType) {
    auto array = createDefiniteArray(0);
    ASSERT_NE(array, nullptr);

    auto args = getMsgArgsFromCborArray(array.get());

    EXPECT_TRUE(args.has_value());
    EXPECT_TRUE(args->empty());
}

TEST_F(GetMsgArgsFromCborArrayTest, ValidArray) {
    auto array = createDefiniteArray(0, {123, 456});
    ASSERT_NE(array, nullptr);

    auto args = getMsgArgsFromCborArray(array.get());

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->size(), 2);
    EXPECT_EQ((*args)[0], 123);
    EXPECT_EQ((*args)[1], 456);
}

TEST_F(GetMsgArgsFromCborArrayTest, LargeUint64Values) {
    auto array = createDefiniteArray(0, {UINT64_MAX, 0x0123456789ABCDEF});
    ASSERT_NE(array, nullptr);

    auto args = getMsgArgsFromCborArray(array.get());

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->size(), 2);
    EXPECT_EQ((*args)[0], UINT64_MAX);
    EXPECT_EQ((*args)[1], 0x0123456789ABCDEF);
}

TEST_F(GetMsgArgsFromCborArrayTest, NullArrayReturnsNullopt) {
    auto args = getMsgArgsFromCborArray(nullptr);
    EXPECT_FALSE(args.has_value());
}

TEST_F(GetMsgArgsFromCborArrayTest, NonArrayReturnsNullopt) {
    CborTestPtr item(cbor_build_uint8(42));
    auto args = getMsgArgsFromCborArray(item.get());
    EXPECT_FALSE(args.has_value());
}

TEST_F(GetMsgArgsFromCborArrayTest, EmptyCborArrayReturnsNullopt) {
    CborTestPtr empty_array(cbor_new_definite_array(0));
    ASSERT_NE(empty_array, nullptr);

    auto args = getMsgArgsFromCborArray(empty_array.get());
    EXPECT_FALSE(args.has_value());
}
