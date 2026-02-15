#pragma once

using Alloc = Allocator<AllocType::MiMalloc>;

// A class with strict alignment requirements
struct alignas(64) AlignedObject {
  float data[16];
};

static bool IsAligned(void* p, size_t alignment) {
  return reinterpret_cast<uintptr_t>(p) % alignment == 0;
}

static bool IsAlignedAt(void* p, size_t alignment, size_t offset) {
  return (reinterpret_cast<uintptr_t>(p) + offset) % alignment == 0;
}

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

TEST(TestMiMalloc, SmartPointers) {
  MockObject::constructed_count = 0;

  // 1. Test make_unique
  {
    auto uptr = make_unique<MockObject>(10, 2.5f);
    ASSERT_NE(uptr, nullptr);
    EXPECT_EQ(uptr->a_, 10);
    EXPECT_EQ(MockObject::constructed_count, 1);

    // Memory and destructor should be handled automatically when uptr goes out of scope
  }
  EXPECT_EQ(MockObject::constructed_count, 0);

  // 2. Test make_shared
  {
    auto sptr1 = make_shared<MockObject>(20, 5.5f);
    ASSERT_NE(sptr1, nullptr);
    EXPECT_EQ(MockObject::constructed_count, 1);

    {
      auto sptr2 = sptr1;
      EXPECT_EQ(MockObject::constructed_count, 1);
      EXPECT_EQ(sptr2->a_, 20);
    }

    EXPECT_EQ(MockObject::constructed_count, 1);
  }
  EXPECT_EQ(MockObject::constructed_count, 0);
}

TEST(TestMiMalloc, SmartPointerAlignment) {
  // Test if make_unique/make_shared handle aligned objects correctly

  // unique_ptr
  auto uptr = make_unique<AlignedObject>();
  EXPECT_TRUE(IsAligned(uptr.get(), 64));
  uptr.reset();

  // shared_ptr
  auto sptr = make_shared<AlignedObject>();
  EXPECT_TRUE(IsAligned(sptr.get(), 64));
  sptr.reset();
}

TEST(TestMiMalloc, UniquePtrCustomDeleter) {
  MockObject::constructed_count = 0;

  // Manually using unique_ptr with our Deleter
  {
    MockObject* rawPtr = Alloc::create<MockObject>(1, 1.0f);
    unique_ptr<MockObject> uptr(rawPtr);
    EXPECT_EQ(MockObject::constructed_count, 1);
  }

  // Deleter should have called destroy()
  EXPECT_EQ(MockObject::constructed_count, 0);
}
