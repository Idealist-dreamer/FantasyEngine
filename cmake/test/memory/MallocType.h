#pragma once

// A sample class to test constructor and alignment
class MockObject {
 public:
  MockObject(int a = 0, float b = 0) : a_(a), b_(b) { constructed_count++; }
  ~MockObject() { constructed_count--; }

  int a_;
  float b_;
  static int constructed_count;
};
int MockObject::constructed_count = 0;

// A class with strict alignment requirements
struct alignas(64) AlignedObject {
  float data[16];
};

TEST(TestMiMalloc, TypeAllocators) {
  struct SimpleStruct {
    int x;
    int y;
  };

  // 1. Test mallocType
  SimpleStruct* p1 = Alloc::mallocType<SimpleStruct>();
  ASSERT_NE(p1, nullptr);
  p1->x = 100;
  EXPECT_EQ(p1->x, 100);
  Alloc::free(p1);

  // 2. Test zallocType
  SimpleStruct* p2 = Alloc::zallocType<SimpleStruct>();
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(p2->x, 0);
  EXPECT_EQ(p2->y, 0);
  Alloc::free(p2);
}

TEST(TestMiMalloc, CreateAndDestroyObject) {
  MockObject::constructed_count = 0;

  // 1. Test create with arguments
  MockObject* obj = Alloc::create<MockObject>(42, 3.14f);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->a_, 42);
  EXPECT_FLOAT_EQ(obj->b_, 3.14f);
  EXPECT_EQ(MockObject::constructed_count, 1);

  // 2. Test destroy (should call destructor and free memory)
  Alloc::destroy(obj);
  EXPECT_EQ(MockObject::constructed_count, 0);

  // 3. Test create with high alignment requirement
  AlignedObject* aObj = Alloc::create<AlignedObject>();
  ASSERT_NE(aObj, nullptr);
  EXPECT_TRUE(IsAligned(aObj, 64));

  // 4. Test destroy for aligned object
  Alloc::destroy(aObj);
}

TEST(TestMiMalloc, CreateVariadicArgs) {
  struct VarArgs {
    VarArgs() : val(0) {}
    VarArgs(int v) : val(v) {}
    int val;
  };

  // Default constructor
  VarArgs* v1 = Alloc::create<VarArgs>();
  EXPECT_EQ(v1->val, 0);
  Alloc::destroy(v1);

  // Parameterized constructor
  VarArgs* v2 = Alloc::create<VarArgs>(123);
  EXPECT_EQ(v2->val, 123);
  Alloc::destroy(v2);
}

TEST(TestMiMalloc, DestroyNull) {
  // Should handle nullptr safely without crash
  MockObject* pNull = nullptr;
  EXPECT_NO_THROW(Alloc::destroy(pNull));
}